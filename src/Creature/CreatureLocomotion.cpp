/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureLocomotion.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::creature_locomotion;

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;
constexpr float k_HalfPi = k_Pi / 2.0f;
/// Walking speed is this many units a second for each unit of the size factor, running this many
constexpr float k_WalkPerSize = 8.0f;
constexpr float k_RunPerSize = 20.0f;
/// The size factor is size times this plus the offset
constexpr float k_SizeSlope = 0.875f;
constexpr float k_SizeOffset = 0.25f;
/// A creature going slower than this isn't moving its feet
constexpr float k_Still = 1e-4f;
/// The arrival ring never reaches past the destination by less than this, and is never thinner than this
constexpr float k_RingInside = 0.05f;
constexpr float k_RingThickness = 0.001f;
/// A walk is given this long and this much longer per unit of distance before it is given up on
constexpr float k_TimeLimitBaseMs = 5000.0f;
constexpr float k_TimeLimitPerUnitMs = 2.0f;

float Loop(float time, float duration)
{
	if (duration <= 0.0f)
	{
		return 0.0f;
	}
	const auto looped = std::fmod(time, duration);
	return looped < 0.0f ? looped + duration : looped;
}
} // namespace

Speeds creature_locomotion::SpeedsFor(float size)
{
	const auto factor = (k_SizeSlope * std::clamp(size, k_MinSize, k_MaxSize)) + k_SizeOffset;
	return {.walk = k_WalkPerSize * factor, .run = k_RunPerSize * factor};
}

float creature_locomotion::TargetSpeed(float fraction, float runSpeed)
{
	return std::clamp(fraction, 0.0f, 1.0f) * runSpeed * k_TopSpeedMargin;
}

float creature_locomotion::RequiredFraction(float wanted, float slowFraction, float exhaustion)
{
	return exhaustion >= k_ExhaustedAt ? slowFraction : wanted;
}

float creature_locomotion::LeashFraction(float walkFraction, float runFraction, float pull)
{
	return walkFraction + (pull * ((2.0f * runFraction) - walkFraction));
}

float creature_locomotion::StopCap(float remaining)
{
	return std::max(std::sqrt(2.0f * k_Acceleration * std::max(remaining, 0.0f)), k_MinSpeed);
}

float creature_locomotion::CornerCap(float remaining, float cornerAllowance)
{
	return std::max(
	    std::sqrt((k_Acceleration * std::max(cornerAllowance, 0.0f)) + (2.0f * k_Acceleration * std::max(remaining, 0.0f))),
	    k_MinSpeed);
}

float creature_locomotion::SlopeFactor(float altitudeAhead, float altitude, float radius)
{
	if (radius <= 0.0f)
	{
		return 1.0f;
	}
	return std::clamp(1.0f - (k_SlopeSlowing * (altitudeAhead - altitude) / radius), k_MinSlopeFactor, k_MaxSlopeFactor);
}

float creature_locomotion::Accelerate(float speed, float target, float seconds)
{
	if (speed < target)
	{
		return std::min(target, speed + (k_Acceleration * seconds));
	}
	return speed;
}

float creature_locomotion::WrapAngle(float angle)
{
	angle = std::fmod(angle + k_Pi, 2.0f * k_Pi);
	if (angle < 0.0f)
	{
		angle += 2.0f * k_Pi;
	}
	return angle - k_Pi;
}

float creature_locomotion::HeadingOf(glm::vec2 direction)
{
	return std::atan2(-direction.x, -direction.y);
}

glm::vec2 creature_locomotion::DirectionOf(float heading)
{
	return {-std::sin(heading), -std::cos(heading)};
}

float creature_locomotion::LerpHeading(float heading, float target, float timeMs, float durationMs)
{
	if (durationMs <= 0.0f || timeMs >= durationMs)
	{
		return target;
	}
	return WrapAngle(heading + (WrapAngle(target - heading) * std::max(timeMs, 0.0f) / durationMs));
}

