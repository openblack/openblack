/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandMotion.h"

#include <cmath>

#include <algorithm>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

using namespace openblack::magic;

glm::vec3 openblack::magic::FilterHandVelocity(glm::vec3 smoothed, glm::vec3 raw, float seconds)
{
	if (!(seconds > 0.0f))
	{
		return smoothed;
	}
	// Eased so that the share is reached over the time, whatever the step
	const float rate = -std::log(1.0f - k_HandVelocityShare) / k_HandVelocitySeconds;
	return smoothed + (raw - smoothed) * (1.0f - std::exp(-rate * seconds));
}

void openblack::magic::StepHandSpin(HandSpin& spin, glm::vec3 step, glm::vec3 smoothedVelocity, float seconds)
{
	if (!(seconds > 0.0f))
	{
		return;
	}
	const auto last = spin.lastStep;
	spin.lastStep = step;
	const float perSecond = 1.0f / seconds;
	// How far it moved sideways to the way it moved the frame before, across the land
	glm::vec3 side {last.z, 0.0f, -last.x};
	float sideways = 0.0f;
	if (side.x != 0.0f || side.z != 0.0f)
	{
		sideways = glm::dot(glm::normalize(side), step);
	}
	const float rate = -10.0f * std::log(1.0f - k_HandLateralShare);
	spin.lateral += (sideways * perSecond * perSecond - spin.lateral) * (1.0f - std::exp(-seconds * rate));
	const float speed = glm::length(smoothedVelocity);
	spin.spin = speed > k_HandStill ? -spin.lateral / speed : 0.0f;
}

CubicSpline::CubicSpline(std::span<const glm::vec2> points, std::optional<float> startSlope, std::optional<float> endSlope)
    : _points(points.begin(), points.end())
    , _bends(points.size(), 0.0f)
{
	const size_t n = _points.size();
	if (n < 2)
	{
		return;
	}
	// The second derivatives at the points, from the tridiagonal system that makes the slopes meet, decomposed going up
	// and solved coming back
	std::vector<float> u(n, 0.0f);
	if (startSlope.has_value())
	{
		const float h = _points[1].x - _points[0].x;
		_bends[0] = -0.5f;
		u[0] = (3.0f / h) * ((_points[1].y - _points[0].y) / h - *startSlope);
	}
	for (size_t i = 1; i + 1 < n; ++i)
	{
		const auto& before = _points[i - 1];
		const auto& at = _points[i];
		const auto& after = _points[i + 1];
		const float sig = (at.x - before.x) / (after.x - before.x);
		const float p = sig * _bends[i - 1] + 2.0f;
		_bends[i] = (sig - 1.0f) / p;
		const float d = (after.y - at.y) / (after.x - at.x) - (at.y - before.y) / (at.x - before.x);
		u[i] = (6.0f * d / (after.x - before.x) - sig * u[i - 1]) / p;
	}
	float qn = 0.0f;
	float un = 0.0f;
	if (endSlope.has_value())
	{
		const float h = _points[n - 1].x - _points[n - 2].x;
		qn = 0.5f;
		un = (3.0f / h) * (*endSlope - (_points[n - 1].y - _points[n - 2].y) / h);
	}
	_bends[n - 1] = (un - qn * u[n - 2]) / (qn * _bends[n - 2] + 1.0f);
	for (size_t k = n - 1; k-- > 0;)
	{
		_bends[k] = _bends[k] * _bends[k + 1] + u[k];
	}
}

float CubicSpline::operator()(float x) const
{
	if (_points.empty())
	{
		return 0.0f;
	}
	if (x <= _points.front().x)
	{
		return _points.front().y;
	}
	if (x >= _points.back().x)
	{
		return _points.back().y;
	}
	const auto next = std::ranges::upper_bound(_points, x, {}, &glm::vec2::x);
	const auto i = static_cast<size_t>(std::distance(_points.begin(), next)) - 1;
	const auto& a = _points[i];
	const auto& b = _points[i + 1];
	const float h = b.x - a.x;
	const float toB = b.x - x;
	const float fromA = x - a.x;
	return _bends[i] * toB * toB * toB / (6.0f * h) + _bends[i + 1] * fromA * fromA * fromA / (6.0f * h) +
	       (a.y / h - _bends[i] * h / 6.0f) * toB + (b.y / h - _bends[i + 1] * h / 6.0f) * fromA;
}

void openblack::magic::StartPour(PourState& pour, const PourSettings& settings)
{
	// The pose it is in carries on from where it was
	pour.active = true;
	pour.settings = settings;
	pour.progress = 0.0f;
	pour.pinned.reset();
}

void openblack::magic::StartPour(PourState& pour, const PourSettings& settings, const glm::vec3& hand)
{
	StartPour(pour, settings);
	if (settings.clampHand)
	{
		pour.pinned = hand;
	}
}

void openblack::magic::StopPour(PourState& pour)
{
	// What was drawn last turn is kept, so that the hand eases back to rest over the next
	pour.active = false;
	pour.pinned.reset();
	pour.current = {};
}

void openblack::magic::StepPour(PourState& pour, float seconds)
{
	pour.previous = pour.current;
	if (!pour.active)
	{
		return;
	}
	pour.progress += pour.settings.totalTime > 0.0f ? seconds / pour.settings.totalTime : 1.0f;
	if (pour.progress > 1.0f)
	{
		if (!pour.settings.loops)
		{
			StopPour(pour);
			return;
		}
		pour.progress = 0.0f;
	}
	static const CubicSpline k_Curve(k_PourKeyPoints, 0.0f, 0.0f);
	const float shape = k_Curve(pour.progress);
	pour.current = {.raise = shape * pour.settings.heightToRaise, .tilt = shape * pour.settings.angleToRaise};
}

PourPose openblack::magic::PourPoseAt(const PourState& pour, float fraction)
{
	const float t = std::clamp(fraction, 0.0f, 1.0f);
	return {.raise = glm::mix(pour.previous.raise, pour.current.raise, t),
	        .tilt = glm::mix(pour.previous.tilt, pour.current.tilt, t),
	        .pinned = pour.active ? pour.pinned : std::nullopt};
}
