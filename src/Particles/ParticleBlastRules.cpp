/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The beam explosion's rules: the explosion itself (its events, its beam and smoke, the wave that shatters what stands
// round it, the rubble or the rings it leaves and the rocks it throws up), the beam dropping and the cones spreading,
// the rule that closes the whole effect down, and the pieces objects break into

#include <cmath>

#include <algorithm>
#include <any>
#include <numbers>
#include <ranges>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ParticleBlast.h"
#include "ParticleClassRegistry.h"
#include "ParticleMaths.h"
#include "ParticleObjectEffects.h"
#include "ParticleShields.h"
#include "Resources/ResourceManager.h"

using namespace openblack;
using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// A spot visual's turns a second
constexpr float k_TurnsPerSecond = 10.0f;
/// The beam's spot visual lasts this many turns at most
constexpr int k_BeamTurns = 60;
/// The smoke and steam puff at this magnitude for so many seconds
constexpr float k_SmokeMagnitude = 8.0f;
constexpr float k_SmokeSeconds = 4.0f;
/// A shield over the centre is looked for this far up
constexpr float k_ShieldTestHeight = 200.0f;
/// A shield over an object the wave reaches is found this far round its point
constexpr float k_WaveShieldMargin = 2.0f;
/// Five rocks are thrown up, spread so far round the centre, each a size between these
constexpr int k_Rocks = 5;
constexpr float k_RockSpread = 4.0f;
constexpr float k_RockSmallest = 0.8f;
constexpr float k_RockLargest = 1.2f;
/// On the water three rings grow to these shares of the largest
constexpr float k_LargestRing = 10.0f;
constexpr std::array<float, 3> k_RingShares {0.5f, 0.7f, 1.0f};

/// The world's point of a rule's collection: its parent atom's, on the land
glm::vec3 CollectionPoint(const Effect& effect, const Collection& collection)
{
	glm::vec3 point = collection.parent != nullptr ? effect.GlobalPosition(*collection.parent) : effect.GetOrigin();
	point.y = effect.Services().world.LandHeight({point.x, point.z});
	return point;
}

/// The whole effect closes down once the rule's condition holds: its rules marked for it stop, and what fades after
/// closing down fades
class SetPSysCloseDown final: public Modifier
{
public:
	bool ModifyCollection(Effect& effect, Collection& /*collection*/, Collection::Slot& /*slot*/) const override
	{
		effect.CloseDown();
		return false;
	}
};

/// An atom moves in its frame from one point to another over a time, steadily or easing in and out; outside the time it
/// is left where it is
class MoveAtom final: public Modifier
{
public:
	explicit MoveAtom(const ParticleObject& object)
	    : start(object.Float("StartTime", 0.0f))
	    , stop(object.Float("StopTime", 1.0f))
	    , from(object.Float("StartX", 0.0f), object.Float("StartY", 0.0f), object.Float("StartZ", 0.0f))
	    , to(object.Float("StopX", 0.0f), object.Float("StopY", 0.0f), object.Float("StopZ", 0.0f))
	    , smoothly(object.Bool("MoveSmoothly", false))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		if (const auto t = blast::MoveFraction(effect.AtomAge(atom), effect.GetDt(), start, stop, smoothly))
		{
			atom.position = from + (to - from) * *t;
		}
		return true;
	}

	float start;
	float stop;
	glm::vec3 from;
	glm::vec3 to;
	bool smoothly;
};

