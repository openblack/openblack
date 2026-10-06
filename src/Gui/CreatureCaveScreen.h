/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::gui
{

/// Draws the Creature Cave's screen while it is open, with ImGui: a page for each of the cave's four scrolls, written as
/// the temple's scrolls are, with what the creature has learnt to do and not to do and how far it has learnt each
/// miracle, and a page for its tattoos. It is the 2D stand-in for the parts of the creature's room the temple doesn't
/// show yet, the creature itself above all, which a click on opens the tattoo editor in the game.
void DrawCreatureCaveScreen();

} // namespace openblack::gui
