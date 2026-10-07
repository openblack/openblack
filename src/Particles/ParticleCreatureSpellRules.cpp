/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The particles of the spells cast on creatures. The first rule finds the creature the spell was cast on and keeps an
// atom at its feet, which plays the wisps' hum and the casting sound; the others hang off that atom. The wisps fly out
// of the hand and wind round the creature's body, fading as an invisible creature fades; hearts rise off a creature
// made nice, from all over its body; the flies of an itchy creature circle its head; steam comes off a frozen
// creature's feet.

#include <cmath>
#include <cstdint>

#include <algorithm>
#include <numbers>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>

#include "CreatureSpellMaths.h"
#include "ParticleClassRegistry.h"
#include "ParticleSounds.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// The invisible spell's kind, which fades the wisps as the creature fades
constexpr int k_InvisibleKind = 7;
/// Points about a bone the hearts and steam start from lie within this share of the creature's size of it
constexpr float k_AboutBone = 0.3f;
/// A creature casting a spell plays this sound for it rather than the spell's own
constexpr std::string_view k_CastByCreatureSound = "SOUND_SPELL_CREATURE_SPELL_CAST_BY_OTHER_CREATURE";

class CreatureSpell final: public Modifier
{
public:
	explicit CreatureSpell(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , hum({.action = object.Sound("SoundCreatureSpell")})
	    , cast({.action = object.Sound("SoundCreatureSpellCast")})
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		auto& world = effect.Services().world;
		// Once: an atom at the feet of the creature the spell was cast on
		if (slot.first)
		{
			slot.first = false;
			const auto creature = effect.TakeTarget();
			const auto body = creature.has_value() ? world.CreatureBody(*creature) : std::nullopt;
			if (!body.has_value())
			{
				return true;
			}
			auto& atom = effect.NewAtom(collection, effect.FindCreator(creator), nextGroups);
			atom.position = body->origin;
			atom.data[this].object = *creature;
			StartAtomSound(effect, atom, hum);
			// A spell a script casts makes no casting sound; one a creature casts sounds as cast by a creature
			const auto* sink = effect.GetSink();
			if (sink == nullptr || !sink->IsScriptCasting())
			{
				auto sound = cast;
				if (sink != nullptr && sink->IsCreatureCasting())
				{
					sound.action.sound = std::string(k_CastByCreatureSound);
				}
				StartAtomSound(effect, atom, sound);
			}
		}
		for (auto& atom : collection.atoms)
		{
			auto found = atom->data.find(this);
			if (found == atom->data.end())
			{
				continue;
			}
			// It stays at the creature's feet, and lets go of it once it has gone
			if (const auto body = world.CreatureBody(found->second.object); body.has_value())
			{
				atom->position = body->origin;
			}
			else
			{
				found->second.object = entt::null;
			}
			// Closing down, its sounds go
			if (effect.Closing())
			{
				StopAllAtomSounds(*atom);
			}
		}
		return true;
	}

	std::string creator;
	std::vector<int> nextGroups;
	ParticleSound hum;
	ParticleSound cast;
};

/// The creature a spell's atom keeps, from the first rule's data on its parent atom
std::optional<entt::entity> CreatureOf(const Collection& collection)
{
	if (collection.parent == nullptr)
	{
		return std::nullopt;
	}
	for (const auto& [modifier, data] : collection.parent->data)
	{
		if (dynamic_cast<const CreatureSpell*>(modifier) != nullptr && data.object != entt::null)
		{
			return data.object;
		}
	}
	return std::nullopt;
}

