/*******************************************************************************
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
#include <functional>
#include <optional>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/OrientedText.h"

namespace openblack
{
namespace lnd
{
struct LNDCell;
}

/// The island in relief over the main room's pool (MiniMap).
///
/// The map has a vertex at every eighth cell of the land, two a block, 65 by 65 over the 32 by 32 blocks there can be.
/// It is framed on the blocks the island has, so that its diagonal is 11 units however big the island, and centred in
/// the room. Every frame it takes the land's heights and brightness afresh, and fades out at the coast.
class TempleMap
{
public:
	/// The cell of the land at cell coordinates, null where there is no block (LandIslandInterface::FindCell)
	using FindCell = std::function<const lnd::LNDCell*(glm::u16vec2)>;
	static constexpr int32_t k_Vertices = 65;
	static constexpr int32_t k_Blocks = 32;
	/// How long the map's diagonal is, in the room's units
	static constexpr float k_Diagonal = 11.0f;
	/// The world units to a vertex of the map: 8 cells of 10
	static constexpr float k_WorldPerVertex = 80.0f;
	/// The world units the land's texture spans, along x and z
	static constexpr float k_TextureSpan = k_WorldPerVertex * (k_Vertices - 1);

	/// MiniMap::BuildMapTex: frames the map on the blocks the land has. False without any.
	bool Frame(const FindCell& findCell);

	/// fn_007977A0: takes the land's heights and brightness, and gives the map's triangles in the main room, textured by
	/// the land seen from above, its u along x and v along z across the texture's span
	void Build(const FindCell& findCell, std::vector<OrientedTextVertex>& triangles);

	/// MiniMap::CalcPoint: where a point of the world, in x and z, lies on the map, at the height of the map's land
	/// there as last built
	[[nodiscard]] glm::vec3 ToMap(glm::vec2 world) const;
	/// fn_00797760: the point of the world in x and z under a point of the map
	[[nodiscard]] glm::vec2 ToWorld(glm::vec3 map) const;
	/// The room's units to a world unit across the map (0xC383DC)
	[[nodiscard]] float GetWorldScale() const { return _scale / k_WorldPerVertex; }

private:
	/// The map's units to a vertex, and the vertex at its centre, doubled (MiniMap +0, +4 and +8)
	float _scale {0.0f};
	glm::ivec2 _centre {0, 0};
	/// The land's altitude at each vertex, as last built, rows along z
	std::array<uint8_t, static_cast<size_t>(k_Vertices* k_Vertices)> _altitudes {};
};

} // namespace openblack