/// An atom's scale across and its height scale apart over a time: the height drawn is its own, whatever the scale across
class ChangeScaleXYZ final: public Modifier
{
public:
	explicit ChangeScaleXYZ(const ParticleObject& object)
	    : start(object.Float("StartTime", 0.0f))
	    , stop(object.Float("StopTime", 1.0f))
	    , fromXZ(object.Float("StartScaleXZ", 1.0f))
	    , toXZ(object.Float("StopScaleXZ", 1.0f))
	    , fromY(object.Float("StartScaleY", 1.0f))
	    , toY(object.Float("StopScaleY", 1.0f))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		const float age = effect.AtomAge(atom);
		const float dt = effect.GetDt();
		const auto across = maths::TimedValue(age, dt, start, stop, fromXZ, toXZ);
		const auto height = maths::TimedValue(age, dt, start, stop, fromY, toY);
		if (across.has_value() && height.has_value())
		{
			const auto scale = blast::ScaleXYZ(*across, *height);
			atom.ruleScale = scale.across;
			atom.stretch = scale.stretch;
		}
		return true;
	}

	float start;
	float stop;
	float fromXZ;
	float toXZ;
	float fromY;
	float toY;
};

/// What an explosion keeps
struct ExplosionState
{
	bool beamStarted {false};
	uint32_t beam {0};
	bool started {false};
	bool cancelled {false};
	bool closed {false};
	bool smoked {false};
	/// How far the wave has spread
	float front {0.0f};
	/// The objects the wave is yet to reach
	std::vector<WaveTarget> wave;
	int exploded {0};
	int removed {0};
};

/// One explosion of the beam, from its parent atom's point on the land: the beam visual at once; after a short delay a
/// shield over it may stop it, or it leaves rubble (or rings on the water), throws up rocks and gathers the fixed objects
/// round it for its wave; then for some seconds it sends its miracle an event every step, puffs smoke or steam once, and
/// its wave spreads, shattering one object a step. Closing down stops it all.
class Explosion final: public Modifier
{
public:
	explicit Explosion(const ParticleObject& object)
	    : rules({
	          .maxObjectsToDelete = object.Int("MaxObjectsToDelete", 20),
	          .maxObjectsToExplode = object.Int("MaxObjectsToExplode", 20),
	          .maxDistance = object.Float("MaxDistance", 100.0f),
	          .blastSpeed = object.Float("BlastSpeed", 10.0f),
	          .spreadSpeed = object.Float("SpreadSpeed", 10.0f),
	          .timeToDoEventsFor = object.Float("TimeToDoEventsFor", 5.0f),
	          .initialDelay = object.Float("InitialDelay", 3.5f),
	          .smokeDelay = object.Float("SmokeDelay", 3.0f),
	          .beamDelay = object.Float("BeamDelay", 0.0f),
	      })
	{
	}

	[[nodiscard]] bool KeepsAlive() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (!slot.data.has_value())
		{
			slot.data = ExplosionState {};
		}
		auto& state = std::any_cast<ExplosionState&>(slot.data);
		auto* objects = effect.Services().world.ObjectEffects();
		const auto centre = CollectionPoint(effect, collection);
		const float age = effect.CollectionAge(collection);
		if (effect.Closing() || state.cancelled)
		{
			// Everything stops: the beam is closed and the wave forgotten
			if (!state.closed)
			{
				state.closed = true;
				if (objects != nullptr && state.beam != 0)
				{
					objects->CloseSpotVisual(state.beam);
				}
				state.wave.clear();
			}
			return true;
		}
		if (objects != nullptr && !state.beamStarted && age > rules.beamDelay)
		{
			state.beamStarted = true;
			state.beam = objects->StartSpotVisual(SpotVisualType::BeamExplosionFx, centre, 1.0f, k_BeamTurns);
		}
		if (!state.started && age > rules.initialDelay)
		{
			state.started = true;
			Start(effect, state, centre, objects);
			if (state.cancelled)
			{
				return true;
			}
		}
		if (!state.started)
		{
			return true;
		}
		if (blast::SendsEvents(age, rules))
		{
			effect.SendSpellEvent({.type = SpellEventInfo::Type::Point,
			                       .position = centre,
			                       .velocity = glm::vec3(0.0f),
			                       .strength = 1.0f,
			                       .checkShields = false,
			                       .target = entt::null});
		}
		if (objects != nullptr && !state.smoked && age > rules.smokeDelay)
		{
			state.smoked = true;
			const auto type = objects->IsDryLand(centre) ? SpotVisualType::Smoke : SpotVisualType::Steam;
			objects->StartSpotVisual(type, centre, k_SmokeMagnitude, static_cast<int>(k_SmokeSeconds * k_TurnsPerSecond));
		}
		if (objects != nullptr)
		{
			Wave(effect, state, centre, *objects);
		}
		return true;
	}

