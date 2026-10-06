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
#include <string>

#include "EditorPalette.h"

namespace openblack::ecs
{
class Registry;
}
namespace openblack::ecs::systems
{
class EditorSystemInterface;
}
namespace openblack::debug::gui
{
class CreatureSpawner;
}

namespace openblack::editor
{
struct NameTables;

/// Placing things from the palette: the item that follows the mouse until a click puts it down, and the way it faces
struct Placement
{
	std::optional<PlaceItem> item;
	float yawRadians {0.0f};
	/// What became of the last placing
	std::string last;
};

/// What the editor's panels share
struct EditorContext
{
	ecs::systems::EditorSystemInterface& system;
	ecs::Registry& registry;
	const NameTables& names;
	debug::gui::CreatureSpawner& spawner;
	Placement& placement;
};

} // namespace openblack::editor
