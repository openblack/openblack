/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AnimalAnimation.h"

#include <cmath>

#include <algorithm>

using namespace openblack;

namespace
{
/// A speed state times the milliseconds over this is metres: 655.36 speed state is a metre a second
constexpr double k_SpeedStateMillisecondsPerMetre = 655350.0;
/// The milliseconds of a turn, and the most of them the clock counts into one
constexpr uint32_t k_TurnMilliseconds = 100;
constexpr uint32_t k_LastMillisecond = 99;
} // namespace

uint32_t animals::AdvanceClip(const ClipTiming& clip, uint32_t place, int32_t played)
{
	const auto next = static_cast<int64_t>(place) + played;
	const auto playTime = static_cast<int64_t>(clip.playTime);
	if (next < playTime)
	{
		return static_cast<uint32_t>(std::max<int64_t>(next, 0));
	}
	if (!clip.looping)
	{
		return clip.playTime;
	}
	return playTime > 0 ? static_cast<uint32_t>(next % playTime) : 0;
}

int32_t animals::MovingPlay(const ClipTiming& clip, uint16_t speedState, uint32_t frameMilliseconds, float scale)
{
	// A clip without a stride doesn't play by the ground covered
	if (!(clip.stride > 0.0f) || !(scale > 0.0f))
	{
		return 0;
	}
	// The metres are kept to single precision, then the share of the play time is truncated
	const auto metres = static_cast<float>(static_cast<double>(static_cast<float>(uint64_t {speedState} * frameMilliseconds)) /
	                                       (static_cast<double>(scale) * k_SpeedStateMillisecondsPerMetre));
	return static_cast<int32_t>(static_cast<double>(metres) / static_cast<double>(clip.stride) *
	                            static_cast<double>(clip.playTime));
}

animals::KeyframeSpan animals::SpanAt(const ClipTiming& clip, uint32_t place)
{
	const auto count = static_cast<uint32_t>(clip.frameCount);
	if (count == 0 || clip.playTime == 0 || (!clip.looping && count == 1))
	{
		return {};
	}
	// A clip that holds its end reaches its last keyframe at its end
	const uint32_t period = clip.looping ? clip.playTime : (clip.playTime * count) / (count - 1);
	uint32_t from = static_cast<uint32_t>((uint64_t {count} * place) / period);
	if (from >= count)
	{
		from = 0;
		place = 0;
	}
	const uint32_t to = from + 1 == count ? 0 : from + 1;
	const float t =
	    (static_cast<float>(count) / static_cast<float>(period)) * static_cast<float>(place) - static_cast<float>(from);
	return {.from = from, .to = to, .t = t};
}

uint32_t animals::DrawTime(uint32_t turn, float turnFraction)
{
	const auto into = static_cast<uint32_t>(std::clamp(turnFraction, 0.0f, 1.0f) * static_cast<float>(k_TurnMilliseconds));
	return (turn * k_TurnMilliseconds) + std::min(into, k_LastMillisecond);
}
