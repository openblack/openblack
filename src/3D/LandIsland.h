/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

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

	void Update(float timeOfDay, float bumpMapStrength, float smallBumpMapStrength);

	// const uint8_t GetAltitudeAt(glm::ivec2) const;

	[[nodiscard]] float GetHeightAt(glm::vec2) const;
	[[nodiscard]] const LandBlock* GetBlock(const glm::u8vec2& coordinates) const;
	[[nodiscard]] const lnd::LNDCell& GetCell(const glm::u16vec2& coordinates) const;

	// Debug
	void DumpTextures();
	void DumpMaps();

private:
	std::array<uint8_t, 1024> _blockIndexLookup;
	std::vector<LandBlock> _landBlocks;
	std::vector<lnd::LNDCountry> _countries;

	// Renderer
public:
	void Draw(graphics::RenderPass viewId, const graphics::ShaderProgram& program, bool cullBack) const;

	[[nodiscard]] const std::vector<LandBlock>& GetBlocks() const { return _landBlocks; }
	[[nodiscard]] const std::vector<lnd::LNDCountry>& GetCountries() const { return _countries; }

	uint8_t GetNoise(int x, int y);
	graphics::Texture2D* GetSmallBumpMap() { return _textureSmallBump.get(); }

private:
	std::unique_ptr<graphics::Texture2D> _materialArray;
	std::unique_ptr<graphics::Texture2D> _countryLookup;

	std::unique_ptr<graphics::Texture2D> _textureNoiseMap;
	std::unique_ptr<graphics::Texture2D> _textureBumpMap;
	std::unique_ptr<graphics::Texture2D> _textureSmallBump;

	std::array<uint8_t, 256 * 256> _noiseMap;

	float _timeOfDay;
	float _bumpMapStrength;
	float _smallBumpMapStrength;
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