private:
	/// A shield over it stops it; else it marks the land, gathers its wave and throws up rocks
	void Start(Effect& effect, ExplosionState& state, const glm::vec3& centre, ObjectEffectsInterface* objects) const
	{
		const auto* sink = effect.GetSink();
		const float margin = sink != nullptr ? sink->EffectRadius() : blast::k_DefaultEffectRadius;
		// A shield over it sparks where the way from high above down to it enters the shield, grown by the blast's
		// reach, and may stop it
		if (const auto shield = effect.Services().world.FindShield(centre, margin))
		{
			const auto top = centre + glm::vec3(0.0f, k_ShieldTestHeight, 0.0f);
			StrikeShield(*shield, blast::EntersSphere(top, centre, shield->centre, shield->radius + margin));
			if (!effect.SendSpellEvent({.type = SpellEventInfo::Type::HitSpell,
			                            .position = centre,
			                            .velocity = glm::vec3(0.0f),
			                            .strength = 1.0f,
			                            .checkShields = false,
			                            .target = shield->spell}))
			{
				state.cancelled = true;
				return;
			}
		}
		if (objects == nullptr)
		{
			return;
		}
		if (objects->IsDryLand(centre))
		{
			objects->AddRubbleMark(centre);
		}
		else
		{
			for (const float share : k_RingShares)
			{
				objects->AddWaterRing(centre, k_LargestRing * share);
			}
		}
		const float tribalPower = sink != nullptr ? sink->TribalPower() : 1.0f;
		state.wave = objects->FixedObjectsNear(centre, blast::WaveRange(rules.maxDistance, tribalPower));
		state.front = 0.0f;
		const auto origin = blast::FragmentOrigin(centre);
		for (int i = 0; i < k_Rocks; ++i)
		{
			// Drawn across then along, then its size and the way it faces; each starts at the centre's height, whatever
			// the land under it
			const float z = effect.Random(-k_RockSpread, k_RockSpread);
			const float x = effect.Random(-k_RockSpread, k_RockSpread);
			const glm::vec3 at(centre.x + x, centre.y, centre.z + z);
			const float scale = effect.Random(k_RockSmallest, k_RockLargest);
			const float yaw = effect.Random(0.0f, k_TwoPi);
			const auto transform = glm::scale(
			    glm::rotate(glm::translate(glm::mat4(1.0f), at), yaw, glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(scale));
			objects->ExplodeMesh(resources::HashIdentifier(blast::k_RockMesh), transform, origin, rules.blastSpeed);
		}
	}

	/// The wave spreads; of the objects it has reached, the first that it may destroy shatters and goes
	void Wave(Effect& effect, ExplosionState& state, const glm::vec3& centre, ObjectEffectsInterface& objects) const
	{
		state.front += rules.spreadSpeed * effect.GetDt();
		std::erase_if(state.wave, [&objects](WaveTarget& target) {
			const auto still = objects.Available(target.object);
			if (still.has_value())
			{
				target = *still;
			}
			return !still.has_value();
		});
		if (state.exploded >= rules.maxObjectsToExplode && state.removed >= rules.maxObjectsToDelete)
		{
			state.wave.clear();
			return;
		}
		for (auto it = state.wave.begin(); it != state.wave.end();)
		{
			if (!blast::Reached(state.front, *it, centre))
			{
				++it;
				continue;
			}
			const auto target = *it;
			it = state.wave.erase(it);
			// A shield over what it reaches sparks there, and takes the wave's blow outwards from the centre
			if (const auto shield = effect.Services().world.FindShield(target.point, k_WaveShieldMargin))
			{
				StrikeShield(*shield, target.point);
				if (!effect.SendSpellEvent({.type = SpellEventInfo::Type::HitSpell,
				                            .position = target.point,
				                            .velocity = target.point - centre,
				                            .strength = 1.0f,
				                            .checkShields = false,
				                            .target = shield->spell}))
				{
					continue;
				}
			}
			if (!effect.SendSpellEvent({.type = SpellEventInfo::Type::Capture,
			                            .position = target.point,
			                            .velocity = glm::vec3(0.0f),
			                            .strength = 1.0f,
			                            .checkShields = false,
			                            .target = target.object}))
			{
				continue;
			}
			// The one object this step
			if (!objects.IsCreature(target.object) && state.exploded < rules.maxObjectsToExplode)
			{
				++state.exploded;
				objects.ExplodeObject(target.object, blast::FragmentOrigin(centre), rules.blastSpeed);
			}
			if (state.removed < rules.maxObjectsToDelete)
			{
				++state.removed;
				objects.DestroyByBeam(target.object);
			}
			break;
		}
	}

	blast::ExplosionRules rules;
};

