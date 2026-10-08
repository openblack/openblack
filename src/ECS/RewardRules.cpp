/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "RewardRules.h"

#include <algorithm>

using namespace openblack::ecs;
using namespace openblack::ecs::reward;

float reward::FallHeight(float seconds)
{
	return k_FallFrom - k_FallSpeed * seconds;
}

float reward::FallYaw(float seconds)
{
	return k_FallTurnsPerSecond * seconds;
}

std::array<DustSprite, k_DustSprites> reward::MakeDust(const std::function<float(float, float)>& random)
{
	std::array<DustSprite, k_DustSprites> dust {};
	for (auto& sprite : dust)
	{
		// Across, then up, then along
		sprite.offset.z = random(-k_DustSpread, k_DustSpread);
		sprite.offset.y = random(0.0f, k_DustRise);
		sprite.offset.x = random(-k_DustSpread, k_DustSpread);
		sprite.frame = static_cast<uint32_t>(static_cast<int32_t>(random(0.0f, k_DustFrames))) & k_DustFrameMask;
		const float shade = random(k_DustLeastShade, 1.0f);
		for (size_t i = 0; i < sprite.rgb.size(); ++i)
		{
			sprite.rgb.at(i) = static_cast<uint8_t>(static_cast<int32_t>(k_DustColour.at(i) * shade));
		}
	}
	return dust;
}

DustLook reward::LookOfDust(float millisecondsLeft)
{
	// From fully seen and small to gone and wide as its time runs out
	const float left = millisecondsLeft * k_DustFadePerMillisecond;
	return {
	    .alpha = static_cast<uint8_t>(std::clamp(static_cast<int32_t>(255.0f * left), 0, 255)),
	    .size = std::max((1.0f - left) * k_DustGrowth + k_DustStartSize, k_DustSmallest),
	};
}
