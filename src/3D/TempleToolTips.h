/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <optional>
#include <span>

#include "3D/CreatureCaveTargets.h"
#include "3D/TempleInteriorInterface.h"
#include "3D/TempleScrolls.h"
#include "3D/TempleToggles.h"
#include "Gui/ToolTips.h"

namespace openblack
{

/// The tooltip the temple submits each turn: the one of its 170 (from text 0xE73), the action whose mouse it shows and
/// its arrows. It stays as it was in what the rooms don't set, as the game's do.
struct TempleToolTip
{
	/// None submits nothing, as an id outside the tooltips' does
	std::optional<uint32_t> index;
	gui::ToolTipAction action;
	uint32_t arrows;
};

/// What the rooms choose their tooltip by, each frame
struct TempleToolTipInput
{
	TempleRoom room;
	/// Whether the player has the room's camera, rather than its path or a walk through a door
	bool inControl;
	/// How close the camera has come to the scroll it looks at, and whether it looks at one
	float zoom;
	bool lookingAtScroll;
	/// Whether one of the room's controls has the mouse
	bool controlHeld;
	bool overPool;
	bool pressingPool;
	std::optional<uint32_t> hoveredDoor;
	bool overWayBack;
	/// The creature's room's target the cursor is over, and whether the camera zooms to one
	std::optional<CreatureCaveTargets::Target> caveTarget;
	bool zoomingToCaveTarget;
	/// The submesh of the room's mesh the cursor is over
	std::optional<uint32_t> hoveredSubMesh;
	std::span<const TempleScrolls::Control> scrolls;
	/// The main room's buttons drawn
	std::span<const TempleToggles::Control> toggles;
};

/// The tooltip before the temple has set one: Rotate, with the left button
constexpr TempleToolTip k_FirstTempleToolTip {.index = 4, .action = gui::ToolTipAction::Select, .arrows = 0};

/// What the hand shows in the temple for what it is over, as each room, its scrolls and then the temple as a whole
/// decide it
void UpdateTempleToolTip(TempleToolTip& toolTip, const TempleToolTipInput& input);

} // namespace openblack
