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

#include <LNDFile.h>

using LNDCell = openblack::lnd::LNDCell;

namespace openblack
{
class LandBlock;
namespace graphics
{
class FrameBuffer;
class Texture2D;
}
class LandIslandSystemInterface
{
public:
	virtual void LoadFromFile(const std::filesystem::path& path) = 0;
	[[nodiscard]] virtual float GetHeightAt(glm::vec2) const = 0;
	[[nodiscard]] virtual const LandBlock* GetBlock(const glm::u8vec2& coordinates) const = 0;
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

	virtual void GetIndexExtent(glm::u16vec2& extentMin, glm::u16vec2& extentMax) const = 0;
	virtual void GetOrthoViewProj(glm::mat4& view, glm::mat4& proj) const = 0;
	virtual void GetExtent(glm::vec2& extentMin, glm::vec2& extentMax) const = 0; 
	virtual uint8_t GetNoise(glm::u8vec2 pos) = 0;
};
} // namespace openblack