class CreatureSpellGeneric final: public Modifier
{
public:
	explicit CreatureSpellGeneric(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , numAtoms(object.Int("NumAtoms", 10))
	    , emitDuration(object.Float("EmitDuration", 2.0f))
	    , phiSpeed(object.Float("PhiSpeed", 1.0f))
	    , thetaSpeed(object.Float("ThetaSpeed", 2.0f))
	    , delayBeforeEmit(object.Float("DelayBeforeEmit", 1.0f))
	    , taperFrac(object.Float("TaperFrac", 1.0f))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto creature = CreatureOf(collection);
		auto& world = effect.Services().world;
		const auto body = creature.has_value() ? world.CreatureBody(*creature) : std::nullopt;
		const auto* sink = effect.GetSink();
		const int kind = sink != nullptr ? sink->CreatureSpellKind() : -1;
		if (!body.has_value())
		{
			return true;
		}
		const auto hand = effect.GetProcessInfo().handPosition;
		// Where the creature and the caster's hands were as the wisps began
		if (slot.first)
		{
			slot.first = false;
			slot.extra = glm::vec4(body->origin.x, body->origin.z, hand.x, hand.z);
		}
		const float dt = effect.GetDt();
		// The wisps come out one after another once the delay is over
		auto& due = slot.state.x;
		auto& made = slot.state.y;
		if (effect.CollectionAge(collection) > delayBeforeEmit && due < static_cast<float>(numAtoms))
		{
			due = maths::WispsDue(due, numAtoms, dt, emitDuration);
		}
		// The count made, a whole number, against the fraction due: the first comes as soon as any is due
		while (made < due)
		{
			made += 1.0f;
			auto& atom = effect.NewAtom(collection, effect.FindCreator(creator), nextGroups);
			auto thetaRate = thetaSpeed * effect.Random(0.5f, 1.5f);
			if (effect.Random(1.0f) < 0.5f)
			{
				thetaRate = -thetaRate;
			}
			const float phiRate = phiSpeed * effect.Random(0.5f, 1.5f);
			auto& data = atom.data[this];
			data.a = glm::vec4(0.0f, 0.0f, thetaRate, phiRate);
			data.b.x = atom.baseScale;
		}
		// As the creature goes invisible its wisps go with it, and come back as it ends
		if (kind == k_InvisibleKind && !effect.Closing())
		{
			collection.alpha = static_cast<float>(static_cast<int32_t>((1.0f - body->invisible) * 255.0f));
		}
		// A creature casting it gives up once the way from its hands to the creature has swung too far round
		if (!effect.Closing() && sink != nullptr && sink->IsCreatureCasting())
		{
			if (const auto limit = sink->MaxDirectionChange();
			    limit.has_value() &&
			    maths::SwungTooFar(glm::vec2(slot.extra.x, slot.extra.y) - glm::vec2(slot.extra.z, slot.extra.w),
			                       glm::vec2(body->origin.x, body->origin.z) - glm::vec2(hand.x, hand.z), *limit))
			{
				effect.GetSink()->CloseDown();
			}
		}
		const auto box = maths::WispBoxOf(body->bones);
		for (auto& atom : collection.atoms)
		{
			auto found = atom->data.find(this);
			if (found == atom->data.end())
			{
				continue;
			}
			auto& angles = found->second.a;
			angles.x = std::fmod(angles.x + dt * angles.z, k_TwoPi);
			angles.y = std::fmod(angles.y + dt * angles.w, k_TwoPi);
			const float age = effect.AtomAge(*atom);
			const auto orbit = maths::WispOrbit(box, angles.x, angles.y, taperFrac, age);
			atom->position = effect.GlobalToLocal(collection, maths::WispPosition(hand, orbit, age));
			atom->baseScale = found->second.b.x * body->size;
			atom->rgba[3] = maths::WispAlpha(age);
		}
		return true;
	}

	std::string creator;
	std::vector<int> nextGroups;
	int numAtoms;
	float emitDuration;
	float phiSpeed;
	float thetaSpeed;
	float delayBeforeEmit;
	float taperFrac;
};

/// A point within a share of the creature's size of one of its bones, any of them as likely
glm::vec3 AboutRandomBone(Effect& effect, const CreatureSpellBody& body)
{
	if (body.bones.empty())
	{
		return body.origin;
	}
	const auto bone = static_cast<size_t>(effect.Rand(static_cast<int32_t>(body.bones.size())));
	return body.bones.at(bone) + effect.RandomInBall() * (body.size * k_AboutBone);
}

class CreatureSpellCompassion final: public Modifier
{
public:
	explicit CreatureSpellCompassion(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , maxAtoms(object.Int("MaxAtoms", 20))
	    , dieAge(object.Float("DieAge", 5.0f))
	    , initSpeed(object.Float("InitSpeed", 10.0f))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto creature = CreatureOf(collection);
		const auto body = creature.has_value() ? effect.Services().world.CreatureBody(*creature) : std::nullopt;
		auto& due = slot.state.x;
		auto& made = slot.state.y;
		due += effect.GetDt() * static_cast<float>(maxAtoms) / dieAge;
		if (body.has_value() && !effect.Closing())
		{
			while (made < due && collection.atoms.size() < static_cast<size_t>(std::max(0, maxAtoms)))
			{
				made += 1.0f;
				auto& atom = effect.NewAtom(collection, effect.FindCreator(creator), nextGroups);
				atom.baseScale *= body->size;
				atom.position = effect.GlobalToLocal(collection, AboutRandomBone(effect, *body));
				atom.velocity = effect.RandomInBall() * initSpeed * body->size;
			}
		}
		std::erase_if(collection.atoms, [&](const auto& atom) { return effect.AtomAge(*atom) > dieAge; });
		return true;
	}

