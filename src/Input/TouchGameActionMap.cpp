/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TouchGameActionMap.h"

#include <algorithm>
#include <span>
#include <string_view>
#include <vector>

#include <SDL3/SDL_stdinc.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <spdlog/spdlog.h>

#include "ECS/Systems/HandSystemInterface.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using openblack::Locator;
using namespace openblack::input;
using namespace std::chrono_literals;

namespace
{
constexpr auto k_TapSlop = 0.03f;
constexpr auto k_DoubleTapDistance = 0.06f;
constexpr auto k_PinchThreshold = 0.05f;
constexpr auto k_TiltThreshold = 0.05f;
constexpr auto k_TwistThreshold = glm::radians(12.0f);

constexpr auto k_LongPressDelay = 450ms;
constexpr auto k_DoubleTapDelay = 300ms;

constexpr auto k_ReferenceShortSide = 1024.0f;
// TODO: Tune on device. Signs follow the desktop mouse gestures and may need flipping.
constexpr auto k_PinchScale = 1.5f;
constexpr auto k_TwistScale = 400.0f;
constexpr auto k_TiltScale = 1.0f;

template <typename T>
T WithAction(T map, T action, bool value)
{
	using U = std::underlying_type_t<T>;
	return static_cast<T>(value ? static_cast<U>(map) | static_cast<U>(action) : static_cast<U>(map) & ~static_cast<U>(action));
}

glm::vec2 GetWindowSize()
{
	return Locator::windowing::has_value() ? static_cast<glm::vec2>(Locator::windowing::value().GetSize())
	                                       : glm::vec2(1.0f, 1.0f);
}

float GetShortSide()
{
	const auto size = GetWindowSize();
	return glm::max(glm::min(size.x, size.y), 1.0f);
}

float WrapAngle(float angle)
{
	return glm::mod(angle + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
}

[[maybe_unused]] std::string_view GestureName(uint8_t gesture)
{
	static constexpr std::array<std::string_view, 10> k_Names = {
	    "None", "Pending", "Drag", "LongPress", "DoubleTap", "TwoFinger", "Pinch", "Twist", "Tilt", "Finished",
	};
	return gesture < k_Names.size() ? k_Names.at(gesture) : "Unknown";
}

template <typename T>
bool HasAction(T map, T action)
{
	using U = std::underlying_type_t<T>;
	return (static_cast<U>(map) & static_cast<U>(action)) != 0;
}
} // namespace

bool TouchGameActionMap::GetBindable(BindableActionMap action) const
{
	return HasAction(_bindableMap, action);
}

bool TouchGameActionMap::GetUnbindable(UnbindableActionMap action) const
{
	return HasAction(_unbindableMap, action);
}

bool TouchGameActionMap::GetBindableChanged(BindableActionMap action) const
{
	return HasAction(
	    static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) ^ static_cast<uint64_t>(_bindableMapPrevious)),
	    action);
}

bool TouchGameActionMap::GetUnbindableChanged(UnbindableActionMap action) const
{
	return HasAction(
	    static_cast<UnbindableActionMap>(static_cast<uint8_t>(_unbindableMap) ^ static_cast<uint8_t>(_unbindableMapPrevious)),
	    action);
}

bool TouchGameActionMap::GetBindableRepeat(BindableActionMap action) const
{
	return HasAction(_bindableMap, action) && HasAction(_bindableMapPrevious, action);
}

bool TouchGameActionMap::GetUnbindableRepeat(UnbindableActionMap action) const
{
	return HasAction(_unbindableMap, action) && HasAction(_unbindableMapPrevious, action);
}

glm::uvec2 TouchGameActionMap::GetMousePosition() const
{
	return static_cast<glm::uvec2>(glm::clamp(_pointer, glm::vec2(0.0f), GetWindowSize() - 1.0f));
}

glm::ivec2 TouchGameActionMap::GetMouseDelta() const
{
	return static_cast<glm::ivec2>(glm::round(_delta));
}

std::array<std::optional<glm::vec3>, 2> TouchGameActionMap::GetHandPositions() const
{
	auto handPositions = Locator::handSystem::value().GetPlayerHandPositions();

	return {{
	    handPositions[static_cast<size_t>(openblack::ecs::systems::HandSystemInterface::Side::Left)],
	    handPositions[static_cast<size_t>(openblack::ecs::systems::HandSystemInterface::Side::Right)],
	}};
}

