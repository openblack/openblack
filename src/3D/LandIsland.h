/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <entt/core/hashed_string.hpp>
#include <glm/mat4x4.hpp>

#include "LandBlock.h"

namespace openblack
{

namespace ecs::systems
{
class DynamicsSystem;
}

namespace graphics
{
class FrameBuffer;
}

namespace lnd
{
struct LNDCountry;
}

class LandIsland
{
public:
	static const uint8_t k_CellCount;
	static const float k_HeightUnit;
	static const float k_CellSize;
	static constexpr entt::hashed_string k_SmallBumpTextureId = entt::hashed_string("raw/smallbumpa");

	LandIsland();
	~LandIsland();

	void LoadFromFile(const std::filesystem::path& path);

	[[nodiscard]] float GetHeightAt(glm::vec2) const;
	[[nodiscard]] const LandBlock* GetBlock(const glm::u8vec2& coordinates) const;
	[[nodiscard]] const lnd::LNDCell& GetCell(const glm::u16vec2& coordinates) const;

	// Debug
	void DumpTextures() const;
	void DumpMaps() const;

private:
	[[nodiscard]] std::vector<uint8_t> CreateHeightMap() const;

	std::array<uint8_t, 1024> _blockIndexLookup {0};
	std::vector<LandBlock> _landBlocks;
	std::vector<lnd::LNDCountry> _countries;

	// Renderer, Dynamics
public:
	[[nodiscard]] std::vector<LandBlock>& GetBlocks() { return _landBlocks; }
	[[nodiscard]] const std::vector<LandBlock>& GetBlocks() const { return _landBlocks; }
	[[nodiscard]] const std::vector<lnd::LNDCountry>& GetCountries() const { return _countries; }
	[[nodiscard]] const graphics::Texture2D& GetAlbedoArray() const { return *_materialArray; }
	[[nodiscard]] const graphics::Texture2D& GetBump() const { return *_textureBumpMap; }
	[[nodiscard]] const graphics::Texture2D& GetHeightMap() const { return *_heightMap; }
	[[nodiscard]] const graphics::FrameBuffer& GetFootprintFramebuffer() const { return *_footprintFrameBuffer; }
	void GetOrthoViewProj(glm::mat4& view, glm::mat4& proj) const
	{
		view = _view;
		proj = _proj;
	}
	void GetIndexExtent(glm::u16vec2& extentMin, glm::u16vec2& extentMax) const
	{
		extentMin = _extentIndexMin;
		extentMax = _extentIndexMax;
	}
	void GetExtent(glm::vec2& extentMin, glm::vec2& extentMax) const
	{
		extentMin = _extentMin;
		extentMax = _extentMax;
	}

	uint8_t GetNoise(glm::u8vec2 pos);

private:
	std::unique_ptr<graphics::Texture2D> _materialArray;
	std::unique_ptr<graphics::Texture2D> _countryLookup;

	std::unique_ptr<graphics::Texture2D> _heightMap;
	std::unique_ptr<graphics::Texture2D> _textureNoiseMap;
	std::unique_ptr<graphics::Texture2D> _textureBumpMap;

	std::unique_ptr<graphics::FrameBuffer> _footprintFrameBuffer;
	glm::mat4 _proj;
	glm::mat4 _view;
	glm::u16vec2 _extentIndexMin;
	glm::u16vec2 _extentIndexMax;
	glm::vec2 _extentMin;
	glm::vec2 _extentMax;

	std::array<uint8_t, 256 * 256> _noiseMap;
};
} // namespace openblack

/*
LH3DIsland methods:
AdjustAlti((void *))
BuildSoundTGA((char *))
Create((void))
CreateCommonPart((void))
CreateSmallBump((void))
Draw((void))
DrawUnderWater((void))
GetAltitude((LH3DMapCoords const &))
GetAltitudeAndSetColorSpecular((LH3DMapCoords const &,ulong *,ulong *))
GetCell((long,long))
GetColorAndSpecular((LH3DMapCoords const &,ulong *,ulong *))
GetColorAndSpecular((LHPoint const *,ulong *,ulong *))
GetCountry((long,long,uchar &))
GetFogValue((LHPoint const *,ulong,ulong *))
GetIndex((long,long))
GetNormal((LH3DMapCoords const &,LHPoint *))
GetTerrainMaterial((ulong,ulong))
IsCompressed((void))
LoadFromDisk((char *))
PreDraw((void))
RayCast((LHPoint const &,LHPoint const &,float *,float *))
RayCastFrom2DPoint((LHCoord const &,float *,float *,bool,float))
RayCastInternal((float,float,float,float,float,float))
Release((void))
ReleaseSmallBump((void))
RequestUpdateAllTextures((void))
SaveOnDisk((char *))
SetColor((long,long,uchar))
SetCountry((long,long,long))
SetFileToLoad((char *))
SetFogColor((float,float,float,ulong))
SetFogMinMax((float,float))
SetHeightAsByte((long,long,long))
UseLowResTexture((int))
*/
