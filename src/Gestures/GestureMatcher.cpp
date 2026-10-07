/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GestureMatcher.h"

#include <cmath>

#include <algorithm>

#include <GestureFile.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::gesture;

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;

/// A sum of turns brought back within a half turn either way
float Wrap(float angle)
{
	if (std::abs(angle) <= k_Pi)
	{
		return angle;
	}
	return angle < 0.0f ? angle + (2.0f * k_Pi) : angle - (2.0f * k_Pi);
}

/// Whether the path sets off from a key point the way the template does; the mirror image of a direction is the one
/// mirrored left to right
bool SetsOffAlike(const gestures::GestureTemplate& gestureTemplate, const KeyPoint& key, bool mirrored)
{
	if (!gestureTemplate.ChecksDirection())
	{
		return true;
	}
	const auto first = gestureTemplate.Points().front().direction;
	if (mirrored)
	{
		return first != 0 ? ((8u - first) & 0xFFu) == key.direction : key.direction == 0;
	}
	return first == key.direction;
}

bool BoxFits(const gestures::GestureTemplate& gestureTemplate, std::span<const KeyPoint> keys, size_t first, size_t last,
             float screenAspect)
{
	if (!gestureTemplate.ChecksAspectRatio())
	{
		return true;
	}
	ScreenBox box {.min = keys[first].screen, .max = keys[first].screen};
	for (size_t i = first + 1; i <= last && i < keys.size(); ++i)
	{
		box.min = glm::min(box.min, keys[i].screen);
		box.max = glm::max(box.max, keys[i].screen);
	}
	return AspectFits(AspectOf(box, screenAspect), gestureTemplate.aspectRatio);
}
} // namespace

float gesture::AspectOf(const ScreenBox& box, float screenAspect)
{
	const auto size = box.Size();
	return size.x / std::max(screenAspect * size.y, 1.0f);
}

bool gesture::AspectFits(float pathAspect, float templateAspect)
{
	if (pathAspect < k_TallAspect)
	{
		return templateAspect < k_TallAspect;
	}
	if (pathAspect > k_WideAspect)
	{
		return templateAspect > k_TallAspect;
	}
	return templateAspect >= k_TallAspect && templateAspect <= k_WideAspect;
}

std::optional<Match> gesture::MatchTemplate(const gestures::GestureTemplate& gestureTemplate, std::span<const KeyPoint> keys,
                                            bool mirrored, float screenAspect)
{
	const auto points = gestureTemplate.Points();
	if (points.empty() || keys.size() < 3)
	{
		return std::nullopt;
	}
	// The path's turns are compared the other way round when it may be the template's mirror image; the running sum of
	// a mirrored path is not brought back within a half turn, as in the game
	const float sign = mirrored ? -1.0f : 1.0f;
	const auto keep = [mirrored](float angle) { return mirrored ? angle : Wrap(angle); };
	const auto lastKey = keys.size() - 1;
	const auto lastPoint = points.size() - 1;
	for (size_t start = 1; start < lastKey; ++start)
	{
		if (!SetsOffAlike(gestureTemplate, keys[start - 1], mirrored))
		{
			continue;
		}
		size_t key = start;
		size_t point = 1;
		float difference = 0.0f;
		float largest = 0.0f;
		bool fits = true;
		while (point < lastPoint)
		{
			float pathTurn = 0.0f;
			if (key < lastKey)
			{
				pathTurn = sign * keys[key].turn;
				++key;
			}
			const float templateTurn = points[point].turn;
			++point;
			difference = keep(templateTurn - pathTurn + difference);
			// A small turn on the path may be passed over, if that brings the sums closer
			if (key < lastKey)
			{
				const auto next = sign * keys[key].turn;
				if (std::abs(next) < k_SmallTurn || std::abs(pathTurn) < k_SmallTurn)
				{
					const auto skipped = keep(difference - next);
					if (std::abs(skipped) < std::abs(difference))
					{
						difference = skipped;
						++key;
					}
				}
			}
			// And so may one in the template
			if (point < lastPoint)
			{
				const auto next = points[point].turn;
				if (std::abs(next) < k_SmallTurn || std::abs(templateTurn) < k_SmallTurn)
				{
					const auto skipped = keep(next + difference);
					if (std::abs(skipped) < std::abs(difference))
					{
						difference = skipped;
						++point;
					}
				}
			}
			largest = std::max(largest, std::abs(difference));
			if (std::abs(difference) > k_TurnTolerance)
			{
				fits = false;
				break;
			}
		}
		if (!fits)
		{
			continue;
		}
		const auto first = mirrored ? start : start - 1;
		const auto last = mirrored ? key - 1 : key;
		if (BoxFits(gestureTemplate, keys, first, last, screenAspect))
		{
			return Match {.mirrored = mirrored, .firstKey = first, .lastKey = last, .largestDifference = largest};
		}
	}
	return std::nullopt;
}

std::optional<Match> gesture::Recognise(std::span<const gestures::GestureTemplate> templates, GestureType gesture,
                                        std::span<const KeyPoint> keys, float screenAspect)
{
	for (size_t i = 0; i < templates.size(); ++i)
	{
		const auto& entry = templates[i];
		if (entry.Gesture() != static_cast<uint8_t>(gesture))
		{
			continue;
		}
		auto match = MatchTemplate(entry, keys, false, screenAspect);
		if (!match.has_value() && entry.AllowsMirror())
		{
			match = MatchTemplate(entry, keys, true, screenAspect);
		}
		if (match.has_value())
		{
			match->templateIndex = i;
			return match;
		}
	}
	return std::nullopt;
}

std::optional<Match> gesture::ClosestMatch(std::span<const gestures::GestureTemplate> templates, std::span<const KeyPoint> keys,
                                           float screenAspect)
{
	std::optional<Match> closest;
	for (size_t i = 0; i < templates.size(); ++i)
	{
		for (const bool mirrored : {false, true})
		{
			if (mirrored && !templates[i].AllowsMirror())
			{
				continue;
			}
			auto match = MatchTemplate(templates[i], keys, mirrored, screenAspect);
			if (match.has_value() && (!closest.has_value() || match->largestDifference < closest->largestDifference))
			{
				match->templateIndex = i;
				closest = match;
			}
		}
	}
	return closest;
}

ScreenBox gesture::GestureBox(const GestureRecorder& recorder, const Match& match)
{
	const auto range = recorder.PointsOfKeys(match.firstKey, match.lastKey);
	return recorder.BoxOf(range.first, range.last);
}

glm::vec2 gesture::CircleEdge(const ScreenBox& box)
{
	const auto size = box.Size();
	const auto centre = box.Centre();
	return {centre.x + (std::max(size.x, size.y) * 0.5f), centre.y};
}

float gesture::CircleRadius(glm::vec3 centre, glm::vec3 edge, glm::vec3 cameraRight)
{
	// Measured along the camera's right across the land, so the camera's tilt doesn't stretch it
	auto across = glm::vec3(cameraRight.x, 0.0f, cameraRight.z);
	const auto length = glm::length(across);
	if (length <= 0.0f)
	{
		return glm::length(edge - centre) * k_CircleRadiusScale;
	}
	across /= length;
	return std::abs(glm::dot(edge - centre, across)) * k_CircleRadiusScale;
}
