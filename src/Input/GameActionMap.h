/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <optional>
#include <unordered_map>

#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "GameActionMapInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#warning "Locator interface implementations should only be included in Locator.cpp"
#endif

namespace openblack::input
{
class GameActionMap final: public GameActionInterface
{

public:
	GameActionMap();
	GameActionMap(const GameActionMap&) = delete;
	GameActionMap& operator=(const GameActionMap&) = delete;

	bool GetBindable(BindableActionMap action) const final;
	bool GetUnbindable(UnbindableActionMap action) const final;
	bool GetBindableChanged(BindableActionMap action) const final;
	bool GetUnbindableChanged(UnbindableActionMap action) const final;
	bool GetBindableRepeat(BindableActionMap action) const final;
	bool GetUnbindableRepeat(UnbindableActionMap action) const final;

	void Frame() final;
	void ProcessEvent(const SDL_Event& event) final;

private:
	BindableActionMap _bindableMap = BindableActionMap::NONE;
	UnbindableActionMap _unbindableMap = UnbindableActionMap::NONE;
	BindableActionMap _bindableMapPrevious = BindableActionMap::NONE;
	UnbindableActionMap _unbindableMapPrevious = UnbindableActionMap::NONE;
	std::unordered_map<SDL_Keycode, std::pair<SDL_Keymod, BindableActionMap>> _keyboardModBindings;
	std::unordered_map<SDL_Keycode, BindableActionMap> _keyboardBindings;
	std::unordered_map<uint8_t /*mousebutton*/, BindableActionMap> _mouseBindings;
	std::unordered_map<uint8_t /*mousebutton*/, std::pair<SDL_Keymod, BindableActionMap>> _mouseModBindings;
	std::array<std::optional<BindableActionMap>, 2> _mouseWheelBinding; // up, down
};
} // namespace openblack::input
