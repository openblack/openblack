#pragma once
#ifndef OPENBLACK_LANDISLAND_H
#define OPENBLACK_LANDISLAND_H

#define OPENBLACK_LANDISLAND_HEIGHT_UNIT 0.67f;

#include <string>

#include <Graphics/Texture2D.h>
#include <Graphics/Texture2DArray.h>
#include <3D/LandBlock.h>

using namespace OpenBlack::Graphics;

namespace OpenBlack
{
	class LandIsland
	{
	public:
		LandIsland();
		~LandIsland();

		void LoadFromDisk(std::string filePath);

		void Draw();

		const int GetAltitudeAt(glm::ivec2) const;
		const float GetHeightAt(glm::ivec2) const;
	private:
		void AddTexture(uint16_t* data);
		void CreateCountryLookupTexture();

		std::unique_ptr<Texture2DArray> m_LowResTextureArray;
		std::unique_ptr<Texture2DArray> m_MaterialArray;
		std::unique_ptr<Texture2D> m_CountryLookup;
		std::unique_ptr<LandBlock*[]> m_LandBlocks;
	};
}

/*class LH3DIsland {
public:
	static void CreateCommonPart();
	static void LoadFromDisk(const char* filename);
	static void Release();
	static std::vector<LH3DVertex> GetVerticies();
	static std::vector<uint32_t> GetIndices();

	static bool g_b_created;
	static bool g_b_need_rebuild_all;
	static bool g_b_use_small_bump;
	static uint8_t g_index_block[1024];
	static LH3DLandBlock** g_blocks;
	//static LandBlock TC_g_index_block[1024];

	static uint32_t g_block_count;
	static uint32_t g_material_count;
	static uint32_t g_country_count;
	static uint32_t g_lowrestexture_count;

	static Texture2DArray* g_materialarray;
	static Texture2D* g_materials;
	static LNDCountry* g_countries;
	static uint32_t* g_lowrestextures;

	static const float g_height_unit;

};*/

#endif
