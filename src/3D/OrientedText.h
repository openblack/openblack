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
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace openblack
{
namespace gui
{
class GameFont;
}

/// A corner of a letter of text laid out in the world, in triangles of three
struct OrientedTextVertex
{
	glm::vec3 position;
	glm::vec2 uv;
	/// RGBA, a byte each
	uint32_t colour;
};

/// The frame text is laid out in: an origin and three axes, of which the text runs along one, its letters go down
/// another, and it stands off the page along the third
struct TextFrame
{
	glm::vec3 origin {0.0f};
	std::array<glm::vec3, 3> axes {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};
};

/// Lays text out in a frame, from offset along the frame's across, down and third
/// axes, its letters size high down the down axis and stretched along the across axis. Each glyph is a quad of its
/// cached bitmap, as wide as its inked width and a unit more, coloured by colour. The third axis is the one that is
/// neither, as the game picks it.
void AppendOrientedText(std::vector<OrientedTextVertex>& vertices, const gui::GameFont& font, const TextFrame& frame,
                        int across, int down, std::u16string_view text, glm::vec3 offset, float size, float stretch,
                        glm::u8vec4 colour);

/// The axis the game stands text off the page along, for the across and down axes
[[nodiscard]] int OrientedTextDepthAxis(int across, int down);

} // namespace openblack
