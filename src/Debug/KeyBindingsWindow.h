/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <SDL_scancode.h>

#include "Window.h"

namespace openblack::debug::gui
{

/// The options screen's controls: every action with its category, key and mouse input and whether it does anything
/// yet. A key can be bound anew, and each action pressed as its key would press it, to try out its handling.
class KeyBindingsWindow final: public Window
{
public:
	KeyBindingsWindow() noexcept;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;
	[[nodiscard]] bool TakesEvent(const SDL_Event& event) const noexcept override;

private:
	/// The row waiting for its new key
	std::optional<size_t> _rebinding;
	/// A modifier key pressed while waiting, which is bound alone if it is let go before another key is pressed
	std::optional<SDL_Scancode> _modifierDown;
};

} // namespace openblack::debug::gui
