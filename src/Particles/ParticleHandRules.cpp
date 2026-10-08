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
#include <any>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ParticleClassRegistry.h"
#include "ParticleMaths.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// The shortest step the rules divide a move by, so that a zero step doesn't make an endless speed
constexpr float k_MinimumStep = 1e-4f;

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
		target.y = std::min(target.y, land + openblack::particles::k_MaximumSprinkleHeight);
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
/// The stream of what the hand scoops: while the effect runs, atoms are let out at a rate a second, each starting on the
/// land under the hand, and each rises in a straight line to wherever the hand now is over the rise time, its speed the
/// move it made, and goes once that time has passed
class MultiPickup final: public Modifier
{
public:
	explicit MultiPickup(const ParticleObject& object)
	    : _creator(object.String("PCreator"))
	    , _nextGroups(object.IntArray("NextGroups"))
	    , _emitRate(object.Float("EmitRate", 0.0f))
	    , _raiseTime(object.Float("RaiseTime", 0.0f))
	    , _randomOrientations(object.Bool("RandomiseOrientations", false))
	    , _defaultOrientation(object.Float("DefaultOrientation", 0.0f))
	{
	}
	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto* atomCreator = effect.FindCreator(_creator);
		if (atomCreator == nullptr)
		{
			return false;
		}
		if (!slot.data.has_value())
		{
			slot.data = State {};
		}
		auto& state = std::any_cast<State&>(slot.data);
		const auto hand = effect.GetProcessInfo().handPosition;
		// Atoms are let out only while the effect runs, not as it closes down
		if (!effect.Closing())
		{
			state.owed += effect.GetDt() * _emitRate;
			while (static_cast<float>(state.emitted) < state.owed)
			{
				++state.emitted;
				auto& atom = effect.NewAtom(collection, atomCreator, _nextGroups);
				// TODO(hand): a random orientation, which none of the game's files asks for
				atom.rotation = _randomOrientations ? atom.rotation : maths::AngleY(_defaultOrientation);
				const glm::vec3 start(hand.x, effect.Services().world.LandHeight({hand.x, hand.z}), hand.z);
				atom.data[this].a = glm::vec4(start, 0.0f);
				atom.position = effect.GlobalToLocal(collection, start);
			}
		}
		const float step = std::max(effect.GetDt(), k_MinimumStep);
		for (size_t i = 0; i < collection.atoms.size();)
		{
			auto& atom = *collection.atoms[i];
			const float age = effect.AtomAge(atom);
			if (age > _raiseTime)
			{
				collection.atoms.erase(collection.atoms.begin() + static_cast<std::ptrdiff_t>(i));
				continue;
			}
			const auto start = glm::vec3(atom.data[this].a);
			const auto risen = start + (hand - start) * (age / _raiseTime);
			const auto local = effect.GlobalToLocal(collection, risen);
			atom.velocity = (local - atom.position) / step;
			atom.position = local;
			++i;
		}
		return true;
	}

private:
	struct State
	{
		float owed {0.0f};
		int emitted {0};
	};

	std::string _creator;
	std::vector<int> _nextGroups;
	float _emitRate;
	float _raiseTime;
	bool _randomOrientations;
	float _defaultOrientation;
};
} // namespace

void openblack::particles::RegisterHandRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_FollowLocalHand", ParticleClassRegistry::Make<FollowLocalHand>);
	registry.AddModifier("UR_FollowCastPosn", ParticleClassRegistry::Make<FollowCastPosition>);
	registry.AddModifier("UR_HandSprinkle", ParticleClassRegistry::Make<HandSprinkle>);
	registry.AddModifier("AppearanceRuleTumble", ParticleClassRegistry::Make<Tumble>);
	registry.AddModifier("ER_MultiPickup", ParticleClassRegistry::Make<MultiPickup>);
}
