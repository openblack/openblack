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

#include <array>
#include <string_view>

namespace openblack::graphics
{

enum class RenderPass : uint8_t
{
	Footprint,
	ObjectShadow,
	HandShadow,
	Reflection,
	Main,
	Interface,
	ImGui,
	/// The game's pointer, over the debug windows too
	Cursor,
	MeshViewer,

	_count
};

static constexpr std::array<std::string_view, static_cast<uint8_t>(RenderPass::_count)> k_RenderPassNames {
    "Footprint Pass",     //
    "Object Shadow Pass", //
    "Hand Shadow Pass",   //
    "Reflection Pass",    //
    "Main Pass",          //
    "Interface Pass",     //
    "ImGui Pass",         //
    "Cursor Pass",        //
    "Mesh Viewer Pass",   //
};

} // namespace openblack::graphics
