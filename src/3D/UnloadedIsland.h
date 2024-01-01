/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "3D/LandIslandInterface.h"

namespace openblack
{
/// Default Island when none have been loaded that just throws errors when accessed
class UnloadedIsland final: public LandIslandInterface
{
public:
	[[nodiscard]] float GetHeightAt(glm::vec2) const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] const LandBlock* GetBlock(const glm::u8vec2&) const
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] const lnd::LNDCell& GetCell(const glm::u16vec2&) const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	void DumpTextures() const override { throw std::runtime_error("Cannot get landscape before any are loaded"); }

	void DumpMaps() const override { throw std::runtime_error("Cannot get landscape before any are loaded"); }

	[[nodiscard]] std::vector<LandBlock>& GetBlocks() override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] const std::vector<LandBlock>& GetBlocks() const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] const std::vector<lnd::LNDCountry>& GetCountries() const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] const graphics::Texture2D& GetAlbedoArray() const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] const graphics::Texture2D& GetBump() const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] const graphics::Texture2D& GetHeightMap() const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] const graphics::FrameBuffer& GetFootprintFramebuffer() const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] glm::mat4 GetOrthoView() const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] glm::mat4 GetOrthoProj() const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] U16Extent2 GetIndexExtent() const override
	{
		throw std::runtime_error("Cannot get landscape before any are loaded");
	}

	[[nodiscard]] Extent2 GetExtent() const override { throw std::runtime_error("Cannot get landscape before any are loaded"); }

	uint8_t GetNoise(glm::u8vec2) override { throw std::runtime_error("Cannot get landscape before any are loaded"); }
};
} // namespace openblack
