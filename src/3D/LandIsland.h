#pragma once
#ifndef OPENBLACK_LANDISLAND_H
#define OPENBLACK_LANDISLAND_H

#define OPENBLACK_LANDISLAND_HEIGHT_UNIT 0.67f;
// #define OPENBLACK_LANDISLAND_HEIGHT_UNIT 1.0f;

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
	#pragma pack(push, 1)
	struct LandVertex
	{
		GLfloat position[3];
		GLubyte color[3];
		GLfloat uv[2];
		GLubyte country;
		GLubyte altitude;
	};
	#pragma pack(pop)

	class LandIsland
	{
	public:
		LandIsland();
		~LandIsland();

		void LoadFromDisk(std::string filePath);

		void Draw();

		const uint8_t GetAltitudeAt(glm::ivec2) const;
		const float GetHeightAt(glm::ivec2) const;

		void DumpTextures();


		// AdjustAlti
		// BuildSoundTGA(char*)
		// CreateSmallBump()
		// GetAltitude(LH3DMapCoords)
		// GetCell(long, long)
		// GetCountry(long, long, uchar &)
		// GetIndex(long, long)
		// GetTerrainMaterial(ulong, ulong)
		// SetColor(long, long, uchar)
		// SetCountry(long, long long)
		// SetHeightAsByte(long, long, long)

	private:
		void addTexture(uint16_t* data);
		void buildMesh();
		std::vector<LandVertex> getVerticies();
		std::vector<uint16_t> getIndices();
		void createCountryLookupTexture();

		Mesh* m_Mesh;

		std::unique_ptr<Texture2DArray> m_LowResTextureArray;
		std::unique_ptr<Texture2DArray> m_MaterialArray;
		std::unique_ptr<Texture2D> m_CountryLookup;
		std::unique_ptr<LandBlock[]> m_LandBlocks;

		unsigned int m_blockCount;
		unsigned int m_materialCount;
		unsigned int m_countryCount;
		unsigned int m_lowresCount;
	};
}

#endif