void TouchGameActionMap::Frame()
{
	_bindableMapPrevious = _bindableMap;
	_unbindableMapPrevious = _unbindableMap;
	_bindableMap =
	    static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) & ~static_cast<uint64_t>(_bindablePulses));
	_bindablePulses = BindableActionMap::NONE;
	_delta -= glm::round(_delta);

	DropStaleFingers();

	if (_gesture == Gesture::Pending && Clock::now() - _downTime >= k_LongPressDelay)
	{
		SetGesture(Gesture::LongPress);
		SetBindable(BindableActionMap::ACTION, true);
	}

	if (_holdPointer)
	{
		_holdPointer = false;
		if (const auto finger = _fingers.find(_gestureFingers[0]); finger != _fingers.end())
		{
			_pointer = finger->second;
		}
	}
}

void TouchGameActionMap::ProcessEvent(const SDL_Event& event)
{
	if (event.type != SDL_EVENT_FINGER_DOWN && event.type != SDL_EVENT_FINGER_MOTION && event.type != SDL_EVENT_FINGER_UP &&
	    event.type != SDL_EVENT_FINGER_CANCELED)
	{
		return;
	}
	const auto& finger = event.tfinger;
	if (finger.touchID == SDL_MOUSE_TOUCHID)
	{
		return;
	}
	if (_fingers.empty())
	{
		_touchId = finger.touchID;
	}
	else if (finger.touchID != _touchId)
	{
		return;
	}

	if (event.type == SDL_EVENT_FINGER_DOWN)
	{
		OnFingerDown(finger.fingerID, ToWindowCoordinates(finger));
	}
	else if (event.type == SDL_EVENT_FINGER_MOTION)
	{
		OnFingerMotion(finger.fingerID, ToWindowCoordinates(finger));
	}
	else
	{
		OnFingerUp(finger.fingerID, event.type == SDL_EVENT_FINGER_CANCELED);
	}
}

void TouchGameActionMap::OnFingerDown(SDL_FingerID id, glm::vec2 position)
{
	_fingers.insert_or_assign(id, position);
	const auto now = Clock::now();

	if (_fingers.size() == 1)
	{
		_gestureFingers[0] = id;
		_downTime = now;
		_downPosition = position;
		_pointer = position;
		_delta = glm::vec2(0.0f);

		if (_lastTap.has_value() && now - _lastTap->first <= k_DoubleTapDelay &&
		    glm::distance(position, _lastTap->second) <= k_DoubleTapDistance * GetShortSide())
		{
			_lastTap.reset();
			SetGesture(Gesture::DoubleTap);
			SetUnbindable(UnbindableActionMap::DOUBLE_CLICK, true);
		}
		else
		{
			SetGesture(Gesture::Pending);
		}
	}
	else if (_fingers.size() == 2)
	{
		SetBindable(BindableActionMap::MOVE, false);
		SetBindable(BindableActionMap::ACTION, false);
		SetUnbindable(UnbindableActionMap::DOUBLE_CLICK, false);
		_lastTap.reset();

		std::ranges::transform(_fingers, _gestureFingers.begin(), [](const auto& pair) { return pair.first; });
		_twoFingerStart = GetTwoFingerMetrics();
		_twoFingerLast = _twoFingerStart;
		_pointer = _twoFingerStart.midpoint;
		_delta = glm::vec2(0.0f);
		SetGesture(Gesture::TwoFinger);
	}
	else
	{
		SetBindable(BindableActionMap::ROTATE_AROUND_MOUSE_ON, false);
		SetUnbindable(UnbindableActionMap::TWO_BUTTON_CLICK, false);
		SetGesture(Gesture::Finished);
	}
}

void TouchGameActionMap::OnFingerMotion(SDL_FingerID id, glm::vec2 position)
{
	const auto finger = _fingers.find(id);
	if (finger == _fingers.end())
	{
		return;
	}
	finger->second = position;

	switch (_gesture)
	{
	case Gesture::Pending:
		if (glm::distance(position, _downPosition) > k_TapSlop * GetShortSide())
		{
			SetGesture(Gesture::Drag);
			SetBindable(BindableActionMap::MOVE, true);
			_holdPointer = true;
		}
		break;
	case Gesture::Drag:
	case Gesture::LongPress:
	case Gesture::DoubleTap:
		if (!_holdPointer)
		{
			_pointer = position;
		}
		break;
	case Gesture::TwoFinger:
	case Gesture::Pinch:
	case Gesture::Twist:
	case Gesture::Tilt:
		UpdateTwoFingerGesture();
		break;
	case Gesture::None:
	case Gesture::Finished:
		break;
	}
}