/// The pieces the objects break into: each waiting model breaks into chains of a few joined triangles, each a piece
/// flying out from below the blast's centre, a little at random
class ExplodeObject final: public Modifier
{
public:
	ExplodeObject(const ParticleObject& object, int queue)
	    : randomFactor(object.Float("RandomFactor", 0.3f))
	    , queue(queue)
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		auto* objects = effect.Services().world.ObjectEffects();
		auto* resources = effect.Services().world.FragmentSource();
		if (objects == nullptr)
		{
			return true;
		}
		// The newest first
		const auto records = objects->TakeExplodeRecords(queue);
		for (const auto& record : records | std::views::reverse)
		{
			const auto pieces = resources != nullptr ? resources->Fragments(record.mesh) : nullptr;
			if (pieces == nullptr)
			{
				continue;
			}
			for (const auto& piece : *pieces)
			{
				const auto placed = blast::PlaceFragment(piece, record.transform, record.mesh);
				auto& atom = effect.NewAtom(collection, &creator, {});
				atom.position = placed.centre;
				atom.fragment = placed.shape;
				auto random = effect.RandomInBall();
				const float length = glm::length(random);
				random = length > 0.0f ? random / length : glm::vec3(0.0f, 1.0f, 0.0f);
				atom.velocity = blast::FragmentVelocity(placed.centre, record.origin, record.speed, randomFactor, random);
			}
		}
		return true;
	}

	float randomFactor;
	int queue;
	Creator creator {[] {
		Creator fragment;
		fragment.kind = Creator::Kind::Fragment;
		fragment.className = "MeshFragment";
		return fragment;
	}()};
};
} // namespace

void openblack::particles::RegisterBlastRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("SetPSysCloseDown", ParticleClassRegistry::Make<SetPSysCloseDown>);
	registry.AddModifier("UR_MoveAtom", ParticleClassRegistry::Make<MoveAtom>);
	registry.AddModifier("UR_ChangeScaleXYZ", ParticleClassRegistry::Make<ChangeScaleXYZ>);
	registry.AddModifier("UR_Explosion", ParticleClassRegistry::Make<Explosion>);
	registry.AddModifier("UR_ExplodeObject",
	                     [](const ParticleObject& object) { return std::make_unique<ExplodeObject>(object, 0); });
	// The second queue's pieces are made the same way, tumbling longer; nothing in the game fills it
	registry.AddModifier("UR_ExplodeObject2",
	                     [](const ParticleObject& object) { return std::make_unique<ExplodeObject>(object, 1); });
}
