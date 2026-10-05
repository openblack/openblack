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

#include <algorithm>
#include <array>
#include <string_view>

namespace openblack::graphics
{

enum class RenderPass : uint8_t
{
	/// The clouds' shadows over the land this frame, and the lights at night
	LandShade,
	/// The land's luminosity this frame: as it was laid, shaded by the clouds and lit by the lights
	LandLuminosity,
	/// The land cells' colours this frame, which light the land and the models on it
	LandColour,
	/// The sky's dome for each alignment, blended for the time of day a band of rows at a time
	SkyDome,
	Footprint,
	/// The rivers' channels, into the land's alpha
	LandAlpha,
	ObjectShadow,
	/// The land seen from above, which the map in the temple's pool is textured with
	TempleMap,
	HandShadow,
	/// The sky of the sea's reflection, drawn first into its target
	ReflectionSky,
	Reflection,
	/// The sky, drawn first into the view: everything in the scene is drawn over it and in front of it
	Sky,
	Main,
	Interface,
	ImGui,
	/// The game's pointer, over the debug windows too
	Cursor,
	MeshViewer,

	_count
};

static constexpr std::array<std::string_view, static_cast<uint8_t>(RenderPass::_count)> k_RenderPassNames {
    "Land Shade Pass",      //
    "Land Luminosity Pass", //
    "Land Colour Pass",     //
    "Sky Dome Pass",        //
    "Footprint Pass",       //
    "Land Alpha Pass",      //
    "Object Shadow Pass",   //
    "Temple Map Pass",      //
    "Hand Shadow Pass",     //
    "Reflection Sky Pass",  //
    "Reflection Pass",      //
    "Sky Pass",             //
    "Main Pass",            //
    "Interface Pass",       //
    "ImGui Pass",           //
    "Cursor Pass",          //
    "Mesh Viewer Pass",     //
};
// Every pass has a name: a short list would leave the last ones empty
static_assert(std::ranges::none_of(k_RenderPassNames, &std::string_view::empty));

/// Whether a pass draws a scene with its sky in a pass of its own before it
[[nodiscard]] constexpr bool HasSkyPass(RenderPass pass)
{
	return pass == RenderPass::Main || pass == RenderPass::Reflection;
}

/// The pass a scene's sky is drawn in: the one before it, into the same target, or the scene's own if it has none
[[nodiscard]] constexpr RenderPass SkyPassOf(RenderPass pass)
{
	switch (pass)
	{
	case RenderPass::Main:
		return RenderPass::Sky;
	case RenderPass::Reflection:
		return RenderPass::ReflectionSky;
	default:
		return pass;
	}
}

} // namespace openblack::graphics
