/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// Flocking: the atoms of a collection fly as a flock round their parent, the butterflies or bats over a forest and the
// flies round a creature. The flock as a whole is drawn towards the parent; each atom keeps towards the flock's middle,
// away from its nearest neighbour, at the flock's speed, and banks into its turns.

#include <cmath>

#include <algorithm>
#include <numbers>
#include <string>

#include <ParticleFile.h>
#include <glm/geometric.hpp>

#include "ParticleClassRegistry.h"
#include "ParticleFlocking.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// Distances are never taken as shorter than this
constexpr float k_MinimumDistance = 0.01f;
constexpr float k_QuarterTurn = std::numbers::pi_v<float> * 0.5f;

/// How the pull of something falls off with distance: not at all, with the distance or its square, as a strength or,
/// inverted, as a weakness
enum class Falloff : uint8_t
{
	None,
	Linear,
	Square,
};

Falloff FalloffOf(float value)
{
	const auto type = static_cast<int>(value);
	return type == 1 ? Falloff::Linear : (type == 2 ? Falloff::Square : Falloff::None);
}

glm::vec3 Direction(glm::vec3 offset, float& distance)
{
	distance = glm::length(offset);
	return distance > 0.0f ? offset / distance : glm::vec3(0.0f);
}

glm::vec3 Clamped(glm::vec3 v, float most)
{
	const float length = glm::length(v);
	return length > most && length > 0.0f ? v * (most / length) : v;
}

class Flocking final: public Modifier
{
public:
	explicit Flocking(const ParticleObject& object)
	    : velocityMatching(object.Float("K_VelocityMatching", 0.0f))
	    , centralAttraction(object.Float("K_CentralAttraction", 0.0f))
	    , neighbourAccn(object.Float("K_NeighbourAccn", 0.0f))
	    , neighbourAvoidance(object.Float("K_NeighbourAvoidance", 0.0f))
	    , damping(object.Float("K_Damping", 0.0f))
	    , flockDamping(object.Float("K_FlockDamping", 0.0f))
	    , idealVel(object.Float("K_IdealVel", 0.0f))
	    , maxAccn(object.Float("F_MaxAccn", 0.0f))
	    , maxVel(object.Float("F_MaxVel", 0.0f))
	    , gravityForBanking(object.Float("GravityForBanking", 0.0f))
	    , reducePitchBy(object.Float("ReducePitchBy", 0.0f))
	    , scaleModifier(object.Float("ScaleModifier", 1.0f))
	    , centralFalloff(FalloffOf(object.Float("AxisChosen", 0.0f)))
	    , neighbourFalloff(FalloffOf(object.Float("NeghbourAccnType", 0.0f)))
	    , idealFalloff(FalloffOf(object.Float("IdealVelAccnType", 0.0f)))
	    , invertIdeal(object.Bool("F_InvertAccnIdealVel", false))
	    , invertCentral(object.Bool("F_InvertAccn", false))
	    , spriteRotation(object.Bool("SpriteRotation", false))
	    , invertNeighbour(object.Bool("NeighbourAccnInvert", false))
	    , localScale(object.String("LocalScaleFP"))
	{
	}

	[[nodiscard]] float Weight(float distance, Falloff falloff, bool invert) const
	{
		return flocking::Falloff(distance, static_cast<int>(falloff), invert, scaleModifier);
	}

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto n = collection.atoms.size();
		if (n == 0)
		{
			return true;
		}
		const float dt = effect.GetDt();
		// The flock is drawn towards its parent
		const auto origin = effect.GlobalToLocal(
		    collection, collection.parent != nullptr ? effect.GlobalPosition(*collection.parent) : effect.GetOrigin());
		glm::vec3 averagePosition(0.0f);
		glm::vec3 averageVelocity(0.0f);
		for (const auto& atom : collection.atoms)
		{
			averagePosition += atom->position;
			averageVelocity += atom->velocity;
		}
		averagePosition /= static_cast<float>(n);
		averageVelocity /= static_cast<float>(n);
		float toOrigin = 0.0f;
		const auto originDirection = Direction(origin - averagePosition, toOrigin);
		const auto ideal = originDirection * Weight(toOrigin, idealFalloff, invertIdeal) * idealVel;
		// The flock's own velocity, kept in the collection's slot
		const glm::vec3 oldFlock(slot.state);
		const glm::vec3 flock = (ideal * dt) + (oldFlock * (1.0f - (dt * flockDamping)));
		slot.state = glm::vec4(flock, 0.0f);
		const float scale = localScale.empty() ? 1.0f : effect.FloatProvider(localScale, 1.0f);
		const bool scaled = !localScale.empty();

