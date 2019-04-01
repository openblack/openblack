/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef OPENBLACK_LANDISLAND_H
#define OPENBLACK_LANDISLAND_H

#define OPENBLACK_LANDISLAND_HEIGHT_UNIT 0.67f

#include <string>
#include <vector>
#include <memory>

#include <Graphics/Mesh.h>
#include <Graphics/Texture2D.h>
#include <Graphics/Texture2DArray.h>

#include <3D/LandBlock.h>

using namespace OpenBlack::Graphics;

namespace OpenBlack
{
	struct MapMaterial
	{
		uint32_t FirstMaterialIndex;
		uint32_t SecondMaterialIndex;
		uint32_t Coeficient;
	};

	struct Country
	{
		uint32_t TerrainType;
		MapMaterial MapMaterials[256]; // altitude 0-255
	};

	class LandIsland
	{
	public:
		LandIsland();
		~LandIsland();

		void LoadFromDisk(std::string &filePath);

		const uint8_t GetAltitudeAt(glm::ivec2) const;
		const float GetHeightAt(glm::ivec2) const;

		// Debug
		void DumpTextures();
		void DumpMaps();
	private:
		std::unique_ptr<LandBlock[]> _landBlocks;
		std::unique_ptr<Country[]> _countries;

		unsigned int _blockCount;
		unsigned int _materialCount;
		unsigned int _countryCount;
		unsigned int _lowresCount;

		// Renderer
	public:
		void Draw(ShaderProgram* program);
		std::shared_ptr<Texture2DArray> GetMaterialArray() const { return _materialArray; }
		std::shared_ptr<Texture2DArray> GetLowResArray() const { return _lowResTextureArray; }
		Country* GetCountries() const { return _countries.get(); }

		std::shared_ptr<Texture2D> GetNoiseMap() { return _textureNoiseMap; }
		std::shared_ptr<Texture2D> GetBumpMap() { return _textureBumpMap; }
	private:
		void convertRGB5ToRGB8(uint16_t* rgba5, uint32_t* rgba8, size_t pixels);

		std::shared_ptr<Texture2DArray> _lowResTextureArray;
		std::shared_ptr<Texture2DArray> _materialArray;
		std::unique_ptr<Texture2D> _countryLookup;

		std::shared_ptr<Texture2D> _textureNoiseMap;
		std::shared_ptr<Texture2D> _textureBumpMap;
	};
}

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

#endif
