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

#include <filesystem>
#include <vector>

#include <entt/core/hashed_string.hpp>
#include <glm/mat4x4.hpp>

#include "Extent.h"

namespace openblack
{
class LandBlock;
namespace graphics
{
class FrameBuffer;
class Texture2D;
} // namespace graphics
namespace lnd
{
struct LNDCell;
struct LNDCountry;
} // namespace lnd
class LandIslandInterface
{
public:
	static const uint8_t k_CellCount;
	/// Cells along each side of the map
	static constexpr int32_t k_MapCellsPerSide = 512;
	static const float k_HeightUnit;
	static const float k_CellSize;
	/// The small bump detail drawn over the land near the camera: its colour and its alpha
	static constexpr entt::hashed_string k_SmallBumpTextureId = entt::hashed_string("raw/smallbump");
	static constexpr entt::hashed_string k_SmallBumpAlphaTextureId = entt::hashed_string("raw/smallbumpa");

	/// The height of the land at a world position, as the game works it out
	[[nodiscard]] virtual float GetHeightAt(glm::vec2) const = 0;
	[[nodiscard]] virtual glm::vec3 GetNormalAt(glm::vec2) const = 0;
	[[nodiscard]] virtual const lnd::LNDCell& GetCell(const glm::u16vec2& coordinates) const = 0;
	/// The cell at the given coordinates in the 17x17 cell array of its block, so the neighbours at +1 (z + 1),
	/// +17 (x + 1) and +18 are also valid. Null outside the map or where there is no block.
	[[nodiscard]] virtual const lnd::LNDCell* FindCell(const glm::u16vec2& coordinates) const = 0;

	// Debug
	virtual void DumpTextures() const = 0;
	virtual void DumpMaps() const = 0;

	[[nodiscard]] virtual std::vector<LandBlock>& GetBlocks() = 0;
	[[nodiscard]] virtual const std::vector<LandBlock>& GetBlocks() const = 0;
	[[nodiscard]] virtual const std::vector<lnd::LNDCountry>& GetCountries() const = 0;

	[[nodiscard]] virtual const graphics::Texture2D& GetHeightMap() const = 0;
	/// Each cell corner's luminosity, laid out as the height map, 255 where there is no block
	[[nodiscard]] virtual const graphics::Texture2D& GetLuminosityMap() const = 0;
	/// Each cell corner's colour, laid out as the height map and black where there is no block. The game reads a
	/// cell's colour bytes with red and blue swapped, as Direct3D's colours keep them, and so does this.
	[[nodiscard]] virtual const graphics::Texture2D& GetCellColourMap() const = 0;
	/// The blocks' painted textures (see block_texture), a layer for each block in the order of GetBlocks
	[[nodiscard]] virtual const graphics::Texture2D& GetBlockTextures() const = 0;
	[[nodiscard]] virtual const graphics::FrameBuffer& GetFootprintFramebuffer() const = 0;
	/// What of the land's alpha the rivers' channels leave, laid out as the footprints are
	[[nodiscard]] virtual const graphics::FrameBuffer& GetLandAlphaFramebuffer() const = 0;

	[[nodiscard]] virtual U16Extent2 GetIndexExtent() const = 0;
	[[nodiscard]] virtual glm::mat4 GetOrthoView() const = 0;
	[[nodiscard]] virtual glm::mat4 GetOrthoProj() const = 0;
	[[nodiscard]] virtual Extent2 GetExtent() const = 0;
	virtual uint8_t GetNoise(glm::u8vec2 pos) = 0;

	/// The height of the land at a map position, in 1/65536 of a cell. It is interpolated
	/// over the triangle of the cell the position is in, the way the land is drawn, with land at sea level flat.
	[[nodiscard]] double GetAltitude(int32_t mapX, int32_t mapZ) const;
	/// A world coordinate in 1/65536 of a cell (map_coords::ToFixed)
	[[nodiscard]] static int32_t ToMapCoords(float unit);
	/// The height the game draws land of a cell altitude at: land at or below sea level is drawn flat at height 0
	[[nodiscard]] static float GetDrawnAltitude(uint8_t altitude);
};
} // namespace openblack