		std::vector<glm::vec3> velocities;
		velocities.reserve(n);
		for (const auto& atom : collection.atoms)
		{
			glm::vec3 neighbours(0.0f);
			const Atom* nearest = nullptr;
			float nearestSquared = 0.0f;
			for (const auto& other : collection.atoms)
			{
				if (other == atom)
				{
					continue;
				}
				const auto offset = other->position - atom->position;
				float distance = 0.0f;
				const auto direction = Direction(offset, distance);
				if (scaled)
				{
					distance *= scale;
				}
				neighbours += direction * Weight(distance, neighbourFalloff, invertNeighbour);
				const float squared = glm::dot(offset, offset);
				if (nearest == nullptr || squared < nearestSquared)
				{
					nearest = other.get();
					nearestSquared = squared;
				}
			}
			if (nearest == nullptr)
			{
				// Alone, it keeps flying as it was
				velocities.push_back(atom->velocity);
				continue;
			}
			neighbours *= neighbourAccn / static_cast<float>(n);
			const auto relative = atom->velocity - oldFlock;
			float nearDistance = 0.0f;
			const auto nearDirection = Direction(nearest->position - atom->position, nearDistance);
			if (scaled)
			{
				nearDistance *= scale;
			}
			const auto avoid = nearDirection * Weight(nearDistance, Falloff::Square, false) * neighbourAvoidance;
			float toMiddle = 0.0f;
			const auto middleDirection = Direction(averagePosition - atom->position, toMiddle);
			const auto central = middleDirection * Weight(toMiddle, centralFalloff, invertCentral) * centralAttraction;
			auto matching = (averageVelocity - relative) * velocityMatching;
			if (scaled)
			{
				matching *= scale;
			}
			const auto acceleration = Clamped((central - avoid) + matching + neighbours, maxAccn);
			auto velocity = (relative * (1.0f - (dt * damping))) + (acceleration * dt) + flock;
			velocity = Clamped(velocity, maxVel);
			if (spriteRotation)
			{
				// Turned on the screen to face where it flies
				const auto right = effect.Services().world.CameraRight();
				const auto up = effect.Services().world.CameraUp();
				const float angle = std::atan2(-glm::dot(velocity, up), glm::dot(velocity, right)) + k_QuarterTurn;
				atom->rotation = glm::mat3(glm::vec3(std::cos(angle), 0.0f, -std::sin(angle)), glm::vec3(0.0f, 1.0f, 0.0f),
				                           glm::vec3(std::sin(angle), 0.0f, std::cos(angle)));
			}
			else
			{
				const auto accelerating = dt > 0.0f ? (velocity - atom->velocity) / dt : glm::vec3(0.0f);
				atom->rotation = flocking::Banking(velocity, accelerating, reducePitchBy, gravityForBanking);
			}
			velocities.push_back(velocity);
		}
		for (size_t i = 0; i < n; ++i)
		{
			auto& atom = *collection.atoms[i];
			atom.velocity = velocities[i];
			atom.position += atom.velocity * dt;
		}
		return true;
	}

	float velocityMatching;
	float centralAttraction;
	float neighbourAccn;
	float neighbourAvoidance;
	float damping;
	float flockDamping;
	float idealVel;
	float maxAccn;
	float maxVel;
	float gravityForBanking;
	float reducePitchBy;
	float scaleModifier;
	Falloff centralFalloff;
	Falloff neighbourFalloff;
	Falloff idealFalloff;
	bool invertIdeal;
	bool invertCentral;
	bool spriteRotation;
	bool invertNeighbour;
	std::string localScale;
};
} // namespace

float flocking::Falloff(float distance, int type, bool invert, float scaleModifier)
{
	const float x = std::max(distance, k_MinimumDistance) * scaleModifier;
	const float base = type == 1 ? x : (type == 2 ? x * x : 1.0f);
	if (invert)
	{
		return base;
	}
	return base != 0.0f ? 1.0f / base : 0.0f;
}

glm::mat3 flocking::Banking(glm::vec3 velocity, glm::vec3 acceleration, float reducePitchBy, float gravityForBanking)
{
	const float across = std::sqrt((velocity.x * velocity.x) + (velocity.z * velocity.z));
	const float yaw = std::atan2(velocity.z, velocity.x);
	const float pitch = std::atan2(velocity.y, across) * reducePitchBy;
	const float lateral = across == 0.0f ? 0.0f : ((acceleration.z * velocity.x) - (acceleration.x * velocity.z)) / across;
	const float bank = gravityForBanking != 0.0f ? std::atan(lateral / gravityForBanking) : 0.0f;
	// Lying along its heading, then banked, pitched by its climb and turned to its heading
	const float c0 = std::cos(-k_QuarterTurn);
	const float s0 = std::sin(-k_QuarterTurn);
	const float cb = std::cos(bank);
	const float sb = std::sin(bank);
	glm::mat3 m(glm::vec3(c0, s0 * sb, s0 * cb), glm::vec3(0.0f, cb, -sb), glm::vec3(-s0, c0 * sb, c0 * cb));
	const float cp = std::cos(-pitch);
	const float sp = std::sin(-pitch);
	const float cy = std::cos(yaw);
	const float sy = std::sin(yaw);
	for (int i = 0; i < 3; ++i)
	{
		const float x = (m[i].x * cp) + (m[i].y * sp);
		const float y = (m[i].y * cp) - (m[i].x * sp);
		m[i].x = (cy * x) - (sy * m[i].z);
		m[i].z = (sy * x) + (cy * m[i].z);
		m[i].y = y;
	}
	return m;
}

void openblack::particles::RegisterFlockingRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_Flocking", ParticleClassRegistry::Make<Flocking>);
}
