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
		MapMaterial MapMaterial[256]; // altitude 0-255
	};

	#pragma pack(push, 1)
	struct LandVertex
	{
		GLfloat position[3];
		GLfloat textureCoords[2];
		GLfloat weight[3];
		GLubyte firstMaterialID[4];
		GLubyte secondMaterialID[4];
		GLubyte materialBlendCoefficient[4];
		GLfloat alpha;

		LandVertex(glm::vec3 _position, glm::vec2 _uv, glm::vec3 _weight,
			GLubyte mat1, GLubyte mat2, GLubyte mat3,
			GLubyte mat4, GLubyte mat5, GLubyte mat6,
			GLubyte blend1, GLubyte blend2, GLubyte blend3,
			GLfloat _alpha)
		{
			position[0] = _position.x; position[1] = _position.y; position[2] = _position.z;
			textureCoords[0] = _uv.x; textureCoords[1] = _uv.y;
			weight[0] = _weight.x; weight[1] = _weight.y; weight[2] = _weight.z;
			firstMaterialID[0] = mat1; firstMaterialID[1] = mat2; firstMaterialID[2] = mat3;
			secondMaterialID[0] = mat4; secondMaterialID[1] = mat5; secondMaterialID[2] = mat6;
			materialBlendCoefficient[0] = blend1; materialBlendCoefficient[1] = blend2; materialBlendCoefficient[2] = blend3;
			alpha = _alpha;
		}
	};

	#pragma pack(pop)



	class LandIsland
	{
	public:
		LandIsland();
		~LandIsland();

		void LoadFromDisk(std::string filePath);

		const uint8_t GetAltitudeAt(glm::ivec2) const;
		const float GetHeightAt(glm::ivec2) const;

		// Debug
		void DumpTextures();
	private:
		std::unique_ptr<LandBlock[]> _landBlocks;
		std::unique_ptr<Country[]> _countries;

		unsigned int _blockCount;
		unsigned int _materialCount;
		unsigned int _countryCount;
		unsigned int _lowresCount;

		// Renderer
	public:
		void Draw();
		std::shared_ptr<Texture2DArray> GetMaterialArray() const { return _materialArray; }
		std::shared_ptr<Texture2DArray> GetLowResArray() const { return _lowResTextureArray; }

		std::shared_ptr<Texture2D> GetNoiseMap() { return _textureNoiseMap; }
		std::shared_ptr<Texture2D> GetBumpMap() { return _textureBumpMap; }
	private:
		void convertRGB5ToRGB8(uint16_t* rgba5, uint32_t* rgba8, size_t pixels);
		void buildMesh();
		std::vector<LandVertex> buildVertexList();

		std::unique_ptr<Mesh> _mesh;
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
