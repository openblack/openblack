/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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

#include "3D/LandIslandInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#warning "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{
class LandIsland final: public LandIslandInterface
{
public:
	explicit LandIsland(const std::filesystem::path& path);
	~LandIsland();

	void LoadFromFile(const std::filesystem::path& path);

	[[nodiscard]] float GetHeightAt(glm::vec2) const override;
	[[nodiscard]] glm::vec3 GetNormalAt(glm::vec2) const override;
	[[nodiscard]] const LandBlock* GetBlock(const glm::u8vec2& coordinates) const;
	[[nodiscard]] const lnd::LNDCell& GetCell(const glm::u16vec2& coordinates) const override;

	// Debug
	void DumpTextures() const override;
	void DumpMaps() const override;

private:
	[[nodiscard]] std::vector<uint8_t> CreateHeightMap() const;
	std::vector<LandBlock> _landBlocks;
	std::vector<lnd::LNDCountry> _countries;

	std::array<uint8_t, 1024> _blockIndexLookup {0};

	// Renderer, Dynamics
public:
	[[nodiscard]] std::vector<LandBlock>& GetBlocks() override { return _landBlocks; }
	[[nodiscard]] const std::vector<LandBlock>& GetBlocks() const override { return _landBlocks; }
	[[nodiscard]] const std::vector<lnd::LNDCountry>& GetCountries() const override { return _countries; }

	[[nodiscard]] const graphics::Texture2D& GetAlbedoArray() const override { return *_materialArray; }
	[[nodiscard]] const graphics::Texture2D& GetBump() const override { return *_textureBumpMap; }
	[[nodiscard]] const graphics::Texture2D& GetHeightMap() const override { return *_heightMap; }
	[[nodiscard]] const graphics::FrameBuffer& GetFootprintFramebuffer() const override { return *_footprintFrameBuffer; }

	[[nodiscard]] glm::mat4 GetOrthoView() const override { return _view; }
	[[nodiscard]] glm::mat4 GetOrthoProj() const override { return _proj; }
	[[nodiscard]] U16Extent2 GetIndexExtent() const override { return U16Extent2 {_extentIndexMin, _extentIndexMax}; }
	[[nodiscard]] Extent2 GetExtent() const override { return Extent2 {_extentMin, _extentMax}; }

	uint8_t GetNoise(glm::u8vec2 pos) override;

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
