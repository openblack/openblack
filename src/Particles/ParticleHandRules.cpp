/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The rules that keep particles on the casting hand: the miracle held in the hand follows it, the gesture trail sits
// on it, the sprinkling miracles (food, wood, water) pour from a source that follows it, and their logs tumble as they
// fall

#include <cmath>

#include <algorithm>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ParticleClassRegistry.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// The shortest step the rules divide a move by, so that a zero step doesn't make an endless speed
constexpr float k_MinimumStep = 1e-4f;
/// The sprinkling source keeps no higher than this above the land, however high the hand is
constexpr float k_MaximumSprinkleHeight = 58.0f;

/// A point of the world in an atom's collection's frame
glm::vec3 ToCollection(const Effect& effect, const Atom& atom, const glm::vec3& global)
{
	return atom.collection != nullptr ? effect.GlobalToLocal(*atom.collection, global) : global;
}

/// Each atom goes to the hand, its speed the move it made
class FollowLocalHand final: public Modifier
{
public:
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		const auto local = ToCollection(effect, atom, effect.GetProcessInfo().handPosition);
		atom.velocity = (local - atom.position) / std::max(effect.GetDt(), k_MinimumStep);
		atom.position = local;
		return true;
	}
};

/// Each atom sits on the hand, without a speed
class FollowCastPosition final: public Modifier
{
public:
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		atom.position = ToCollection(effect, atom, effect.GetProcessInfo().handPosition);
		return true;
	}
};

/// One source atom that follows the hand, no higher than a limit above the land, from which the sprinkled grains, logs
/// or drops fall. Its speed is the hand's rise or fall, plus a lift of its own when a human player casts.
class HandSprinkle final: public Modifier
{
public:
	explicit HandSprinkle(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , liftForHuman(object.Float("InitSpeedYHumanPlayerCasting", 0.0f))
	{
	}

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		auto target = effect.GetProcessInfo().handPosition;
		const float land = effect.Services().world.LandHeight({target.x, target.z});
		target.y = std::min(target.y, land + k_MaximumSprinkleHeight);
		if (slot.first)
		{
			slot.first = false;
			const auto* source = effect.FindCreator(creator);
			if (source == nullptr)
			{
				return false;
			}
			effect.NewAtom(collection, source, nextGroups).position = target;
		}
		const float dt = std::max(effect.GetDt(), k_MinimumStep);
		const float lift = effect.IsHumanPlayerCasting() ? liftForHuman : 0.0f;
		for (auto& atom : collection.atoms)
		{
			atom->velocity = glm::vec3(0.0f, (target.y - atom->position.y) / dt + lift, 0.0f);
			atom->position = target;
		}
		return true;
	}

	std::string creator;
	std::vector<int> nextGroups;
	float liftForHuman;
};

/// The atom turns as it moves, by its speed times the tumble speed (at most the maximum when restricted): about the
/// forward axis when it moves more east to west than north to south, else about the east to west axis
class Tumble final: public Modifier
{
public:
	explicit Tumble(const ParticleObject& object)
	    : tumbleSpeed(object.Float("TumbleSpeed", 0.0f))
	    , maxTumbleSpeed(object.Float("MaxTumbleSpeed", 0.0f))
	    , restrict(object.Bool("RestrictMaxRotation", false))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		const auto& v = atom.velocity;
		float speed = glm::length(v) * tumbleSpeed;
		if (restrict)
		{
			speed = std::clamp(speed, -maxTumbleSpeed, maxTumbleSpeed);
		}
		const auto axis = std::abs(v.z) < std::abs(v.x) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
		atom.rotation = glm::mat3(glm::rotate(glm::mat4(1.0f), speed * effect.GetDt(), axis)) * atom.rotation;
		return true;
	}

	float tumbleSpeed;
	float maxTumbleSpeed;
	bool restrict;
};
} // namespace

void openblack::particles::RegisterHandRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_FollowLocalHand", ParticleClassRegistry::Make<FollowLocalHand>);
	registry.AddModifier("UR_FollowCastPosn", ParticleClassRegistry::Make<FollowCastPosition>);
	registry.AddModifier("UR_HandSprinkle", ParticleClassRegistry::Make<HandSprinkle>);
	registry.AddModifier("AppearanceRuleTumble", ParticleClassRegistry::Make<Tumble>);
}
