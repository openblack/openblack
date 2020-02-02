/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Graphics/Mesh.h"
#include "Graphics/Texture2D.h"
#include "LandBlock.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace openblack
{

namespace ecs::systems
{
class DynamicsSystem;
}

namespace lnd
{
struct LNDCountry;
}

class LandIsland
{
public:
	static const float HeightUnit;
	static const float CellSize;

	LandIsland();
	~LandIsland();

	void LoadFromFile(const std::string& filename);

	[[nodiscard]] float GetHeightAt(glm::vec2) const;
	[[nodiscard]] const LandBlock* GetBlock(const glm::u8vec2& coordinates) const;
	[[nodiscard]] const lnd::LNDCell& GetCell(const glm::u16vec2& coordinates) const;

	// Debug
	void DumpTextures() const;
	void DumpMaps() const;

private:
	std::array<uint8_t, 1024> _blockIndexLookup;
	std::vector<LandBlock> _landBlocks;
	std::vector<lnd::LNDCountry> _countries;

	// Renderer, Dynamics
public:
	[[nodiscard]] std::vector<LandBlock>& GetBlocks() { return _landBlocks; }
	[[nodiscard]] const std::vector<LandBlock>& GetBlocks() const { return _landBlocks; }
	[[nodiscard]] const std::vector<lnd::LNDCountry>& GetCountries() const { return _countries; }
	[[nodiscard]] const graphics::Texture2D& GetAlbedoArray() const { return *_materialArray; }
	[[nodiscard]] const graphics::Texture2D& GetBump() const { return *_textureBumpMap; }
	[[nodiscard]] const graphics::Texture2D& GetSmallBump() const { return *_textureSmallBump; }

	uint8_t GetNoise(int x, int y);
	graphics::Texture2D* GetSmallBumpMap() { return _textureSmallBump.get(); }

private:
	std::unique_ptr<graphics::Texture2D> _materialArray;
	std::unique_ptr<graphics::Texture2D> _countryLookup;

	std::unique_ptr<graphics::Texture2D> _textureNoiseMap;
	std::unique_ptr<graphics::Texture2D> _textureBumpMap;
	std::unique_ptr<graphics::Texture2D> _textureSmallBump;

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
