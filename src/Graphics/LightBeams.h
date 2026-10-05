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

#include <span>
#include <vector>

#include <glm/ext/vector_uint4_sized.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack
{
struct LightCone;
}

namespace openblack::l3d
{
struct L3DVertex;
}

namespace openblack::graphics
{

/// Shafts of light drawn additively: the texture times the vertices' colours, added by alpha,
/// without writing depth and from both sides
struct BeamVertex
{
	glm::vec3 position;
	glm::u8vec4 colour;
	glm::vec2 uv;
};

struct BeamMesh
{
	std::vector<BeamVertex> vertices;
	std::vector<uint16_t> indices;
};

/// The beam the game's glow of a spot light draws: an open cone of 8 sides from the light, coloured there and fading to
/// black at its far end. Its texture, the atmosphere's, drifts around as phase goes on.
void AppendCone(const LightCone& cone, float phase, BeamMesh& mesh);

/// How far the game draws a window's edges out, of the length its name record gives
constexpr float k_VolumeLightLengthScale = 0.55f;

/// The volume of light a window sheds: each edge of its triangles drawn out away from the source,
/// with the window's texture, half lit at the window and fading to nothing
[[nodiscard]] BeamMesh MakeVolumeLight(std::span<const l3d::L3DVertex> vertices, std::span<const uint16_t> indices,
                                       glm::vec3 source, float length);

} // namespace openblack::graphics
