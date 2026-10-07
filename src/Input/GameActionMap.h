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
#include <bitset>
#include <optional>
#include <unordered_map>
#include <vector>

#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <glm/vec3.hpp>

#include "CursorFreeze.h"
#include "GameActionMapInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp"
#endif

namespace openblack::input
{
class GameActionMap final: public GameActionInterface
{

public:
	GameActionMap();
	GameActionMap(const GameActionMap&) = delete;
	GameActionMap& operator=(const GameActionMap&) = delete;

	[[nodiscard]] bool GetBindable(BindableActionMap action) const final;
	[[nodiscard]] bool GetUnbindable(UnbindableActionMap action) const final;
	[[nodiscard]] bool GetBindableChanged(BindableActionMap action) const final;
	[[nodiscard]] bool GetUnbindableChanged(UnbindableActionMap action) const final;
	[[nodiscard]] bool GetBindableRepeat(BindableActionMap action) const final;
	[[nodiscard]] bool GetUnbindableRepeat(UnbindableActionMap action) const final;
	[[nodiscard]] glm::uvec2 GetMousePosition() const final;
	[[nodiscard]] glm::ivec2 GetMouseDelta() const final;
	[[nodiscard]] float GetMouseWheelDelta() const final;
	[[nodiscard]] std::array<std::optional<glm::vec3>, 2> GetHandPositions() const final;

	[[nodiscard]] std::span<const KeyBinding> GetKeyBindings() const final;
	void SetKeyBinding(BindableActionMap action, std::optional<KeyChord> key) final;
	void ResetKeyBindings() final;
	void QueuePress(BindableActionMap action) final;
	[[nodiscard]] bool HasQueuedPresses() const final;

	void SetScriptedPointer(std::optional<ScriptedPointer> pointer) final;
	[[nodiscard]] std::optional<ScriptedPointer> GetScriptedPointer() const final;
	void WarpCursor(glm::ivec2 position) final;
	[[nodiscard]] std::optional<glm::ivec2> GetCursorWarp() const final;
	void AllowCursorFreeze(bool allowed) final;
	[[nodiscard]] bool IsCursorFrozen() const final;

	void Frame() final;
	void ProcessEvent(const SDL_Event& event) final;

private:
	BindableActionMap _bindableMap = BindableActionMap::NONE;
	UnbindableActionMap _unbindableMap = UnbindableActionMap::NONE;
	BindableActionMap _bindableMapPrevious = BindableActionMap::NONE;
	UnbindableActionMap _unbindableMapPrevious = UnbindableActionMap::NONE;
	/// Puts the mouse's bindings in the lookups the mouse events use, after the table changes
	void ApplyMouseBindings();
	/// Lets go of the keys whose letting go went elsewhere, as to a text field of the debug windows
	void ReleaseKeysNoLongerHeld();
	/// The buttons held and where the pointer is, the mouse's or the scripted pointer's
	uint32_t PointerState(glm::ivec2* position) const;
	/// Makes the presses queued for testing, and lets go of last frame's
	void ApplyQueuedPresses();

	/// What each of the options screen's actions is bound to, starting as the game's defaults
	KeyBindingTable _bindings {k_DefaultKeyBindings};
	/// The keys down, as pressed, so their letting go can be made up when it goes elsewhere
	std::bitset<SDL_NUM_SCANCODES> _heldKeys;
	std::vector<BindableActionMap> _queuedPresses;
	/// The events that let go of last frame's queued presses
	std::vector<SDL_Event> _queuedReleases;
	/// The actions without a key pressed for testing, let go of the next frame
	BindableActionMap _queuedHeld = BindableActionMap::NONE;
	std::unordered_map<int /*mousebutton*/, BindableActionMap> _mouseBindings;
	std::unordered_map<int /*mousebutton*/, std::pair<SDL_Keymod, BindableActionMap>> _mouseModBindings;
	uint8_t _currentMouseButtons = 0;
	std::array<std::optional<BindableActionMap>, 2> _mouseWheelBinding; // up, down
	glm::uvec2 _mousePosition;
	glm::ivec2 _mouseDelta {0};
	float _mouseWheelDelta = 0.0f;
	/// Holds the cursor while the mouse turns the camera
	CursorFreeze _cursorFreeze;
	bool _cursorFreezeAllowed {false};
	std::optional<ScriptedPointer> _scriptedPointer;
	std::optional<glm::ivec2> _cursorWarp;
};
} // namespace openblack::input
