/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ResourcePiles.h"

#include <cmath>

#include <algorithm>

using namespace openblack;
using namespace openblack::magic;

namespace
{
/// The sample numbers of the thuds in the in-game bank: six small and two big for food, six of each for wood
constexpr uint32_t k_FoodSmallThud = 0x4D;
constexpr uint32_t k_FoodBigThud = 0x4B;
constexpr uint32_t k_WoodSmallThud = 0x5C;
constexpr uint32_t k_WoodBigThud = 0x56;
constexpr uint32_t k_SixThuds = 6;
} // namespace

float piles::ProportionRaised(ResourceType type, uint32_t amount, uint32_t fullAmount)
{
	const float share = static_cast<float>(amount) / static_cast<float>(fullAmount);
	if (type == ResourceType::Wood)
	{
		const float shown = share > 0.0f ? (1.0f - k_LeastShown) * share + k_LeastShown : share;
		return std::clamp(shown, 0.0f, 1.0f);
	}
	float shown = 0.0f;
	if (share > 0.0f)
	{
		shown = (1.0f - k_LeastShown) * std::min(share, 1.0f) + k_LeastShown;
	}
	const float raised = 1.0f - (1.0f - shown) * (1.0f - shown);
	return std::max(raised, 0.0f);
}

float piles::SunkOffset(float proportion, float height)
{
	return (proportion - 1.0f) * height;
}

piles::Rise piles::SunkRise(float height)
{
	Rise rise;
	rise.start = -height;
	rise.target = -height;
	rise.offset = -height;
	return rise;
}

void piles::RiseTo(Rise& rise, float target)
{
	// The quartic from where it is, at its speed, that ends a second later at the target with no speed or acceleration
	const float change = target - rise.offset;
	const float speed = rise.currentSpeed;
	rise.start = rise.offset;
	rise.speed = speed;
	rise.target = target;
	rise.time = 0.0f;
	rise.duration = 1.0f;
	rise.acceleration = 12.0f * change - 6.0f * speed;
	rise.jerk = 18.0f * speed - 48.0f * change;
	rise.snap = 72.0f * change - 24.0f * speed;
}

void piles::StepRise(Rise& rise, float seconds)
{
	rise.time += seconds;
	if (rise.time < rise.duration)
	{
		const float t = rise.time;
		const float half = t * t * 0.5f;
		const float sixth = t * half * (1.0f / 3.0f);
		rise.currentSpeed = t * rise.acceleration + sixth * rise.snap + half * rise.jerk + rise.speed;
		rise.offset = rise.speed * t + half * rise.acceleration + sixth * rise.jerk + half * half * (1.0f / 6.0f) * rise.snap +
		              rise.start;
		return;
	}
	rise.offset = rise.target;
	rise.currentSpeed = 0.0f;
	rise.time = rise.duration;
}

bool piles::Shown(const Rise& rise, float height)
{
	return -height < rise.offset;
}

float piles::GrainFlow(float offset, float height)
{
	const float up = std::clamp(offset / height + 1.0f, 0.0f, 1.0f);
	return (1.0f - up) * k_GrainFlowSpan;
}

bool piles::GrainFlows(PotInfo type)
{
	return type == PotInfo::MagicFood || type == PotInfo::StoragePitFoodPile;
}

uint32_t piles::PileSoundSample(ResourceType type, uint32_t amount, uint32_t tick)
{
	if (amount < k_BigThud)
	{
		return type == ResourceType::Food ? k_FoodSmallThud + tick % k_SixThuds : k_WoodSmallThud + tick % k_SixThuds;
	}
	return type == ResourceType::Food ? k_FoodBigThud + (tick & 1u) : k_WoodBigThud + tick % k_SixThuds;
}

bool piles::Thuds(PotInfo type)
{
	return type != PotInfo::HandWood && type != PotInfo::HandFood;
}

glm::ivec2 piles::CellOf(glm::vec2 xz)
{
	return {static_cast<int>(std::floor(xz.x / k_CellSize)), static_cast<int>(std::floor(xz.y / k_CellSize))};
}

float piles::RadiusOnGround(glm::vec2 halfExtent, float scale)
{
	return std::max(halfExtent.x, halfExtent.y) * scale;
}

bool piles::IsCapped(PotInfo nextPotForResource)
{
	return nextPotForResource != PotInfo::_COUNT;
}

uint32_t piles::AmountTaken(uint32_t holds, uint32_t given, uint32_t maximum, bool capped)
{
	if (capped && holds + given > maximum)
	{
		return maximum > holds ? maximum - holds : 0;
	}
	return given;
}