void TouchGameActionMap::OnFingerUp(SDL_FingerID id, bool canceled)
{
	if (_fingers.erase(id) == 0)
	{
		return;
	}

	switch (_gesture)
	{
	case Gesture::Pending:
		if (!canceled)
		{
			SetBindable(BindableActionMap::MOVE, true);
			_bindablePulses = WithAction(_bindablePulses, BindableActionMap::MOVE, true);
			_lastTap = std::make_pair(Clock::now(), _downPosition);
		}
		break;
	case Gesture::Drag:
		SetBindable(BindableActionMap::MOVE, false);
		break;
	case Gesture::LongPress:
		SetBindable(BindableActionMap::ACTION, false);
		break;
	case Gesture::DoubleTap:
		SetUnbindable(UnbindableActionMap::DOUBLE_CLICK, false);
		break;
	case Gesture::TwoFinger:
	case Gesture::Pinch:
	case Gesture::Twist:
	case Gesture::Tilt:
		SetBindable(BindableActionMap::ROTATE_AROUND_MOUSE_ON, false);
		SetUnbindable(UnbindableActionMap::TWO_BUTTON_CLICK, false);
		break;
	case Gesture::None:
	case Gesture::Finished:
		break;
	}

	SetGesture(_fingers.empty() ? Gesture::None : Gesture::Finished);
}

void TouchGameActionMap::UpdateTwoFingerGesture()
{
	if (!_fingers.contains(_gestureFingers[0]) || !_fingers.contains(_gestureFingers[1]))
	{
		return;
	}

	const auto current = GetTwoFingerMetrics();
	const auto shortSide = GetShortSide();
	const auto toReference = k_ReferenceShortSide / shortSide;

	switch (_gesture)
	{
	case Gesture::TwoFinger:
		if (glm::abs(current.distance - _twoFingerStart.distance) > k_PinchThreshold * shortSide)
		{
			SetGesture(Gesture::Pinch);
			SetUnbindable(UnbindableActionMap::TWO_BUTTON_CLICK, true);
		}
		else if (glm::abs(WrapAngle(current.angle - _twoFingerStart.angle)) > k_TwistThreshold)
		{
			SetGesture(Gesture::Twist);
			SetBindable(BindableActionMap::ROTATE_AROUND_MOUSE_ON, true);
		}
		else if (glm::abs(current.midpoint.y - _twoFingerStart.midpoint.y) > k_TiltThreshold * shortSide)
		{
			SetGesture(Gesture::Tilt);
			SetBindable(BindableActionMap::ROTATE_AROUND_MOUSE_ON, true);
		}
		break;
	case Gesture::Pinch:
		_delta.y -= (current.distance - _twoFingerLast.distance) * toReference * k_PinchScale;
		break;
	case Gesture::Twist:
		_delta.x += WrapAngle(current.angle - _twoFingerLast.angle) * k_TwistScale;
		break;
	case Gesture::Tilt:
		_delta.y += (current.midpoint.y - _twoFingerLast.midpoint.y) * toReference * k_TiltScale;
		break;
	default:
		break;
	}

	_twoFingerLast = current;
}

void TouchGameActionMap::DropStaleFingers()
{
	if (_fingers.empty() || SDL_HasEvents(SDL_EVENT_FINGER_DOWN, SDL_EVENT_FINGER_CANCELED))
	{
		return;
	}

	int count = 0;
	SDL_Finger** fingers = SDL_GetTouchFingers(_touchId, &count);
	const std::span<SDL_Finger*> current(fingers, fingers != nullptr ? static_cast<size_t>(count) : 0);
	std::vector<SDL_FingerID> stale;
	for (const auto& [id, position] : _fingers)
	{
		if (std::ranges::none_of(current, [id](const SDL_Finger* finger) { return finger->id == id; }))
		{
			stale.push_back(id);
		}
	}
	SDL_free(fingers);

	for (const auto id : stale)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("input"), "Touch: dropping stale finger {}", id);
		OnFingerUp(id, true);
	}
}

void TouchGameActionMap::SetGesture(Gesture gesture)
{
	if (gesture != _gesture)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("input"), "Touch: {} -> {}", GestureName(static_cast<uint8_t>(_gesture)),
		                    GestureName(static_cast<uint8_t>(gesture)));
	}
	_gesture = gesture;
}

void TouchGameActionMap::SetBindable(BindableActionMap action, bool value)
{
	_bindableMap = WithAction(_bindableMap, action, value);
}

void TouchGameActionMap::SetUnbindable(UnbindableActionMap action, bool value)
{
	_unbindableMap = WithAction(_unbindableMap, action, value);
}

TouchGameActionMap::TwoFingerMetrics TouchGameActionMap::GetTwoFingerMetrics() const
{
	const auto a = _fingers.at(_gestureFingers[0]);
	const auto b = _fingers.at(_gestureFingers[1]);
	const auto offset = b - a;
	return {
	    .distance = glm::length(offset),
	    .angle = glm::atan(offset.y, offset.x),
	    .midpoint = (a + b) * 0.5f,
	};
}

glm::vec2 TouchGameActionMap::ToWindowCoordinates(const SDL_TouchFingerEvent& event)
{
	return glm::vec2(event.x, event.y) * GetWindowSize();
}
