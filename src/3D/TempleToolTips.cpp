/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TempleToolTips.h"

#include <algorithm>
#include <array>
#include <vector>

using namespace openblack;
using gui::ToolTipAction;
namespace Arrows = gui::ToolTipArrows;

namespace
{
/// The temple's tooltips, by their text less 0xE73
constexpr uint32_t k_ZoomIn = 0xE75 - 0xE73;
constexpr uint32_t k_Move = 0xE7E - 0xE73;
constexpr uint32_t k_ZoomOut = 0xE8B - 0xE73;
constexpr uint32_t k_WorldRoom = 0xE9B - 0xE73;
constexpr uint32_t k_ExitTemple = 0xE98 - 0xE73;
constexpr uint32_t k_TattooCreature = 0xEDF - 0xE73;
constexpr uint32_t k_Scroll = 0xEE2 - 0xE73;
constexpr uint32_t k_WorldStats = 0xEE3 - 0xE73;
constexpr uint32_t k_SaveGameStats = 0xEE4 - 0xE73;
constexpr uint32_t k_ChallengeStats = 0xEE5 - 0xE73;
constexpr uint32_t k_TiltRotate = 0xEED - 0xE73;
/// The main room's doors, in the game's order: the creature, options, exit, future, library, save game and challenge
/// rooms. The scroll's wall, door 6, leaves the tooltip be.
constexpr std::array<std::optional<uint32_t>, 8> k_Doors = {
    0xE95 - 0xE73, 0xE99 - 0xE73, 0xE98 - 0xE73, 0xE9A - 0xE73, 0xE97 - 0xE73, 0xE96 - 0xE73, std::nullopt, 0xE94 - 0xE73,
};

bool IsZoomed(const TempleToolTipInput& input)
{
	return input.zoom == 1.0f;
}

/// The main, challenge and save game rooms' scrolls. Close up, a scroll shows it can be dragged; from afar what it
/// tells of; and looking at it, the way back.
void UpdateRoomScroll(TempleToolTip& toolTip, const TempleToolTipInput& input, const TempleScrolls::Control& scroll,
                      uint32_t stats)
{
	if (input.hoveredSubMesh == scroll.subMesh)
	{
		if (IsZoomed(input))
		{
			toolTip = {.index = k_Scroll, .action = ToolTipAction::Select, .arrows = Arrows::k_UpDown};
		}
		// The world's scroll tells of the world whenever it isn't close, the others only once the camera is back
		else if (input.room == TempleRoom::Main || input.zoom == 0.0f)
		{
			toolTip = {.index = stats, .action = ToolTipAction::Select, .arrows = Arrows::k_None};
		}
	}
	else if (input.lookingAtScroll && IsZoomed(input))
	{
		toolTip = {.index = k_ZoomOut, .action = ToolTipAction::Select, .arrows = Arrows::k_None};
	}
}
} // namespace

