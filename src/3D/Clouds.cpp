/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Clouds.h"

#include <cmath>

#include <algorithm>
#include <array>

using namespace openblack;

namespace
{
constexpr float k_Speed = 70.0f;
constexpr float k_FadeStart = 6000.0f;
constexpr float k_FadeRate = 0.1275f;
/// The wind blows three eighths of a turn round
constexpr float k_WindCos = -0.70710678f;
constexpr float k_WindSin = 0.70710678f;
constexpr float k_Middle = 1280.0f;

/// Each byte of a towards b by f of 256, rounding down
uint32_t LerpBytes(uint32_t a, uint32_t b, int f)
{
	uint32_t result = 0;
	for (const uint32_t shift : {24u, 16u, 8u, 0u})
	{
		const auto ca = static_cast<int>((a >> shift) & 0xFFu);
		const auto cb = static_cast<int>((b >> shift) & 0xFFu);
		const auto c = ca + static_cast<int>(std::floor(static_cast<float>((cb - ca) * f) / 256.0f));
		result |= (static_cast<uint32_t>(c) & 0xFFu) << shift;
	}
	return result;
}
} // namespace

std::vector<clouds::Layout> clouds::MakeLayout(const Random& random)
{
	std::vector<Layout> layout;
	layout.reserve(k_Count);
	for (int i = 0; i < k_Count; ++i)
	{
		// In this order, as the game draws its numbers
		Layout cloud {};
		cloud.track.x = random(-k_TrackHalfLength, k_TrackHalfLength);
		cloud.track.y = random(300.0f, 500.0f);
		cloud.track.z = random(-5000.0f, 5000.0f);
		cloud.size = random(13.0f, 50.0f);
		cloud.edgeShrink = random(2.5f, 5.0f);
		cloud.pinned = false;
		layout.push_back(cloud);
	}
	for (const auto [index, end] : {std::pair {0, k_TrackHalfLength}, std::pair {1, -k_TrackHalfLength}})
	{
		layout.at(index) = {.track = {end, 500.0f, 0.0f}, .size = 300.0f, .edgeShrink = 20.0f, .pinned = true};
	}
	return layout;
}

glm::vec3 clouds::Move(const glm::vec3& track, bool pinned, float milliseconds)
{
	if (pinned)
	{
		return track;
	}
	auto moved = track;
	moved.x += k_Speed * milliseconds * 0.001f;
	if (moved.x > k_TrackHalfLength)
	{
		// Back by whole lengths of the track, truncating
		const float fromStart = moved.x + k_TrackHalfLength;
		moved.x = fromStart - (static_cast<float>(static_cast<int>(fromStart * 6.25e-5f)) * 16000.0f) - k_TrackHalfLength;
	}
	return moved;
}

glm::vec3 clouds::WorldPosition(const glm::vec3& track)
{
	return {(track.x * k_WindCos) - (track.z * k_WindSin) + k_Middle, track.y,
	        (track.x * k_WindSin) + (track.z * k_WindCos) + k_Middle};
}

int clouds::EdgeAlpha(const glm::vec3& track, bool pinned)
{
	if (pinned)
	{
		return 192;
	}
	// Rounded to the nearest
	if (track.x < -k_FadeStart)
	{
		return static_cast<int>(std::lrint((track.x + k_TrackHalfLength) * k_FadeRate));
	}
	if (track.x > k_FadeStart)
	{
		return static_cast<int>(std::lrint((k_TrackHalfLength - track.x) * k_FadeRate));
	}
	return 255;
}

uint32_t clouds::Colour(float skyAlignment, uint32_t fullLight)
{
	constexpr std::array<uint32_t, 3> k_Colours = {0x00FFFFFFu, 0xC8FFFFFFu, 0xFFAAA066u};
	const float towardsEvil = std::clamp(1.0f - skyAlignment, 0.0f, 2.0f);
	const auto index = static_cast<size_t>(towardsEvil);
	const auto fraction = static_cast<int>((towardsEvil - static_cast<float>(index)) * 256.0f);
	const uint32_t colour = LerpBytes(k_Colours.at(index), k_Colours.at(std::min<size_t>(index + 1, 2)), fraction);
	uint32_t result = colour & 0xFF000000u;
	for (const uint32_t shift : {16u, 8u, 0u})
	{
		// In the light, then drawn towards 35 by 70 of 256 of the way, rounding down
		auto c = static_cast<int>((((colour >> shift) & 0xFFu) * ((fullLight >> shift) & 0xFFu)) >> 8);
		c += static_cast<int>(std::floor(static_cast<float>(8960 - (70 * c)) / 256.0f));
		result |= (static_cast<uint32_t>(c) & 0xFFu) << shift;
	}
	return result;
}
