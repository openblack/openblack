/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <unordered_map>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_touch.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "GameActionMapInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp"
#endif

namespace openblack::input
{
class TouchGameActionMap final: public GameActionInterface
{
public:
	[[nodiscard]] bool GetBindable(BindableActionMap action) const final;
	[[nodiscard]] bool GetUnbindable(UnbindableActionMap action) const final;
	[[nodiscard]] bool GetBindableChanged(BindableActionMap action) const final;
	[[nodiscard]] bool GetUnbindableChanged(UnbindableActionMap action) const final;
	[[nodiscard]] bool GetBindableRepeat(BindableActionMap action) const final;
	[[nodiscard]] bool GetUnbindableRepeat(UnbindableActionMap action) const final;
	[[nodiscard]] glm::uvec2 GetMousePosition() const final;
	[[nodiscard]] glm::ivec2 GetMouseDelta() const final;
	[[nodiscard]] std::array<std::optional<glm::vec3>, 2> GetHandPositions() const final;

	void Frame() final;
	void ProcessEvent(const SDL_Event& event) final;

private:
	using Clock = std::chrono::steady_clock;

	enum class Gesture : uint8_t
	{
		None,
		Pending,
		Drag,
		LongPress,
		DoubleTap,
		TwoFinger,
		Pinch,
		Twist,
		Tilt,
		Finished,
	};

	struct TwoFingerMetrics
	{
		float distance;
		float angle;
		glm::vec2 midpoint;
	};

	void OnFingerDown(SDL_FingerID id, glm::vec2 position);
	void OnFingerMotion(SDL_FingerID id, glm::vec2 position);
	void OnFingerUp(SDL_FingerID id, bool canceled);
	void UpdateTwoFingerGesture();
	void DropStaleFingers();
	void SetGesture(Gesture gesture);
	void SetBindable(BindableActionMap action, bool value);
	void SetUnbindable(UnbindableActionMap action, bool value);
	[[nodiscard]] TwoFingerMetrics GetTwoFingerMetrics() const;
	[[nodiscard]] static glm::vec2 ToWindowCoordinates(const SDL_TouchFingerEvent& event);

	BindableActionMap _bindableMap = BindableActionMap::NONE;
	UnbindableActionMap _unbindableMap = UnbindableActionMap::NONE;
	BindableActionMap _bindableMapPrevious = BindableActionMap::NONE;
	UnbindableActionMap _unbindableMapPrevious = UnbindableActionMap::NONE;
	BindableActionMap _bindablePulses = BindableActionMap::NONE;

	SDL_TouchID _touchId = 0;
	std::unordered_map<SDL_FingerID, glm::vec2> _fingers;
	std::array<SDL_FingerID, 2> _gestureFingers {};
	Gesture _gesture = Gesture::None;

	Clock::time_point _downTime;
	glm::vec2 _downPosition {0.0f, 0.0f};
	std::optional<std::pair<Clock::time_point, glm::vec2>> _lastTap;

	TwoFingerMetrics _twoFingerStart {};
	TwoFingerMetrics _twoFingerLast {};

	glm::vec2 _pointer {0.0f, 0.0f};
	bool _holdPointer = false;
	glm::vec2 _delta {0.0f, 0.0f};
};
} // namespace openblack::input
