/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The symbols of belief that rise from the people a miracle impresses. Each symbol waiting is made an atom where it
// rises from, in the believed player's colour, the more belief the more opaque; one worth more than a little plays its
// sound. Each rises at its own speed, wiggling across the land more as it gets going, pulses in size, fades out after a
// second and a half and is gone soon after.

#include <cmath>

#include <algorithm>
#include <string>

#include <ParticleFile.h>

#include "ParticleClassRegistry.h"
#include "ParticleSounds.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// Each symbol rises up to this much faster for each of its thirteen steps of its own number
constexpr float k_RiseStep = 0.0249f;
constexpr uint32_t k_RiseSteps = 13;
/// The symbol's own number makes its size pulse at its own pace
constexpr float k_PulsePhase = 0.23f;
constexpr float k_PulsePace = 0.3f / 256.0f;
/// A symbol is gone this long after it has faded out
constexpr float k_GoneAfterFading = 0.3f;

class BeliefSpriteRule final: public Modifier
{
public:
	explicit BeliefSpriteRule(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , riseSpeed(object.Float("RiseSpeed", 12.0f))
	    , maxWiggle(object.Float("MaxWiggleOffset", 2.0f))
	    , ageAtMaxOffset(object.Float("AgeAtMaxOffset", 0.5f))
	    , noiseFrequency(object.Float("NoiseFreq", 2.0f))
	    , scaleMax(object.Float("ScaleMax", 1.0f))
	    , scaleMin(object.Float("ScaleMin", 0.0f))
	    , scaleFrequency(object.Float("ScaleFreq", 12.0f))
	    , fadeTimeStart(object.Float("FadeTimeStart", 1.5f))
	    , fadeTimeEnd(object.Float("FadeTimeEnd", 2.5f))
	    , alphaMin(object.Int("AlphaMin", 20))
	    , alphaMax(object.Int("AlphaMax", 150))
	    , valueAlphaMin(object.Int("ValueAlphaMin", 10))
	    , valueAlphaMax(object.Int("ValueAlphaMax", 100))
	    , sound({.action = object.Sound("SoundOfBelief")})
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		const auto* spriteCreator = effect.FindCreator(creator);
		if (spriteCreator == nullptr)
		{
			return true;
		}
		auto& world = effect.Services().world;
		for (auto sprite = world.TakeBeliefSprite(); sprite.has_value(); sprite = world.TakeBeliefSprite())
		{
			auto& atom = effect.NewAtom(collection, spriteCreator, {});
			auto& data = atom.data[this];
			data.a = glm::vec4(sprite->position, 0.0f);
			// The more belief, the more opaque it starts
			const float share = std::clamp(static_cast<float>(sprite->amount - valueAlphaMin) /
			                                   static_cast<float>(valueAlphaMax - valueAlphaMin),
			                               0.0f, 1.0f);
			data.b.x = std::round(static_cast<float>(alphaMax - alphaMin) * share + static_cast<float>(alphaMin));
			atom.position = sprite->position;
			atom.rgba = {static_cast<uint8_t>(sprite->colour >> 16u), static_cast<uint8_t>(sprite->colour >> 8u),
			             static_cast<uint8_t>(sprite->colour), atom.rgba[3]};
			atom.ruleScale = 1.0f;
			if (sprite->amount > valueAlphaMin)
			{
				StartAtomSound(effect, atom, sound);
			}
		}
		const auto& noise = effect.Services().noise;
		for (size_t i = 0; i < collection.atoms.size();)
		{
			auto& atom = *collection.atoms[i];
			const auto found = atom.data.find(this);
			if (found == atom.data.end())
			{
				++i;
				continue;
			}
			const auto& data = found->second;
			const float age = effect.AtomAge(atom);
			const auto id = static_cast<float>(atom.random);
			const float along = age * noiseFrequency;
			const float wiggleX = noise.Smooth(id + along * 2.0f) * 0.5f + noise.Smooth(id + along);
			const float wiggleZ = noise.Smooth(id * 0.3f + along * 2.0f) * 0.5f + noise.Smooth(id * 0.3f + along);
			const float ramp = std::clamp(age / ageAtMaxOffset, 0.0f, 1.0f);
			const float rise = (static_cast<float>(atom.random % k_RiseSteps) * k_RiseStep + 1.0f) * riseSpeed;
			const glm::vec3 start(data.a);
			atom.position = {start.x + ramp * maxWiggle * wiggleX, start.y + rise * age, start.z + ramp * maxWiggle * wiggleZ};
			// Faded out between the fade's start and end
			float fade = 0.0f;
			if (age > fadeTimeStart)
			{
				fade = std::clamp((age - fadeTimeStart) / (fadeTimeEnd - fadeTimeStart), 0.0f, 1.0f);
			}
			atom.rgba[3] = static_cast<uint8_t>(std::round(data.b.x - data.b.x * fade));
			const float pulse = std::sin(id * k_PulsePhase + (id * k_PulsePace + 1.0f) * age * scaleFrequency);
			atom.ruleScale = (pulse * 0.5f + 1.0f) * (scaleMax - scaleMin) + scaleMin;
			if (age > fadeTimeEnd + k_GoneAfterFading)
			{
				collection.atoms.erase(collection.atoms.begin() + static_cast<std::ptrdiff_t>(i));
				continue;
			}
			++i;
		}
		return true;
	}

private:
	std::string creator;
	float riseSpeed;
	float maxWiggle;
	float ageAtMaxOffset;
	float noiseFrequency;
	float scaleMax;
	float scaleMin;
	float scaleFrequency;
	float fadeTimeStart;
	float fadeTimeEnd;
	int alphaMin;
	int alphaMax;
	int valueAlphaMin;
	int valueAlphaMax;
	ParticleSound sound;
};
} // namespace

void openblack::particles::RegisterBeliefRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_BeliefSprite", ParticleClassRegistry::Make<BeliefSpriteRule>);
}
