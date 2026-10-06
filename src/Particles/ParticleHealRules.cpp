/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The heal miracle's rules: a chakra over each person the miracle heals, which heals them as it appears, follows them
// and lights them as the burst of sparks under it rises and fades; the burst itself, sparks flying out of a point once
// its collection is old enough; and the chakras' wiggle while the miracle is in the hand

#include <cmath>

#include <numbers>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>

#include "ParticleClassRegistry.h"
#include "ParticleMaths.h"
#include "ParticleSounds.h"

using namespace openblack;
using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;

/// A chakra over each target the miracle gives the effect that no other effect has, which heals it as it appears. Each
/// chakra follows its target, scaled to it when asked, and fades with the burst under it: up to full at one age of the
/// burst and back to nothing at another, lighting the target in its colour as it does. A chakra ends when its burst has
/// gone, or its target has gone or been picked up. It keeps an effect waiting for targets until the effect closes.
class HealSpellChakra final: public Modifier
{
public:
	explicit HealSpellChakra(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , scaleToObject(object.Bool("ScalePropObjectSize", false))
	    , takeCentre(object.Bool("TakeCentrePos", false))
	    , soundHeal({.action = object.Sound("SoundHeal")})
	    , maxAlpha(object.Float("MaxAlpha", 255.0f))
	    , ageMaxAlpha(object.Float("AtomAgeMaxAlpha", 1.0f))
	    , ageZeroAlpha(object.Float("AtomAgeZeroAlpha", 3.0f))
	    , glow(static_cast<float>(object.Int("SpecularColorR", 0)), static_cast<float>(object.Int("SpecularColorG", 0)),
	           static_cast<float>(object.Int("SpecularColorB", 0)))
	{
	}
	[[nodiscard]] bool KeepsAlive() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		auto& world = effect.Services().world;
		for (auto target = effect.TakeTarget(); target.has_value(); target = effect.TakeTarget())
		{
			const auto info = world.Target(*target, takeCentre);
			if (!info.has_value() || world.IsTargetClaimed(*target))
			{
				continue;
			}
			effect.SendSpellEvent({.type = SpellEventInfo::Type::Object,
			                       .position = info->position,
			                       .velocity = glm::vec3(0.0f),
			                       .strength = 1.0f,
			                       .checkShields = false,
			                       .target = *target});
			auto& atom = effect.NewAtom(collection, effect.FindCreator(creator), nextGroups);
			auto& data = atom.data[this];
			data.object = *target;
			atom.position = info->position;
			world.ClaimTarget(*target, true);
		}
		// The newest chakra plays the heal once it has lived a step
		const Atom* newest = collection.atoms.empty() ? nullptr : collection.atoms.back().get();
		for (size_t i = 0; i < collection.atoms.size();)
		{
			auto& atom = *collection.atoms[i];
			const auto found = atom.data.find(this);
			if (found == atom.data.end())
			{
				++i;
				continue;
			}
			auto& data = found->second;
			if (&atom == newest && !data.started && effect.AtomAge(atom) > 0.0f)
			{
				data.started = true;
				StartAtomSound(effect, atom, soundHeal);
			}
			bool done = false;
			const auto info = data.object != entt::null ? world.Target(data.object, takeCentre) : std::nullopt;
			if (info.has_value() && !world.IsTargetHeld(data.object))
			{
				atom.position = info->position;
				if (scaleToObject)
				{
					atom.ruleScale = info->radius;
				}
				if (atom.subCollections.empty())
				{
					// A chakra without a burst under it lets the rule go
					return false;
				}
				done = Fade(effect, *atom.subCollections.front(), atom, data);
			}
			else
			{
				done = true;
			}
			if (done)
			{
				if (data.object != entt::null)
				{
					world.SetTargetGlow(data.object, glm::u8vec3(0));
					world.ClaimTarget(data.object, false);
				}
				collection.atoms.erase(collection.atoms.begin() + static_cast<std::ptrdiff_t>(i));
				continue;
			}
			++i;
		}
		return true;
	}

private:
	/// The burst's fade and the target's glow; true once the burst is over. The burst is made on the chakra's first
	/// step, so it is only over once the chakra has lived a step.
	bool Fade(const Effect& effect, Collection& burst, const Atom& chakra, const AtomRuleData& data) const
	{
		if (burst.atoms.empty())
		{
			return effect.AtomAge(chakra) > 0.0f;
		}
		const float t = maths::ChakraFade(effect.CollectionAge(burst), ageMaxAlpha, ageZeroAlpha);
		const auto alpha = maths::TruncateToByte(t * maxAlpha);
		effect.Services().world.SetTargetGlow(data.object,
		                                      glm::u8vec3(maths::TruncateToByte(glow.r * t), maths::TruncateToByte(glow.g * t),
		                                                  maths::TruncateToByte(glow.b * t)));
		for (auto& spark : burst.atoms)
		{
			spark->rgba[3] = alpha;
		}
		return false;
	}

