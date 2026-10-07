/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "BeamMaths.h"

#include <array>

#include "KeyPointSpline.h"

using namespace openblack::particles;

float maths::BeamBulge(float t)
{
	const float f = (t + t) - 1.0f;
	return 1.0f - (f * f);
}

std::vector<glm::vec3> maths::BeamKeyPoints(glm::vec3 start, glm::vec3 end, int count, const BeamWiggle& wiggle, float age,
                                            int beam, const std::function<float(float)>& noise,
                                            const std::function<float(glm::vec2)>& landHeight)
{
	std::vector<glm::vec3> keys;
	if (count < 2)
	{
		return keys;
	}
	keys.reserve(static_cast<size_t>(count));
	const float step = 1.0f / (static_cast<float>(count) - 1.0f);
	const auto offset = static_cast<float>(beam);
	for (int i = 0; i < count; ++i)
	{
		const float t = static_cast<float>(i) * step;
		auto point = start + ((end - start) * t);
		if (i != 0 && i != count - 1)
		{
			const float bulge = BeamBulge(t);
			const float along = t * wiggle.frequency;
			point.x += noise((age * wiggle.speed) + along + offset) * wiggle.amount * bulge;
			point.z += noise((age * wiggle.speed * k_BeamDepthDriftShare) + along + offset) * wiggle.amount * bulge;
			point.y += (noise((age * wiggle.speed * k_BeamHeightDriftShare) + along + offset) + 1.0f) * wiggle.amount * bulge *
			           k_BeamHeightShare;
			const float ground = landHeight({point.x, point.z});
			if (point.y - ground < wiggle.minHeight)
			{
				point.y = ground + wiggle.minHeight;
			}
		}
		keys.push_back(point);
	}
	return keys;
}

std::vector<maths::BeamJoint> maths::BeamJoints(std::span<const glm::vec3> keys, size_t count, float minScale, float maxScale)
{
	std::vector<BeamJoint> joints;
	if (keys.size() < 2 || count == 0)
	{
		return joints;
	}
	// One curve for each axis, through the key points at their shares of the way
	const float keyStep = 1.0f / (static_cast<float>(keys.size()) - 1.0f);
	std::array<std::vector<float>, 3> pairs;
	for (size_t i = 0; i < keys.size(); ++i)
	{
		const float t = static_cast<float>(i) * keyStep;
		for (size_t axis = 0; axis < pairs.size(); ++axis)
		{
			pairs.at(axis).push_back(t);
			pairs.at(axis).push_back(keys[i][static_cast<glm::length_t>(axis)]);
		}
	}
	const std::array<KeyPointSpline, 3> curves {KeyPointSpline(pairs[0]), KeyPointSpline(pairs[1]), KeyPointSpline(pairs[2])};
	// A lone joint sits at the start
	const float step = count > 1 ? 1.0f / (static_cast<float>(count) - 1.0f) : 0.0f;
	const float range = maxScale - minScale;
	joints.reserve(count);
	// Where two key points share a place on the curve it gives nothing, and the joint stays where the last one was
	glm::vec3 position = keys.back();
	for (size_t j = 0; j < count; ++j)
	{
		const float u = static_cast<float>(j) * step;
		for (size_t axis = 0; axis < curves.size(); ++axis)
		{
			const auto component = static_cast<glm::length_t>(axis);
			position[component] = curves.at(axis).Evaluate(u, position[component]);
		}
		joints.push_back({.position = position, .scale = (BeamBulge(u) * range) + minScale});
	}
	return joints;
}
