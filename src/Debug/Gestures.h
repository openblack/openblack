/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Window.h"

namespace openblack::debug::gui
{

/// The gestures drawn with the hand: what the hand is waiting for, the hand's path drawn over the screen with its
/// corners, the template that fits it most closely and how closely, and the last gesture recognised. Gestures can be
/// drawn through the recogniser from here, or sent to the miracles as if drawn.
class Gestures final: public Window
{
public:
	Gestures() noexcept;
	/// The overlay is also drawn with the window closed while the testbed draws a gesture, and briefly after one is
	/// recognised, so the scenarios can be watched
	void WindowDraw() noexcept override;

protected:
	void Draw() noexcept override;
	void Update() noexcept override {}
	void ProcessEventOpen([[maybe_unused]] const SDL_Event& event) noexcept override {}
	void ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept override {}

private:
	/// The path, its corners, the last gesture recognised and the closest template, over the screen
	void DrawOverlay() const noexcept;
	void DrawState() noexcept;
	void DrawTools() noexcept;

	bool _overlay {true};
	/// The gesture picked to draw
	int _gesture {4};
	/// The gesture is drawn with the Action button held, as a circle sizing a storm or shield is
	bool _holdAction {true};
};

} // namespace openblack::debug::gui
