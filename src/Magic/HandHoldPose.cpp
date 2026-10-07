/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandHoldPose.h"

#include <cmath>

#include <algorithm>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::magic;
using namespace openblack::magic::hand_hold;

namespace
{
/// A vector turned by an angle about an axis, right-handedly; an axis of no length only shrinks it by the angle's cosine
glm::vec3 Turned(glm::vec3 v, glm::vec3 axis, float angle)
{
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	return v * c + glm::cross(axis, v) * s + axis * (glm::dot(axis, v) * (1.0f - c));
}

/// A vector of unit length, or of none when it has none
glm::vec3 UnitOrNone(glm::vec3 v)
{
	const float length = glm::length(v);
	return length > 0.0f ? v / length : glm::vec3(0.0f);
}
} // namespace

HoldType hand_hold::HoldTypeOf(bool ready, HoldType recorded)
{
	return ready ? recorded : HoldType::Magic;
}

std::optional<HandAnimation::Cycle> hand_hold::HoldCycle(HoldType hold)
{
	switch (hold)
	{
	case HoldType::Above:
		return HandAnimation::Cycle::HoldAbove;
	case HoldType::Magic:
		return HandAnimation::Cycle::Wiggle;
	case HoldType::Grain:
		return HandAnimation::Cycle::Horn;
	case HoldType::Tree:
	case HoldType::Side:
	case HoldType::Villager:
		return HandAnimation::Cycle::HoldSide;
	case HoldType::None:
	case HoldType::Fingers:
		break;
	}
	return std::nullopt;
}

uint32_t hand_hold::HoldTimeMs(HoldType hold, uint32_t durationMs, float reach, float handSize)
{
	const float handReach = k_StandardHandHeight * handSize;
	switch (hold)
	{
	case HoldType::Above:
	{
		const float share = std::min(reach / (handReach * k_AboveReachShare), 1.0f);
		return static_cast<uint32_t>(static_cast<float>(durationMs) * 0.5f * (1.0f - share));
	}
	case HoldType::Magic:
		return durationMs >> 1;
	case HoldType::Grain:
	case HoldType::Tree:
	case HoldType::Side:
	case HoldType::Villager:
	{
		const float share = std::min(reach / handReach, 1.0f);
		return static_cast<uint32_t>(static_cast<float>(durationMs >> 1) * share);
	}
	case HoldType::None:
	case HoldType::Fingers:
		break;
	}
	return 0;
}

float hand_hold::SeedHang(HoldType hold, float lowering, float height)
{
	switch (hold)
	{
	case HoldType::Above:
	case HoldType::Grain:
	case HoldType::Tree:
	case HoldType::Side:
	case HoldType::Villager:
		return lowering * height;
	case HoldType::None:
	case HoldType::Magic:
	case HoldType::Fingers:
		break;
	}
	return 0.0f;
}

float hand_hold::HoldLift(HoldType hold, float hang, float handSize)
{
	switch (hold)
	{
	case HoldType::Above:
		return k_AboveLift;
	case HoldType::Magic:
		return k_StandardHandHeight * handSize;
	case HoldType::Grain:
	case HoldType::Tree:
	case HoldType::Side:
	case HoldType::Villager:
		return std::max(hang, k_MinimumSideLift);
	case HoldType::None:
	case HoldType::Fingers:
		break;
	}
	return 0.0f;
}

glm::vec2 hand_hold::CursorSway(glm::vec2 cursorLag)
{
	return glm::clamp(cursorLag, -k_SwayLag, k_SwayLag) * (k_MaxSway / k_SwayLag);
}

glm::vec3 hand_hold::HeldUp(glm::vec3 camera, glm::vec3 hand, float roll, float pitch)
{
	const auto toCamera = UnitOrNone(camera - hand);
	const auto across = UnitOrNone(glm::vec3(-toCamera.z, 0.0f, toCamera.x));
	const auto rolled = Turned(glm::vec3(0.0f, 1.0f, 0.0f), toCamera, -roll);
	return Turned(rolled, across, -pitch);
}

glm::mat3 hand_hold::HeldBasis(const glm::mat3& levelTurn, glm::vec3 up)
{
	// The way the hand faces, kept level
	const auto facing = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), levelTurn[0]);
	auto side = glm::cross(facing, up);
	if (side != glm::vec3(0.0f))
	{
		side = glm::normalize(side);
	}
	return {side, up, glm::cross(side, up)};
}

glm::mat3 hand_hold::SeedTurn(const glm::mat3& basis, float yRotate, bool rightHanded)
{
	auto x = basis[0];
	auto z = basis[2];
	if (rightHanded)
	{
		x = -x;
		z = -z;
	}
	const float c = std::cos(yRotate);
	const float s = std::sin(yRotate);
	const auto turnedX = x * c + z * s;
	const auto turnedZ = z * c - x * s;
	return {UnitOrNone(turnedX), UnitOrNone(basis[1]), UnitOrNone(turnedZ)};
}

void HandFade::Start(glm::vec3 position, const glm::mat3& rotation)
{
	_fromPosition = position;
	_fromRotation = rotation;
	_time = 0.0f;
}

void HandFade::Step(float seconds, glm::vec3& position, glm::mat3& rotation)
{
	if (!_time.has_value())
	{
		return;
	}
	*_time += seconds;
	if (*_time >= k_FadeSeconds)
	{
		_time.reset();
		return;
	}
	const float t = *_time / k_FadeSeconds;
	position = glm::mix(_fromPosition, position, t);
	for (glm::length_t column = 0; column < 3; ++column)
	{
		rotation[column] = glm::mix(_fromRotation[column], rotation[column], t);
	}
}