Gait creature_locomotion::BlendGait(float speed, const Speeds& speeds, float distance, float scale, const Cycle& stand,
                                    const Cycle& walk, const Cycle& run, float walkTimeMs, float breathPhase)
{
	const auto standTime = stand.durationMs * breathPhase;
	if (speed <= k_Still || speeds.walk <= 0.0f)
	{
		return {.slots = {{{.animation = animations::k_Stand, .timeMs = standTime, .weight = 1.0f},
		                   {.animation = animations::k_Walk, .timeMs = walkTimeMs, .weight = 0.0f}}},
		        .walkTimeMs = walkTimeMs,
		        .walkAdvanceMs = 0.0f};
	}

	if (speed < speeds.walk)
	{
		const auto walkWeight = speed / speeds.walk;
		// The blend's stride shrinks with the walk's weight, so the feet don't slide
		const auto stride = walkWeight * walk.stride * scale;
		const auto advance = stride > 0.0f ? walk.durationMs * distance / stride : 0.0f;
		const auto time = Loop(walkTimeMs + advance, walk.durationMs);
		return {.slots = {{{.animation = animations::k_Stand, .timeMs = standTime, .weight = 1.0f - walkWeight},
		                   {.animation = animations::k_Walk, .timeMs = time, .weight = walkWeight}}},
		        .walkTimeMs = time,
		        .walkAdvanceMs = advance};
	}

	const auto range = speeds.run - speeds.walk;
	const auto runWeight = range > 0.0f ? std::clamp((speed - speeds.walk) / range, 0.0f, 1.0f) : 1.0f;
	const auto walkWeight = 1.0f - runWeight;
	const auto stride = ((walkWeight * walk.stride) + (runWeight * run.stride)) * scale;
	const auto advance = stride > 0.0f ? walk.durationMs * distance / stride : 0.0f;
	const auto time = Loop(walkTimeMs + advance, walk.durationMs);
	// The run keeps in step with the walk
	const auto runTime = walk.durationMs > 0.0f ? run.durationMs * time / walk.durationMs : 0.0f;
	return {.slots = {{{.animation = animations::k_Run, .timeMs = runTime, .weight = runWeight},
	                   {.animation = animations::k_Walk, .timeMs = time, .weight = walkWeight}}},
	        .walkTimeMs = time,
	        .walkAdvanceMs = advance};
}

Side creature_locomotion::SideOf(float angle)
{
	return angle > 0.0f ? Side::Right : Side::Left;
}

Pair creature_locomotion::PairFor(size_t first, float angle)
{
	const auto turn = std::min(std::abs(angle), k_Pi);
	if (turn < k_HalfPi)
	{
		return {.from = first, .to = first + 1, .weight = turn / k_HalfPi};
	}
	return {.from = first + 1, .to = first + 2, .weight = std::clamp((turn - k_HalfPi) / k_HalfPi, 0.0f, 1.0f)};
}

Start creature_locomotion::ChooseStart(float angle, float distanceInMesh, const StartOptions& options)
{
	const auto side = SideOf(angle);
	if (options.moving && options.stepStrides.has_value() &&
	    std::ranges::all_of(*options.stepStrides, [distanceInMesh](float stride) { return distanceInMesh > stride; }))
	{
		const auto first = side == Side::Right ? animations::k_RightStep : animations::k_LeftStep;
		return {.kind = Start::Kind::Step, .side = side, .animations = PairFor(first, angle)};
	}
	if (std::abs(angle) < k_WalkStraightAngle)
	{
		return {.kind = Start::Kind::Walk, .side = side, .animations = std::nullopt};
	}
	if (options.hasSpins)
	{
		const auto first = side == Side::Right ? animations::k_RightSpin : animations::k_LeftSpin;
		return {.kind = Start::Kind::Turn, .side = side, .animations = PairFor(first, angle)};
	}
	return {.kind = Start::Kind::Turn, .side = side, .animations = std::nullopt};
}

Ring creature_locomotion::MoveRing(float distance, float minDistance, float maxDistance)
{
	const auto min = std::max(std::min(minDistance, distance - k_RingInside), 0.0f);
	return {.min = min, .max = std::max(maxDistance, min + k_RingThickness)};
}

Ring creature_locomotion::ObjectRing(float creatureRadius, float objectRadius, float extra)
{
	const auto min = creatureRadius + objectRadius;
	return {.min = min, .max = min + std::max(extra, k_RingThickness)};
}

bool creature_locomotion::Arrived(float distance, const Ring& ring)
{
	return distance <= ring.max;
}

float creature_locomotion::TimeLimitMs(float distance)
{
	return (k_TimeLimitPerUnitMs * distance) + k_TimeLimitBaseMs;
}

glm::vec2 creature_locomotion::RunAwayPoint(glm::vec2 creature, glm::vec2 threat, float distance)
{
	auto away = creature - threat;
	// Fleeing something right on top of it, it runs away along x
	away = glm::length(away) > 0.0f ? glm::normalize(away) : glm::vec2(1.0f, 0.0f);
	return creature + (away * distance);
}