	std::string creator;
	std::vector<int> nextGroups;
	int maxAtoms;
	float dieAge;
	float initSpeed;
};

class CreatureSpellItch final: public Modifier
{
public:
	explicit CreatureSpellItch(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , numAtoms(object.Int("NumAtoms", 5))
	    , pauseBeforeGotoCreature(object.Float("PauseBeforeGotoCreature", 5.0f))
	    , orbitSpeed(object.Float("OrbitSpeed", 1.0f))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (slot.first)
		{
			slot.first = false;
			for (int i = 0; i < numAtoms; ++i)
			{
				effect.NewAtom(collection, effect.FindCreator(creator), nextGroups);
			}
		}
		const auto creature = CreatureOf(collection);
		const auto body = creature.has_value() ? effect.Services().world.CreatureBody(*creature) : std::nullopt;
		if (!body.has_value() || !body->rightEye.has_value() || *body->rightEye >= body->bones.size())
		{
			return true;
		}
		const float age = effect.CollectionAge(collection);
		const auto rightEye = body->bones.at(*body->rightEye);
		const auto leftBone = body->Mirror(*body->rightEye);
		const auto leftEye = leftBone < body->bones.size() ? body->bones.at(leftBone) : rightEye;
		// At the hand at first, then round the creature's head; the flies catch up as they flock
		const auto point = age <= pauseBeforeGotoCreature ? effect.GetProcessInfo().handPosition
		                                                  : maths::ItchOrbit(rightEye, leftEye, age, orbitSpeed);
		for (auto& atom : collection.atoms)
		{
			atom->position = effect.GlobalToLocal(collection, point);
		}
		return true;
	}

	std::string creator;
	std::vector<int> nextGroups;
	int numAtoms;
	float pauseBeforeGotoCreature;
	float orbitSpeed;
};

class CreatureSpellFreeze final: public Modifier
{
public:
	explicit CreatureSpellFreeze(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , maxAtoms(object.Int("MaxAtoms", 20))
	    , dieAge(object.Float("DieAge", 5.0f))
	    , initSpeed(object.Float("InitSpeed", 10.0f))
	    , minHeight(object.Float("MinHeight", 5.0f))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto creature = CreatureOf(collection);
		auto& world = effect.Services().world;
		const auto body = creature.has_value() ? world.CreatureBody(*creature) : std::nullopt;
		auto& due = slot.state.x;
		auto& made = slot.state.y;
		due += effect.GetDt() * static_cast<float>(maxAtoms) / dieAge;
		if (body.has_value() && body->rightFoot.has_value() && *body->rightFoot < body->bones.size() && !effect.Closing())
		{
			while (made < due && collection.atoms.size() < static_cast<size_t>(std::max(0, maxAtoms)))
			{
				made += 1.0f;
				auto& atom = effect.NewAtom(collection, effect.FindCreator(creator), nextGroups);
				// From either foot
				const auto bone = effect.Random(1.0f) >= 0.5f ? body->Mirror(*body->rightFoot) : *body->rightFoot;
				const auto foot = bone < body->bones.size() ? body->bones.at(bone) : body->origin;
				atom.position = effect.GlobalToLocal(collection, foot + effect.RandomInBall() * (body->size * k_AboutBone));
				atom.velocity = effect.RandomInBall() * initSpeed * body->size;
			}
		}
		// It stays above the land, and goes when old
		const float lift = minHeight * (body.has_value() ? body->size : 1.0f);
		for (auto& atom : collection.atoms)
		{
			auto at = effect.LocalToGlobal(collection, atom->position);
			const float least = world.LandHeight({at.x, at.z}) + lift;
			if (at.y < least)
			{
				at.y = least;
				atom->position = effect.GlobalToLocal(collection, at);
			}
		}
		std::erase_if(collection.atoms, [&](const auto& atom) { return effect.AtomAge(*atom) > dieAge; });
		return true;
	}

	std::string creator;
	std::vector<int> nextGroups;
	int maxAtoms;
	float dieAge;
	float initSpeed;
	float minHeight;
};

} // namespace

void openblack::particles::RegisterCreatureSpellRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_CreatureSpell", ParticleClassRegistry::Make<CreatureSpell>);
	registry.AddModifier("UR_CreatureSpellGeneric", ParticleClassRegistry::Make<CreatureSpellGeneric>);
	registry.AddModifier("UR_CreatureSpellCompassion", ParticleClassRegistry::Make<CreatureSpellCompassion>);
	registry.AddModifier("UR_CreatureSpellItch", ParticleClassRegistry::Make<CreatureSpellItch>);
	registry.AddModifier("UR_CreatureSpellFreeze", ParticleClassRegistry::Make<CreatureSpellFreeze>);
}
