/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraDrag.h"

#include <cmath>
#include <cstdlib>

#include <glm/gtc/constants.hpp>

namespace openblack::camera_drag
{

namespace
{
// The hints by the cursor's place
constexpr float k_SideEdge = 0.45f;
constexpr float k_BottomEdge = 0.43f;
constexpr float k_VeryBottomEdge = 0.49f;
constexpr float k_TopEdgeWithoutLand = -0.4f;
constexpr float k_TopEdge = -0.49f;

/// The mouse must move this far, in shares of the screen, before a drag is decided
constexpr float k_DecideDistance = 0.02f;
/// Dragging from the side towards the middle pans the land if it is quick, within this long
constexpr uint32_t k_QuickPanMs = 300;
/// A tilting drag that goes down this soon after the press over land pans instead
constexpr uint32_t k_QuickTiltMs = 80;
/// Directions are only made unit length when they are longer than this
constexpr float k_MinDirectionLength = 0.1f;

/// The ring the cursor is held on dragging round the edge, as a share of the screen's half width and height, and the
/// band of squared distances from the middle that are left alone
constexpr float k_RingRadius = 0.9f;
constexpr float k_RingBandMin = 0.89f;
constexpr float k_RingBandMax = 0.91f;

/// A share of the field of view across the drag down the whole screen tilts by
constexpr float k_PitchPerScreen = 2.33333f;

/// Both buttons turn the camera once the mouse has moved this share of the screen's width across
constexpr float k_TwoButtonTurnShare = 0.025f;

glm::vec2 Direction(glm::vec2 vector)
{
	const auto length = std::sqrt(vector.x * vector.x + vector.y * vector.y);
	return length > k_MinDirectionLength ? vector / length : vector;
}
} // namespace

glm::vec2 NormalisedCursor(glm::ivec2 cursor, glm::ivec2 screenSize)
{
	const auto width = static_cast<float>(screenSize.x);
	const auto height = static_cast<float>(screenSize.y);
	return {static_cast<float>(cursor.x) / width - 0.5f, static_cast<float>(cursor.y - (screenSize.y >> 1)) / height};
}

uint32_t IdleTricons(glm::vec2 normalisedCursor, bool landUnderCursor)
{
	uint32_t tricons = tricon::k_Idle;
	if (std::abs(normalisedCursor.x) > k_SideEdge)
	{
		tricons |= tricon::k_Rotate;
	}
	if (normalisedCursor.y > k_BottomEdge)
	{
		tricons |= tricon::k_Rotate;
	}
	if (normalisedCursor.y > k_VeryBottomEdge)
	{
		tricons |= tricon::k_Pitch;
	}
	if ((!landUnderCursor && normalisedCursor.y < k_TopEdgeWithoutLand) || normalisedCursor.y < k_TopEdge)
	{
		tricons |= tricon::k_Rotate | tricon::k_Pitch | tricon::k_Top;
	}
	if ((tricons & tricon::k_Rotate) != 0)
	{
		tricons |= tricon::k_Turning;
	}
	return tricons;
}

void DragClassifier::Start(uint32_t tricons, glm::vec2 normalisedCursor, uint32_t milliseconds)
{
	_tricons = tricons & ~tricon::k_Idle;
	if ((_tricons & tricon::k_Rotate) != 0)
	{
		_tricons |= tricon::k_Turning;
	}
	_pressedAt = normalisedCursor;
	_pressedMs = milliseconds;
	_moved = glm::vec2(0.0f);
	_mode.reset();
	// Pressed away from the edges, the land is gripped at once
	if ((_tricons & (tricon::k_Rotate | tricon::k_Pitch)) == 0)
	{
		_mode = DragMode::Pan;
		_tricons = 0;
	}
}

void DragClassifier::Move(glm::ivec2 delta, glm::ivec2 screenSize, uint32_t milliseconds, bool landUnderCursor,
                          uint32_t features)
{
	// Across by the height and down by the width, as the game adds them up
	_moved += glm::vec2(static_cast<float>(delta.x) / static_cast<float>(screenSize.y),
	                    static_cast<float>(delta.y) / static_cast<float>(screenSize.x));
	if (!_mode.has_value() && k_DecideDistance * k_DecideDistance < _moved.x * _moved.x + _moved.y * _moved.y)
	{
		Decide(milliseconds, landUnderCursor, features);
	}
}

void DragClassifier::Decide(uint32_t milliseconds, bool landUnderCursor, uint32_t features)
{
	const auto elapsed = milliseconds - _pressedMs;
	// Towards the middle of the screen from where it was pressed, and square to that
	const auto towardsMiddle = Direction(-_pressedAt);
	const auto across = Direction({_pressedAt.y, -_pressedAt.x});
	const auto alongMoved = towardsMiddle.x * _moved.x + towardsMiddle.y * _moved.y;
	const auto acrossMoved = across.x * _moved.x + across.y * _moved.y;

	const auto pitch = (_tricons & tricon::k_Top) != 0 ? DragMode::PitchFromTop : DragMode::Pitch;
	const bool canRotate = (features & feature::k_Rotate) != 0;
	auto mode = DragMode::Pan;
	if ((_tricons & (tricon::k_Rotate | tricon::k_Pitch)) == (tricon::k_Rotate | tricon::k_Pitch))
	{
		if ((features & (feature::k_Pitch | feature::k_Rotate)) == (feature::k_Pitch | feature::k_Rotate))
		{
			mode = std::abs(_moved.x) <= std::abs(_moved.y) ? pitch : DragMode::EdgeRotate;
		}
		else
		{
			mode = canRotate ? DragMode::EdgeRotate : pitch;
		}
	}
	else if ((_tricons & tricon::k_Rotate) != 0)
	{
		if ((features & (feature::k_Rotate | feature::k_Strafe)) == (feature::k_Rotate | feature::k_Strafe))
		{
			// A quick drag towards the middle pans; any other way, or a slow one, turns
			const bool turns = std::abs(alongMoved) < std::abs(acrossMoved) || alongMoved < 0.0f || elapsed > k_QuickPanMs;
			mode = turns ? DragMode::EdgeRotate : DragMode::Pan;
		}
		else
		{
			mode = canRotate ? DragMode::EdgeRotate : DragMode::Pan;
		}
	}
	else if ((_tricons & tricon::k_Pitch) != 0)
	{
		mode = pitch;
	}
	if ((mode == DragMode::Pitch || mode == DragMode::PitchFromTop) && _moved.y > 0.0f && elapsed < k_QuickTiltMs &&
	    landUnderCursor)
	{
		mode = DragMode::Pan;
	}
	_mode = mode;
	switch (mode)
	{
	case DragMode::Pan:
		_tricons = 0;
		break;
	case DragMode::EdgeRotate:
		_tricons = tricon::k_Rotate;
		break;
	case DragMode::Pitch:
	case DragMode::PitchFromTop:
		_tricons = tricon::k_Pitch;
		break;
	}
}

RingStep EdgeRotate(glm::ivec2 cursor, glm::ivec2 previousOnRing, glm::ivec2 screenSize)
{
	const auto width = static_cast<float>(screenSize.x);
	const auto height = static_cast<float>(screenSize.y);
	const auto x = (static_cast<float>(cursor.x) - width * 0.5f) / (width * 0.5f);
	const auto y = (static_cast<float>(cursor.y) - height * 0.5f) / (height * 0.5f);
	auto onRing = cursor;
	const auto squared = x * x + y * y;
	if (squared < k_RingBandMin || k_RingBandMax < squared)
	{
		const auto scale = squared != 0.0f ? k_RingRadius / std::sqrt(squared) : 0.0f;
		onRing = {static_cast<int>(((scale * x + 1.0f) * width + 1.0f) * 0.5f),
		          static_cast<int>((scale * y * height + height + 1.0f) * 0.5f)};
	}
	const auto halfWidth = screenSize.x / 2;
	const auto halfHeight = screenSize.y >> 1;
	const auto before =
	    std::atan2(static_cast<float>(previousOnRing.x - halfWidth), static_cast<float>(previousOnRing.y - halfHeight));
	const auto after = std::atan2(static_cast<float>(onRing.x - halfWidth), static_cast<float>(onRing.y - halfHeight));
	auto angle = after - before;
	if (angle > glm::pi<float>())
	{
		angle -= glm::two_pi<float>();
	}
	if (angle < -glm::pi<float>())
	{
		angle += glm::two_pi<float>();
	}
	return {.cursor = onRing, .angle = angle};
}

float PitchStep(int deltaY, int screenHeight, float horizontalFieldOfView)
{
	return static_cast<float>(deltaY) / static_cast<float>(screenHeight) * k_PitchPerScreen * horizontalFieldOfView;
}

int TwoButtonTurn::Update(bool held, int deltaX, int cursorX, int screenWidth)
{
	if (!held)
	{
		_turning = false;
		_pressedX = cursorX;
		return 0;
	}
	const auto threshold = static_cast<float>(screenWidth) * k_TwoButtonTurnShare;
	if (threshold < static_cast<float>(std::abs(deltaX)) || threshold < static_cast<float>(std::abs(cursorX - _pressedX)))
	{
		_turning = true;
	}
	return _turning ? deltaX : 0;
}

} // namespace openblack::camera_drag
