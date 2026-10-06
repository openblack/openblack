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

#include "Editor/EditorOutline.h"

namespace openblack::editor
{
struct EditorContext;

/// Every thing on the land by kind, searched; a click picks a thing and a double click brings it into view. The list is
/// read from the registry twice a second rather than every frame, and only the rows on screen are drawn.
class OutlinerPanel
{
public:
	void Draw(EditorContext& context) noexcept;
	/// Reads the list again at the next draw
	void Invalidate() noexcept { _framesToRefresh = 0; }

private:
	void Rebuild(EditorContext& context) noexcept;

	std::vector<OutlineEntry> _entries;
	std::vector<OutlineGroup> _groups;
	std::string _search;
	std::string _groupedFor;
	int _framesToRefresh {0};
	/// The selection the list last scrolled to
	uint32_t _scrolledFor {0};
};

} // namespace openblack::editor