	std::string creator;
	std::vector<int> nextGroups;
	bool scaleToObject;
	bool takeCentre;
	ParticleSound soundHeal;
	float maxAlpha;
	float ageMaxAlpha;
	float ageZeroAlpha;
	glm::vec3 glow;
};

/// Once its collection is old enough, a number of atoms fly out of the spawn point all at once in random directions,
/// only upwards when asked, at one random speed between two providers' values, the vertical scaled. The first plays the
/// sound, and the parent atom may be hidden. Then the rule is done.
class CreateRuleFusedSphericalExplode final: public Modifier
{
public:
	explicit CreateRuleFusedSphericalExplode(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , count(object.Int("NumAtoms", 100))
	    , minSpeed(object.String("MinSpeed"))
	    , maxSpeed(object.String("MaxSpeed"))
	    , fuseTime(object.Float("FuseTime", 1.0f))
	    , scaleYSpeed(object.Float("ScaleYSpeed", 1.0f))
	    , hemisphere(object.Bool("OnlyHemisphere", false))
	    , disableParent(object.Bool("DisableParent", true))
	    , sound({.action = object.Sound("SoundExplode")})
	{
	}
	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		const auto* atomCreator = effect.FindCreator(creator);
		if (atomCreator == nullptr || minSpeed.empty() || maxSpeed.empty())
		{
			return false;
		}
		if (effect.CollectionAge(collection) < fuseTime)
		{
			return true;
		}
		const float low = effect.FloatProvider(minSpeed, 0.0f);
		const float speed = effect.Random(effect.FloatProvider(maxSpeed, 0.0f) - low) + low;
		for (int i = 0; i < count; ++i)
		{
			auto& atom = effect.NewAtom(collection, atomCreator, nextGroups);
			glm::vec3 direction(0.0f);
			while (glm::dot(direction, direction) == 0.0f)
			{
				direction = effect.RandomInBall();
			}
			if (hemisphere)
			{
				direction.y = std::abs(direction.y);
			}
			direction *= 1.0f / std::sqrt(glm::dot(direction, direction));
			atom.velocity = glm::vec3(direction.x, direction.y * scaleYSpeed, direction.z) * speed;
			if (i == 0)
			{
				StartAtomSound(effect, atom, sound);
			}
		}
		if (disableParent && collection.parent != nullptr)
		{
			collection.parent->visible = false;
		}
		return false;
	}

private:
	std::string creator;
	std::vector<int> nextGroups;
	int count;
	std::string minSpeed;
	std::string maxSpeed;
	float fuseTime;
	float scaleYSpeed;
	bool hemisphere;
	bool disableParent;
	ParticleSound sound;
};

/// While the miracle is in the hand its chakras swing through the parent's point, out and back, the odd ones the other
/// way, so many times a second
class HealInHand final: public Modifier
{
public:
	explicit HealInHand(const ParticleObject& object)
	    : wiggle(object.Float("WiggleFreq", 1.0f))
	{
	}
	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		const glm::vec3 parent = collection.parent != nullptr ? collection.parent->position : effect.GetOrigin();
		const float swing = std::sin(effect.CollectionAge(collection) * wiggle * k_TwoPi);
		bool odd = false;
		// In the game's order, the newest first
		for (auto it = collection.atoms.rbegin(); it != collection.atoms.rend(); ++it)
		{
			(*it)->position = parent * (odd ? -swing : swing);
			odd = !odd;
		}
		return true;
	}

private:
	float wiggle;
};
} // namespace

void openblack::particles::RegisterHealRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_HealSpellChakra", ParticleClassRegistry::Make<HealSpellChakra>);
	registry.AddModifier("CreateRuleFusedSphericalExplode", ParticleClassRegistry::Make<CreateRuleFusedSphericalExplode>);
	registry.AddModifier("UR_HealInHand", ParticleClassRegistry::Make<HealInHand>);
}
