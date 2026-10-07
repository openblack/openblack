/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FireGraphic.h"

#include <cmath>

#include <algorithm>

#include "FireModel.h"

using namespace openblack;
using namespace openblack::fire;
using namespace openblack::fire::graphic;

namespace
{
/// Things this big or bigger burn with more flames
constexpr float k_LargeObject = 3.0f;
constexpr int k_SmallFlames = 2;
constexpr int k_LargeFlames = 7;
/// The flames' size as a share of the height
constexpr float k_TreeFlameScale = 0.2f;
constexpr float k_JointedFlameScale = 0.3f;
constexpr float k_FlameScale = 0.5f;
/// A puff grows from this share of its size to this one over its life, and drifts towards this share of the wind
/// at this rate
constexpr float k_PuffStartScale = 0.2f;
constexpr float k_PuffEndScale = 2.6f;
constexpr float k_PuffWindShare = 0.5f;
constexpr float k_PuffWindRate = 0.1f;
/// Steam starts this bright and rises this fast; smoke this bright and this fast
constexpr float k_SteamAlpha = 100.0f;
constexpr float k_SteamRise = 1.0f;
constexpr float k_SmokeAlpha = 180.0f;
constexpr float k_SmokeRise = 2.0f;
/// The sprite sheets' animations run at this many frames a second through 32 frames
constexpr float k_CellRate = 25.0f;
constexpr float k_Cells = 32.0f;
/// A burning building's light at its fiercest, and how much it flickers
constexpr float k_LightShare = 0.6f;
constexpr float k_LightFlicker = 0.2f;

/// The puffs age, grow, fade from their first alpha and drift with half the wind as they rise
void UpdatePuffs(std::vector<Sprite>& puffs, float seconds, float startAlpha, float rise, const glm::vec3& wind)
{
	std::erase_if(puffs, [&](Sprite& puff) {
		puff.age += seconds;
		if (puff.age > k_PuffLife)
		{
			return true;
		}
		const float t = puff.age / k_PuffLife;
		puff.scale = ((k_PuffEndScale - k_PuffStartScale) * t + k_PuffStartScale) * puff.baseScale;
		puff.alpha = std::round((0.0f - startAlpha) * t + startAlpha);
		puff.velocity += (wind * k_PuffWindShare - puff.velocity) * k_PuffWindRate * seconds;
		puff.velocity.y += rise * seconds;
		puff.position += puff.velocity * (seconds * puff.baseScale);
		return false;
	});
}

Sprite NewPuff(const Graphic& graphic, const glm::vec3& world, const Sampler& sampler)
{
	Sprite puff;
	puff.position = world;
	puff.baseScale = graphic.localScale;
	const float x = sampler.random(2.0f) - 1.0f;
	const float z = sampler.random(2.0f) - 1.0f;
	puff.velocity = glm::vec3(x, 0.0f, z);
	puff.scale = k_PuffStartScale * puff.baseScale;
	return puff;
}

void UpdateFlames(Graphic& graphic, const Inputs& inputs, float seconds, const Sampler& sampler)
{
	if (inputs.fraction != 0.0f)
	{
		const auto most = static_cast<float>(graphic.maxFlames);
		graphic.flameAccumulator += most / k_FlameLife * inputs.fraction * seconds;
		// It flares up once when it first gets very hot, and a little as it catches
		if ((inputs.flags & k_VeryHot) != 0 && !graphic.flared)
		{
			graphic.flared = true;
			graphic.flameAccumulator += most;
		}
		if ((inputs.flags & k_JustIgnited) != 0)
		{
			graphic.flameAccumulator += static_cast<float>((graphic.maxFlames + 1) / 2);
		}
	}
	while (static_cast<float>(graphic.flameCount) < graphic.flameAccumulator)
	{
		++graphic.flameCount;
		if (const auto point = sampler.localPoint ? sampler.localPoint() : std::nullopt)
		{
			graphic.flames.push_back({.position = *point});
		}
	}
	const float fadeOut = 1.0f / (k_FlameLife - k_FlameFadeIn);
	std::erase_if(graphic.flames, [&](Sprite& flame) {
		flame.age += seconds;
		if (flame.age > k_FlameLife)
		{
			return true;
		}
		flame.scale = (inputs.fraction + 1.0f) * 0.5f * graphic.localScale;
		const float share =
		    flame.age < k_FlameFadeIn ? flame.age / k_FlameFadeIn : 1.0f - (flame.age - k_FlameFadeIn) * fadeOut;
		flame.alpha = std::clamp(share, 0.0f, 1.0f) * k_FlameAlpha;
		return false;
	});
}

bool UpdateSteam(Graphic& graphic, const Inputs& inputs, float seconds, const Sampler& sampler)
{
	bool started = false;
	if (!graphic.steamStart.has_value())
	{
		if ((inputs.flags & k_Cooling) != 0 && inputs.temperature > k_SteamTemperature &&
		    inputs.temperature > graphic.steamTemperature)
		{
			graphic.steamStart = inputs.turn;
			graphic.steamCount = 0;
			graphic.steamAccumulator = 0.0f;
			graphic.steamTemperature = inputs.temperature;
			started = true;
		}
	}
	else if (inputs.turn > *graphic.steamStart + k_PuffTurns)
	{
		graphic.steamStart.reset();
	}
	else
	{
		graphic.steamAccumulator += k_PuffsPerSecond * seconds;
		while (static_cast<float>(graphic.steamCount) < graphic.steamAccumulator)
		{
			++graphic.steamCount;
			if (const auto point = sampler.localPoint ? sampler.localPoint() : std::nullopt)
			{
				graphic.steam.push_back(NewPuff(graphic, sampler.toWorld(*point), sampler));
			}
		}
	}
	UpdatePuffs(graphic.steam, seconds, k_SteamAlpha, k_SteamRise, inputs.wind);
	return started;
}

void UpdateSmoke(Graphic& graphic, const Inputs& inputs, float seconds, const Sampler& sampler)
{
	if (!graphic.smokeStart.has_value())
	{
		if ((inputs.flags & k_JustExtinguished) != 0)
		{
			graphic.smokeStart = inputs.turn;
			graphic.smokeCount = 0;
			graphic.smokeAccumulator = 0.0f;
			graphic.smokePoint = sampler.localPoint ? sampler.localPoint().value_or(glm::vec3(0.0f)) : glm::vec3(0.0f);
		}
	}
	else if (inputs.turn > *graphic.smokeStart + k_PuffTurns)
	{
		graphic.smokeStart.reset();
	}
	else
	{
		graphic.smokeAccumulator += k_PuffsPerSecond * seconds;
		while (static_cast<float>(graphic.smokeCount) < graphic.smokeAccumulator)
		{
			++graphic.smokeCount;
			graphic.smoke.push_back(NewPuff(graphic, sampler.toWorld(graphic.smokePoint), sampler));
		}
	}
	UpdatePuffs(graphic.smoke, seconds, k_SmokeAlpha, k_SmokeRise, inputs.wind);
}
} // namespace

