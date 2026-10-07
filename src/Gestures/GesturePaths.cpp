/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GesturePaths.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <GestureFile.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include "Gestures/GestureMatcher.h"

using namespace openblack;
using namespace openblack::gesture;

std::vector<glm::vec2> gesture::Trace(std::span<const glm::vec2> points, float step)
{
	std::vector<glm::vec2> samples;
	for (size_t i = 0; i + 1 < points.size(); ++i)
	{
		const auto from = points[i];
		const auto to = points[i + 1];
		const auto steps = std::max(1, static_cast<int>(glm::length(to - from) / step));
		for (int s = 0; s < steps; ++s)
		{
			// Whole pixels, as a cursor's are
			samples.push_back(glm::round(from + ((to - from) * (static_cast<float>(s) / static_cast<float>(steps)))));
		}
	}
	if (!points.empty())
	{
		samples.push_back(glm::round(points.back()));
	}
	return samples;
}

std::vector<glm::vec2> gesture::TraceTemplate(const gestures::GestureTemplate& gestureTemplate, glm::vec2 middle, float size,
                                              float step)
{
	std::vector<glm::vec2> corners;
	for (const auto& point : gestureTemplate.Points())
	{
		corners.push_back(middle + (glm::vec2(point.x - 0.5f, point.z - 0.5f) * size));
	}
	return Trace(corners, step);
}

std::optional<std::vector<glm::vec2>> gesture::TraceGesture(std::span<const gestures::GestureTemplate> templates,
                                                            GestureType gesture, glm::vec2 middle, float size,
                                                            float screenAspect)
{
	for (const auto& entry : templates)
	{
		if (entry.Gesture() != static_cast<uint8_t>(gesture))
		{
			continue;
		}
		auto path = TraceTemplate(entry, middle, size);
		GestureRecorder recorder;
		for (const auto& point : path)
		{
			recorder.AddPoint(point, glm::vec3(0.0f));
		}
		if (Recognise(templates, gesture, recorder.KeyPoints(), screenAspect).has_value())
		{
			return path;
		}
	}
	return std::nullopt;
}

std::vector<glm::vec2> gesture::TraceCircle(glm::vec2 middle, float radius, bool clockwise, float step)
{
	// A little over a full turn, as hands draw them
	constexpr int k_Corners = 40;
	constexpr float k_Turns = 1.1f;
	std::vector<glm::vec2> corners;
	for (int i = 0; i <= k_Corners; ++i)
	{
		const auto angle = (static_cast<float>(i) / k_Corners) * k_Turns * 2.0f * std::numbers::pi_v<float>;
		const auto across = std::sin(angle) * radius * (clockwise ? 1.0f : -1.0f);
		corners.push_back(middle + glm::vec2(across, -std::cos(angle) * radius));
	}
	return Trace(corners, step);
}

std::vector<glm::vec2> gesture::TraceScribble(glm::vec2 middle, float width, int strokes, float step)
{
	std::vector<glm::vec2> corners;
	for (int i = 0; i <= strokes; ++i)
	{
		const auto side = (i % 2 == 0) ? -0.5f : 0.5f;
		corners.push_back(middle + glm::vec2(side * width, static_cast<float>(i) * 2.0f));
	}
	return Trace(corners, step);
}

gestures::GestureTemplate gesture::MakeTemplate(GestureType gesture, std::span<const KeyPoint> keys, bool checkDirection,
                                                bool allowMirror, bool checkAspect, float screenAspect)
{
	gestures::GestureTemplate made;
	if (keys.empty())
	{
		return made;
	}
	ScreenBox box {.min = keys.front().screen, .max = keys.front().screen};
	for (const auto& key : keys)
	{
		box.min = glm::min(box.min, key.screen);
		box.max = glm::max(box.max, key.screen);
	}
	const auto extent = std::max({box.max.x - box.min.x, box.max.y - box.min.y, 1.0f});
	const auto count = std::min(keys.size(), gestures::GestureTemplate::k_MaxPoints);
	for (size_t i = 0; i < count; ++i)
	{
		const auto at = (keys[i].screen - box.min) / extent;
		made.points.at(i) = {.x = at.x, .y = 0.0f, .z = at.y, .turn = keys[i].turn, .direction = keys[i].direction};
	}
	// The ends turn nowhere
	made.points.at(0).turn = 0.0f;
	made.points.at(count - 1).turn = 0.0f;
	made.pointCountField = static_cast<uint32_t>(count);
	made.gestureField = static_cast<uint32_t>(gesture);
	made.positionModeField = 2;
	made.checkDirection = checkDirection ? 1 : 0;
	made.allowMirror = allowMirror ? 1 : 0;
	made.checkAspectRatio = checkAspect ? 1 : 0;
	made.aspectRatio = AspectOf(box, screenAspect);
	return made;
}
