/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <string>
#include <vector>

#include "Editor/EditorPalette.h"

namespace openblack::editor
{
struct EditorContext;

/// Things to place on the land, by kind: creatures as the creature tools set them up, villagers and buildings by tribe,
/// and trees, features, mobile objects and mobile statics. Picking one has it follow the mouse until a click puts it
/// down. A whole list can be laid out at once, in rows round the middle of the view.
class PalettePanel
{
public:
	void Draw(EditorContext& context) noexcept;

private:
	void DrawCreatures(EditorContext& context) noexcept;
	/// The miracles' dispensers, or their bubbles on their own
	void DrawMiracles(EditorContext& context) noexcept;
	/// A searchable grid of a kind's types, those the filter lets through
	void DrawList(EditorContext& context, PlaceKind kind, const std::vector<int32_t>& types) noexcept;
	void LayOut(EditorContext& context, PlaceKind kind, const std::vector<int32_t>& types) noexcept;

	std::string _search;
	/// The tribe the villagers and buildings are narrowed to, or the ones of no tribe after the last tribe
	int _tribe {0};
	/// Placing the miracles' dispensers (0) or their bubbles (1)
	int _miracleKind {0};
	std::string _lastLayout;
};

} // namespace openblack::editor