int graphic::MaxFlames(bool tree, float radius, float height)
{
	if (tree)
	{
		return k_SmallFlames;
	}
	return std::max(radius, height) >= k_LargeObject ? k_LargeFlames : k_SmallFlames;
}

float graphic::LocalFlameScale(FlameShape shape, float height)
{
	switch (shape)
	{
	case FlameShape::Tree:
		return k_TreeFlameScale * height;
	case FlameShape::Jointed:
		return k_JointedFlameScale * height;
	case FlameShape::Plain:
		break;
	}
	return k_FlameScale * height;
}

bool graphic::Update(Graphic& graphic, const Inputs& inputs, float seconds, const Sampler& sampler)
{
	// It moves on only as it is drawn: after a long while out of sight, with its fire changed, it catches up by a
	// flame's life at once
	if (graphic.drawnTurn.has_value() && inputs.turn - *graphic.drawnTurn > k_CatchUpTurns &&
	    inputs.fraction != graphic.drawnFraction)
	{
		seconds += k_FlameLife;
	}
	graphic.drawnTurn = inputs.turn;
	graphic.drawnFraction = inputs.fraction;
	if (graphic.kinds.flames)
	{
		UpdateFlames(graphic, inputs, seconds, sampler);
	}
	bool sizzles = false;
	if (graphic.kinds.steam)
	{
		sizzles = UpdateSteam(graphic, inputs, seconds, sampler);
	}
	if (graphic.kinds.smoke)
	{
		UpdateSmoke(graphic, inputs, seconds, sampler);
	}
	return sizzles;
}

int graphic::FlameCell(float age)
{
	return static_cast<int>(std::fmod(-k_CellRate * age, k_Cells) + k_Cells);
}

int graphic::PuffCell(float age)
{
	return static_cast<int>(std::fmod(k_CellRate * age, k_Cells));
}

uint8_t graphic::CharredGrey(float charring)
{
	const int k = static_cast<int>(charring * 255.0f) & 0xFF;
	return static_cast<uint8_t>(255 - (k * 175 + 255) / 256);
}

graphic::TreeLook graphic::BurningTree(float temperature, float combustion, float life)
{
	constexpr float k_FullHeat = 1.5f;
	constexpr int k_Darkest = 50;
	constexpr float k_LightLife = 0.9f;
	constexpr float k_GreyPerLife = 2550.0f;
	constexpr float k_LeastReference = 230.0f;
	constexpr float k_MostReference = 254.0f;
	constexpr float k_ShrinkLife = 0.2f;
	constexpr float k_ShrinkRate = 5.0f;
	// How hot it is, of 255: full at one and a half times its combustion temperature
	const float fullHeat = combustion * k_FullHeat;
	const int heat =
	    temperature > fullHeat ? 255 : static_cast<int>((temperature - combustion) * 255.0f / (fullHeat - combustion));
	int grey = k_Darkest;
	if (life > k_LightLife)
	{
		grey = static_cast<int>(std::max(255.0f - (1.0f - life) * k_GreyPerLife, static_cast<float>(k_Darkest)));
	}
	const float reference =
	    std::min((255.0f - k_LeastReference) * static_cast<float>(heat) * (1.0f / 255.0f) + k_LeastReference, k_MostReference);
	const float scale = life < k_ShrinkLife ? 1.0f - (k_ShrinkLife - life) * k_ShrinkRate : 1.0f;
	return {.grey = grey, .alphaReference = reference, .scale = scale};
}

uint32_t graphic::GlowColour(float temperature, float charring, float flicker)
{
	constexpr float k_GlowPerDegree = 0.001f;
	constexpr float k_GlowFlicker = 0.2f;
	const float heat = (flicker * k_GlowFlicker + 1.0f) * (k_GlowPerDegree * temperature);
	const float glow = heat > 0.0f ? (heat < 1.0f ? heat : 1.0f) : 0.0f;
	const auto value = static_cast<uint32_t>(static_cast<int32_t>((1.0f - charring) * glow * 255.0f)) & 0xFFu;
	// Red and a third as much green, of 64ths: red-orange
	return (((value * 45u) >> 6u) << 16u) | (((value * 15u) >> 6u) << 8u);
}

float graphic::LightStrength(float fraction, float charring, float flicker)
{
	return std::clamp(k_LightShare * fraction * (1.0f + k_LightFlicker * flicker) * (1.0f - charring), 0.0f, 1.0f);
}
