/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FieldCrop.h"

#include <algorithm>

#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace openblack::field_crop
{

namespace
{
/// The young crop's green, its colour half grown, and its own colour once ripe
constexpr uint32_t k_Young = 0xAAD443;
constexpr uint32_t k_HalfGrown = 0x799119;
constexpr uint32_t k_Ripe = 0xFFFFFF;
/// The crop shows once it is a quarter of the way to ripening
constexpr float k_ShowsFrom = 0.25f;
/// How deep the crop sinks at most, of its height
constexpr float k_DeepestSink = -0.8f;
/// The time the crop's height eases in over, in seconds
constexpr float k_SettleSeconds = 1.0f;
} // namespace

bool IsSown(const Crop& crop, const Type& type)
{
	return static_cast<float>(crop.timesSown) >= type.timesToSow;
}

bool IsRipe(const Crop& crop, const Type& type)
{
	return crop.age >= type.ageRipe;
}

bool RemoveFood(Crop& crop, const Type& type, float ratioBeforeRipe, float amount)
{
	// Nothing is taken from a field with no food, or not yet sown
	if (crop.food == 0.0f || static_cast<float>(crop.timesSown) < type.timesToSow)
	{
		return false;
	}
	const auto truncated = [](float value) { return static_cast<int64_t>(value); };
	auto removed = truncated(amount);
	if (crop.age < type.ageRipe)
	{
		removed = truncated(amount * ratioBeforeRipe + static_cast<float>(removed));
	}
	if (static_cast<float>(removed) < crop.food)
	{
		crop.food -= static_cast<float>(removed);
		return false;
	}
	crop.food = 0.0f;
	if (crop.age >= type.ageRipe)
	{
		crop.timesSown = 0;
		crop.age = 0.0f;
	}
	return true;
}

float ClampAlignment(float sum)
{
	return std::clamp(sum, -1.0f, 1.0f);
}

void Grow(Crop& crop, const Type& type, float alignment, bool raining)
{
	if (!IsSown(crop, type) || crop.age > type.ageRipe)
	{
		return;
	}
	const bool ripening = crop.age >= type.ageGrowth;
	const float effect = raining ? (ripening ? type.rainWhenRipening : type.rainWhenGrowing)
	                             : (ripening ? type.sunWhenRipening : type.sunWhenGrowing);
	const auto rate = static_cast<float>((alignment * 0.5f + 1.0f) * 2.0f) * effect;
	crop.age += rate;
	crop.food += rate * type.totalFood / type.ageRipe;
}

std::optional<Look> LookOf(const Crop& crop, const Type& type, uint32_t leastFood)
{
	if (type.ageGrowth * k_ShowsFrom > crop.age || static_cast<float>(leastFood) > crop.food)
	{
		return std::nullopt;
	}
	const float empty = 1.0f - crop.food / type.totalFood;
	Look look {.tint = k_Ripe, .height = -empty, .sways = false};
	if (crop.age < type.ageGrowth)
	{
		// A young crop is greener the emptier it is
		look.tint = Blend(k_HalfGrown, k_Young, static_cast<int32_t>(empty * 255.0f));
	}
	else if (crop.age < type.ageRipe)
	{
		// It turns to its own colour as it ripens
		const float ripening = (crop.age - type.ageGrowth) / (type.ageRipe - type.ageGrowth);
		look.tint = Blend(k_HalfGrown, k_Ripe, static_cast<int32_t>(ripening * 255.0f));
	}
	else
	{
		look.sways = true;
	}
	return look;
}

uint32_t Blend(uint32_t from, uint32_t to, int32_t t)
{
	uint32_t blended = 0;
	for (uint32_t shift = 0; shift < 24; shift += 8)
	{
		const auto a = static_cast<int32_t>((from >> shift) & 0xFFu);
		const auto b = static_cast<int32_t>((to >> shift) & 0xFFu);
		const auto channel = (a * (255 - t) + b * t) / 255;
		blended |= (static_cast<uint32_t>(channel) & 0xFFu) << shift;
	}
	return blended;
}

Settle Advance(Settle settle, float target, float seconds)
{
	if (seconds >= k_SettleSeconds)
	{
		return {.position = target, .speed = 0.0f};
	}
	// The quartic that reaches the target at rest, with no acceleration left, after the settling time: its
	// acceleration, jerk and the jerk's change, from where it is and how fast it moves now
	constexpr float k_T = k_SettleSeconds;
	constexpr float k_T2 = k_T * k_T / 2.0f;
	constexpr float k_T3 = k_T * k_T * k_T / 6.0f;
	constexpr float k_T4 = k_T * k_T * k_T * k_T / 24.0f;
	// Columns: the jerk's change, the jerk and the acceleration; rows: where it ends, its speed and its acceleration
	const glm::mat3 constraints {glm::vec3(k_T4, k_T3, k_T2), glm::vec3(k_T3, k_T2, k_T), glm::vec3(k_T2, k_T, 1.0f)};
	const glm::vec3 wanted {target - settle.position - k_T * settle.speed, -settle.speed, 0.0f};
	const auto coefficients = glm::inverse(constraints) * wanted;
	const float snap = coefficients.x;
	const float jerk = coefficients.y;
	const float acceleration = coefficients.z;
	const float t = seconds;
	const float t2 = t * t / 2.0f;
	const float t3 = t * t2 / 3.0f;
	const float t4 = t2 * t2 / 6.0f;
	return {
	    .position = settle.position + settle.speed * t + acceleration * t2 + jerk * t3 + snap * t4,
	    .speed = settle.speed + acceleration * t + jerk * t2 + snap * t3,
	};
}

float Sink(float height)
{
	return std::max(height, k_DeepestSink);
}

} // namespace openblack::field_crop
