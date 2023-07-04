/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once
#include <filesystem>

#include <glm/mat4x4.hpp>
#include <entt/core/hashed_string.hpp>

#include <LNDFile.h>
#include "Extent2D.h"
#include "LandBlock.h"

using LNDCell = openblack::lnd::LNDCell;

namespace openblack
{
class LandBlock;
namespace graphics
{
class FrameBuffer;
class Texture2D;
} // namespace graphics
class LandIslandInterface
{
public:
	static const uint8_t k_CellCount;
	static const float k_HeightUnit;
	static const float k_CellSize;
	static constexpr entt::hashed_string k_SmallBumpTextureId = entt::hashed_string("raw/smallbumpa");

	[[nodiscard]] virtual float GetHeightAt(glm::vec2) const = 0;
	[[nodiscard]] virtual const LNDCell& GetCell(const glm::u16vec2& coordinates) const = 0;

	// Debug
	virtual void DumpTextures() const = 0;
	virtual void DumpMaps() const = 0;

	[[nodiscard]] virtual std::vector<LandBlock>& GetBlocks() = 0;
	[[nodiscard]] virtual const std::vector<LandBlock>& GetBlocks() const = 0;
	[[nodiscard]] virtual const std::vector<lnd::LNDCountry>& GetCountries() const = 0;

	[[nodiscard]] virtual const graphics::Texture2D& GetAlbedoArray() const = 0;
	[[nodiscard]] virtual const graphics::Texture2D& GetBump() const = 0;
	[[nodiscard]] virtual const graphics::Texture2D& GetHeightMap() const = 0;
	[[nodiscard]] virtual const graphics::FrameBuffer& GetFootprintFramebuffer() const = 0;

	virtual IndexExtent GetIndexExtent() const = 0;
	virtual glm::mat4 GetOrthoView() const = 0;
	virtual glm::mat4 GetOrthoProj() const = 0;
	virtual Extent2D GetExtent() const = 0;
	virtual uint8_t GetNoise(glm::u8vec2 pos) = 0;
};
} // namespace openblack
