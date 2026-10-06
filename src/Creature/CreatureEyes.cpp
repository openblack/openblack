/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureEyes.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::creature_eyes;

namespace
{
/// An eye is a tenth of the creature's size, by its species' eye size
constexpr float k_EyeSizePerSize = 0.1f;
/// Sizes above this make the eyes and head no smaller for their size
constexpr float k_MaxSizeEffect = 2.0f;
/// How much bigger an evil (smaller for good) creature's eyes are
constexpr float k_EvilGoodEyeEffect = 0.05f;
/// The darting eyes of a stoned creature are a tenth bigger and take five times as long to turn
constexpr float k_StonedEyeSize = 1.1f;
constexpr float k_StonedLookSlowdown = 5.0f;
/// How long the eyes take to turn, by how open they are
constexpr float k_LookSeconds = 0.3f;
constexpr float k_LookOpennessOffset = 2.0f;
/// The least the back of the eye looks into the head along its normal
constexpr float k_MinInwardLook = 0.1f;
/// How much a calm lid opens further for how open the eyes are, in radians
constexpr float k_LidOpennessAngle = 0.3f;

glm::vec3 NormalisedOr(const glm::vec3& v, const glm::vec3& fallback)
{
	const auto length = glm::length(v);
	return length > 0.0f ? v / length : fallback;
}
} // namespace

SurfacePoint creature_eyes::PointOnTriangle(const glm::vec3& first, const glm::vec3& second, const glm::vec3& third, float u,
                                            float v)
{
	const auto towardsSecond = second - first;
	const auto towardsThird = third - first;
	return {
	    .position = first + (towardsSecond * u) + (towardsThird * v),
	    .normal = glm::cross(towardsThird, towardsSecond),
	};
}

float creature_eyes::EyeSize(float size, float speciesEyeScale, float evilGood, Mode mode)
{
	const auto limited = std::clamp(size, 0.0f, k_MaxSizeEffect);
	const auto eyes = (1.0f + (k_EvilGoodEyeEffect * evilGood)) * (1.1f - (0.1f * limited));
	const auto head = 1.8f - (0.5f * limited);
	const auto eyeSize = head * eyes * speciesEyeScale * size * k_EyeSizePerSize;
	return mode == Mode::Stoned ? eyeSize * k_StonedEyeSize : eyeSize;
}

float creature_eyes::LookSeconds(float openness, Mode mode)
{
	const auto seconds = k_LookSeconds / (openness + k_LookOpennessOffset);
	return mode == Mode::Stoned ? seconds * k_StonedLookSlowdown : seconds;
}

glm::vec3 creature_eyes::ClampLook(const glm::vec3& away, const glm::vec3& inwardNormal)
{
	auto look = away;
	const auto along = glm::dot(inwardNormal, look);
	if (along < k_MinInwardLook)
	{
		look += (k_MinInwardLook - along) * inwardNormal;
	}
	return NormalisedOr(look, inwardNormal);
}

Frame creature_eyes::EyeballFrame(const glm::vec3& centre, const glm::vec3& away)
{
	const auto z = NormalisedOr(away, glm::vec3(0.0f, 0.0f, 1.0f));
	const auto x = NormalisedOr(glm::vec3(-z.z, 0.0f, z.x), glm::vec3(1.0f, 0.0f, 0.0f));
	return {.x = x, .y = glm::cross(z, x), .z = z, .origin = centre};
}

Frame creature_eyes::EyelidFrame(const glm::vec3& centre, const glm::vec3& inwardNormal, const glm::vec3& anchor, bool rightEye)
{
	const auto z = NormalisedOr(-inwardNormal, glm::vec3(0.0f, 0.0f, 1.0f));
	const auto towards = rightEye ? centre - anchor : anchor - centre;
	const auto y = NormalisedOr(glm::cross(towards, z), glm::vec3(0.0f, 1.0f, 0.0f));
	return {.x = glm::cross(y, z), .y = y, .z = z, .origin = centre};
}

float creature_eyes::LidPitch(const Frame& lid, const Frame& eyeball)
{
	return glm::dot(lid.y, eyeball.z);
}

float creature_eyes::EyelidAngle(Mode mode, const LidAngles& angles, float openness, float lidPitch, const Blink& blink)
{
	constexpr auto k_Pi = std::numbers::pi_v<float>;
	switch (mode)
	{
	case Mode::Wide:
	case Mode::Stoned:
		return angles.open * k_Pi;
	case Mode::Closed:
		return angles.closed * k_Pi;
	case Mode::Calm:
	case Mode::Ahead:
		break;
	}
	const auto calm = (angles.calm * k_Pi) - std::asin(std::clamp(lidPitch, -1.0f, 1.0f)) + (k_LidOpennessAngle * openness);
	const auto closed = angles.closed * k_Pi;
	const auto halfBlink = static_cast<float>(k_BlinkMs);
	switch (blink.state)
	{
	case Blink::State::Closing:
		return calm + ((closed - calm) * (halfBlink - static_cast<float>(blink.timerMs)) / halfBlink);
	case Blink::State::Opening:
		return calm + ((closed - calm) * static_cast<float>(blink.timerMs) / halfBlink);
	case Blink::State::Open:
		break;
	}
	return calm;
}

Frame creature_eyes::Turned(const Frame& frame, float angle)
{
	const auto c = std::cos(angle);
	const auto s = std::sin(angle);
	return {
	    .x = frame.x,
	    .y = (c * frame.y) - (s * frame.z),
	    .z = (c * frame.z) + (s * frame.y),
	    .origin = frame.origin,
	};
}

glm::mat4 creature_eyes::ToMatrix(const Frame& frame, float size)
{
	// The game's rows are the axes the mesh's x, y and z go along
	return {
	    glm::vec4(frame.x * size, 0.0f),
	    glm::vec4(frame.y * size, 0.0f),
	    glm::vec4(frame.z * size, 0.0f),
	    glm::vec4(frame.origin, 1.0f),
	};
}
