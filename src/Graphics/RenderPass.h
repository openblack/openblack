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
	/// The land's luminosity this frame: as it was laid, with the clouds' shadows over it
	LandLuminosity,
	Footprint,
	/// The rivers' channels, into the land's alpha
	LandAlpha,
	ObjectShadow,
	/// The land seen from above, which the map in the temple's pool is textured with
	TempleMap,
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
    "Land Luminosity Pass", //
    "Footprint Pass",       //
    "Land Alpha Pass",      //
    "Object Shadow Pass",   //
    "Temple Map Pass",      //
    "Hand Shadow Pass",     //
    "Reflection Pass",      //
    "Main Pass",            //
    "Interface Pass",       //
    "ImGui Pass",           //
    "Cursor Pass",          //
    "Mesh Viewer Pass",     //
};

} // namespace openblack::graphics
