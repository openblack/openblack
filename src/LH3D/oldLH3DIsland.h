#ifndef LH3DISLAND_H
#define LH3DISLAND_H

/*
Porting this to LandIsland.h
*/

#include <stdint.h>
#include <vector>
#include <Graphics/Texture2D.h>
#include <Graphics/Texture2DArray.h>

using namespace OpenBlack::Graphics;

#pragma pack(push, 1)
struct LH3DVertex
{
	GLfloat position[3];
	GLubyte color[3];
	GLfloat uv[2];
	GLubyte country;
	GLubyte altitude;
};
#pragma pack(pop)

struct LH3DLandCell // 8 bytes
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t l;

	uint8_t altitude;
	uint8_t savecolor;

	// Cell splitting: Normal, Other (Triangle direction?)
	// Cell properties: Cell has water, Cell has coastline, Cell is full watercell
	// Sound properties: Has coastal sound, Has land sound, Has sea sound, Has freshwater sound
	// Country Style
	uint8_t flags1; // 0x0F = country 0x80 = split // & 16 = water & 32 = coast
	uint8_t flags2; //
};

struct LH3DLandBlock
{
	LH3DLandCell cells[289]; // 17*17
	uint32_t index; // 2312
	float mapY; // 2316
	float mapX; // 2320
	uint32_t blockY; // 2324
	uint32_t blockX; // 2328
	uint32_t clipped; // 2332 0
	uint32_t frameVisibility; // 0
	uint32_t highestAltitude; // 0
	uint32_t useSmallBump; // 0
	uint32_t forceLowResTex; // 0
	uint32_t meshLOD;// 0
	uint32_t meshBlending; // 0
	uint32_t textureBlend; // 0
	uint32_t meshLODType; // 0
	uint32_t fog; // 0
	uint32_t texPointer; // 0 if LH3DIsland::g_b_use_always_low_texture, else new Tex
	uint32_t matPointer; // 0 if LH3DIsland::g_b_use_always_low_texture, else new Mat(Tex)
	uint32_t drawSomething; // 0aaa
	uint32_t specMatBeforePtr; // 0
	uint32_t specMatAfterPtr; // 0
	float transformUVBefore[3][4];
	float transformUVAfter[3][4];
	uint32_t nextSortingPtr;
	float valueSorting;
	float lowResTexture;
	float fu_lrs; // (iu_lrs / 256)
	float fv_lrs; // (iv_lrs / 256)
	float iu_lrs; // lowrestex x
	float iv_lrs; // lowrestex y
	uint32_t smallTextUpdated;
};

struct LNDMapMaterial
{
	uint32_t FirstMaterialIndex;
	uint32_t SecondMaterialIndex;
	uint32_t Coeficient;
};

struct LNDCountry
{
	uint32_t TerrainType;
	LNDMapMaterial MapMaterial[256]; // altitude 0-255
};

class LH3DIsland {
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

};



#endif