void openblack::UpdateTempleToolTip(TempleToolTip& toolTip, const TempleToolTipInput& input)
{
	const bool overWayBack = input.overWayBack && !input.controlHeld;
	switch (input.room)
	{
	case TempleRoom::Main:
		// In the main room: moving about, the pool zooms in and once close tilts and turns the map, and the
		// doors lead to their rooms
		toolTip = {.index = k_Move, .action = ToolTipAction::Select, .arrows = Arrows::k_All};
		if (input.overPool)
		{
			if (input.pressingPool && IsZoomed(input))
			{
				toolTip.index = k_TiltRotate;
			}
			else if (!IsZoomed(input))
			{
				toolTip = {.index = k_ZoomIn, .action = ToolTipAction::Select, .arrows = Arrows::k_None};
			}
		}
		if (input.hoveredDoor.has_value() && !input.controlHeld && *input.hoveredDoor < k_Doors.size())
		{
			if (const auto door = k_Doors.at(*input.hoveredDoor); door.has_value())
			{
				toolTip = {.index = door, .action = ToolTipAction::Select, .arrows = Arrows::k_None};
			}
		}
		// The controls' draw callbacks, in the order of their submeshes (as the game sorts them on loading): the
		// buttons of what the map shows say what they show when hovered, and the scroll as the others' rooms do
		{
			std::vector<uint32_t> controls;
			for (const auto& toggle : input.toggles)
			{
				controls.push_back(toggle.subMesh);
			}
			for (const auto& scroll : input.scrolls)
			{
				controls.push_back(scroll.subMesh);
			}
			std::ranges::sort(controls);
			for (const auto subMesh : controls)
			{
				if (const auto toggle = std::ranges::find(input.toggles, subMesh, &TempleToggles::Control::subMesh);
				    toggle != input.toggles.end())
				{
					if (input.hoveredSubMesh == subMesh)
					{
						toolTip = {.index = toggle->toolTip, .action = ToolTipAction::Select, .arrows = Arrows::k_None};
					}
				}
				else
				{
					UpdateRoomScroll(toolTip, input,
					                 *std::ranges::find(input.scrolls, subMesh, &TempleScrolls::Control::subMesh),
					                 k_WorldStats);
				}
			}
		}
		break;
	case TempleRoom::Challenge:
	case TempleRoom::SaveGame:
	case TempleRoom::Credits:
		// The challenge, save game and credits rooms: moving about, and the way back
		toolTip = {.index = k_Move, .action = ToolTipAction::Select, .arrows = Arrows::k_All};
		if (overWayBack)
		{
			toolTip = {.index = k_WorldRoom, .action = ToolTipAction::Select, .arrows = Arrows::k_None};
		}
		// TODO(raffclar): the challenge room's pictures and replay button, and the save game room's pictures and
		// buttons, have tooltips of their own
		if (input.room != TempleRoom::Credits)
		{
			for (const auto& scroll : input.scrolls)
			{
				UpdateRoomScroll(toolTip, input, scroll,
				                 input.room == TempleRoom::Challenge ? k_ChallengeStats : k_SaveGameStats);
			}
		}
		break;
	case TempleRoom::CreatureCave:
	{
		// The creature's room leaves the arrows as they were, so after a scroll's they stay up and down
		toolTip.index = k_Move;
		toolTip.action = ToolTipAction::Select;
		if (overWayBack)
		{
			toolTip.index = k_WorldRoom;
		}
		// Zoomed, or zooming, to a target, the camera zooms back; the targets say what clicking them does
		if (input.zoomingToCaveTarget)
		{
			toolTip.index = k_ZoomOut;
		}
		if (input.caveTarget.has_value())
		{
			switch (*input.caveTarget)
			{
			case CreatureCaveTargets::Target::Creature:
				toolTip.index = k_TattooCreature;
				break;
			case CreatureCaveTargets::Target::Exit:
				toolTip.index = k_ExitTemple;
				break;
			default:
				toolTip.index = input.zoomingToCaveTarget ? k_ZoomOut : k_ZoomIn;
				break;
			}
		}
		// The scrolls' callbacks, in turn: hovered, a scroll sets its title, which isn't a tooltip, so none shows unless
		// it is close and the one looked at; looked at, another lets the way back show unless one before was hovered
		bool scrollHovered = false;
		for (const auto& scroll : input.scrolls)
		{
			const bool hovered = input.hoveredSubMesh == scroll.subMesh;
			if (hovered)
			{
				toolTip.index = std::nullopt;
				toolTip.action = ToolTipAction::Select;
				scrollHovered = true;
				if (IsZoomed(input) && scroll.focused)
				{
					toolTip = {.index = k_Scroll, .action = ToolTipAction::Select, .arrows = Arrows::k_UpDown};
				}
			}
			else if (IsZoomed(input) && !scrollHovered)
			{
				toolTip.index = k_ZoomOut;
				toolTip.action = ToolTipAction::Select;
			}
		}
		break;
	}
	default:
		// The options and future rooms set no tooltip
		break;
	}

	// In every room: none on the way in, or on the way through a door
	if (!input.inControl)
	{
		toolTip = {.index = std::nullopt, .action = ToolTipAction::None, .arrows = Arrows::k_None};
	}
}
