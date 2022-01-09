/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace openblack::graphics
{

enum class RenderPass : uint8_t
{
	Reflection,
	Main,
	ImGui,
	MeshViewer,

	_count
};

static constexpr std::array<std::string_view, static_cast<uint8_t>(RenderPass::_count)> RenderPassNames {
    "Reflection Pass",
    "Main Pass",
    "ImGui Pass",
    "Mesh Viewer Pass",
};

} // namespace openblack::graphics
