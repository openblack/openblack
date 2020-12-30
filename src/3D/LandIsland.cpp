/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LandIsland.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Common/FileSystem.h"
#include "Common/IStream.h"
#include "Common/stb_image_write.h"
#include "Game.h"
#include "Graphics/ShaderManager.h"
#include "Renderer.h"

#include <spdlog/spdlog.h>

#include <LNDFile.h>
#include <stdexcept>

using namespace openblack;
using namespace openblack::graphics;

struct PosTexCoord0Vertex
{
	float m_x;
	float m_y;
	float m_z;
	float m_u;
	float m_v;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosTexCoord0Vertex::ms_layout;

void screenSpaceQuad()
{
	bgfx::TransientVertexBuffer vb;
	bgfx::TransientIndexBuffer ib;

	bgfx::allocTransientVertexBuffer(&vb, 4, PosTexCoord0Vertex::ms_layout);
	bgfx::allocTransientIndexBuffer(&ib, 6);

	PosTexCoord0Vertex* vertex = (PosTexCoord0Vertex*)vb.data;
	vertex[0] = { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f }; // TL
	vertex[1] = { 1.0f, -1.0f, 0.0f, 1.0f, 0.0f }; // TR
	vertex[2] = { 1.0f, 1.0f, 0.0f, 1.0f, 1.0f }; // BR
	vertex[3] = { -1.0f, 1.0f, 0.0f, 0.0f, 1.0f }; // BL

	uint16_t* data = (uint16_t*)ib.data;
	data[0] = 0; data[1] = 1; data[2] = 2;
	data[3] = 0; data[4] = 3; data[5] = 2;

	bgfx::setVertexBuffer(0, &vb);
	bgfx::setIndexBuffer(&ib);
}

const float LandIsland::HeightUnit = 0.67f;
const float LandIsland::CellSize = 10.0f;

LandIsland::LandIsland()
    : _blockIndexLookup {0}
{
	auto& filesystem = Game::instance()->GetFileSystem();
	auto file = filesystem.Open(filesystem.TexturePath() / "smallbumpa.raw", FileMode::Read);
	auto* smallbumpa = new uint8_t[file->Size()];
	file->Read(smallbumpa, file->Size());

	_textureSmallBump = std::make_unique<Texture2D>("LandIslandSmallBump");
	_textureSmallBump->Create(256, 256, 1, Format::R8, Wrapping::Repeat, smallbumpa, file->Size());
	delete[] smallbumpa;
}

LandIsland::~LandIsland() = default;

void LandIsland::LoadFromFile(const std::string& filename)
{
	spdlog::debug("Loading Land from file: {}", filename);
	lnd::LNDFile lnd;

	try
	{
		lnd.Open(filename);
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open lnd file from filesystem {}: {}", filename, err.what());
		return;
	}

	std::memcpy(_blockIndexLookup.data(), lnd.GetHeader().lookUpTable, _blockIndexLookup.size() * sizeof(_blockIndexLookup[0]));

	auto& lndBlocks = lnd.GetBlocks();
	spdlog::debug("[LandIsland] loading {} blocks", lndBlocks.size());
	_landBlocks.resize(lndBlocks.size());
	for (size_t i = 0; i < _landBlocks.size(); i++)
	{
		_landBlocks[i]._block = std::make_unique<lnd::LNDBlock>(lndBlocks[i]);
	}

	spdlog::debug("[LandIsland] loading {} countries", lnd.GetCountries().size());
	_countries = lnd.GetCountries();

	auto materialCount = lnd.GetMaterials().size();
	spdlog::debug("[LandIsland] loading {} textures", materialCount);
	std::vector<uint16_t> rgba5TextureData;
	rgba5TextureData.resize(lnd::LNDMaterial::width * lnd::LNDMaterial::height * lnd.GetMaterials().size());
	for (size_t i = 0; i < lnd.GetMaterials().size(); i++)
	{
		std::memcpy(&rgba5TextureData[lnd::LNDMaterial::width * lnd::LNDMaterial::height * i], lnd.GetMaterials()[i].texels,
		            sizeof(lnd.GetMaterials()[i].texels));
	}
	_materialArray = std::make_unique<Texture2D>("LandIslandMaterialArray");
	_materialArray->Create(lnd::LNDMaterial::width, lnd::LNDMaterial::height, materialCount, Format::RGB5A1,
	                       Wrapping::ClampEdge, rgba5TextureData.data(), rgba5TextureData.size() * sizeof(rgba5TextureData[0]));

	// read noise map into Texture2D
	std::memcpy(_noiseMap.data(), lnd.GetExtra().noise.texels, _noiseMap.size() * sizeof(_noiseMap[0]));
	_textureNoiseMap = std::make_unique<Texture2D>("LandIslandNoiseMap");
	_textureNoiseMap->Create(lnd::LNDBumpMap::width, lnd::LNDBumpMap::height, 1, Format::R8, Wrapping::ClampEdge,
	                         _noiseMap.data(), _noiseMap.size() * sizeof(_noiseMap[0]));

	// read bump map into Texture2D
	_textureBumpMap = std::make_unique<Texture2D>("LandIslandBumpMap");
	_textureBumpMap->Create(lnd::LNDBumpMap::width, lnd::LNDBumpMap::height, 1, Format::R8, Wrapping::ClampEdge,
	                        lnd.GetExtra().bump.texels, sizeof(lnd.GetExtra().bump.texels));

	// generate a country lookup texture
	// [country_id, altitude] = [tex 1, tex 2, blend]

	auto* countryMap = new uint8_t[256 * 256 * 3];
	for (int i = 0; i < 256 * 256 * 3; i++) {
		countryMap[i] = 0x00;
	}

	for (int x = 0; x < _countries.size(); x++) {
		auto const& country = _countries[x];
		for (int y = 0; y < 256; y++) {
			auto const& material = country.materials[y];

			countryMap[(y * 256 * 3) + (x * 3) + 0] = material.indices[0];
			countryMap[(y * 256 * 3) + (x * 3) + 1] = material.indices[1];
			countryMap[(y * 256 * 3) + (x * 3) + 2] = material.coefficient;
		}
	}

	auto* blankMemory = new uint8_t[256 * 256 * 3];
	for (int i = 0; i < 256 * 256 * 3; i++) {
		blankMemory[i] = 0x00;
	}

	_textureCountryMap = std::make_unique<Texture2D>("LandIslandCountryMap");
	_textureCountryMap->Create(256, 256, 1, Format::RGB8UI, Wrapping::Repeat, countryMap, 256*256*3);
	// _textureCountryMap->DumpTexture();

	_textureCountryTest = std::make_unique<Texture2D>("LandIslandCountryTest");
	_textureCountryTest->Create(256, 256, 1, Format::RGB8, Wrapping::ClampEdge, blankMemory, 256*256*3);

	auto fb = bgfx::createFrameBuffer(1, &_textureCountryTest->GetNativeHandle());

	// RENDER COUNTRIES
	bgfx::ViewId viewTexture = 9;
	bgfx::setViewName(viewTexture, "Render Texture");
	bgfx::setViewClear(viewTexture, BGFX_CLEAR_COLOR, 0x300000ff, 1.0f, 0);
	bgfx::setViewRect(viewTexture, 0, 0, 256, 256);
	bgfx::setViewFrameBuffer(viewTexture, fb);

	bgfx::touch(viewTexture);

	auto& shaderManager = Game::instance()->GetRenderer().GetShaderManager();
	auto objectShader = shaderManager.GetShader("DebugCountry2D");
	objectShader->SetTextureSampler("s0_materials", 0, *_materialArray);
	objectShader->SetTextureSampler("s1_countryMap", 1, *_textureCountryMap);
	//terrainShader->SetUniformValue("u_timeOfDay", &desc.timeOfDay);

	PosTexCoord0Vertex::init();
	screenSpaceQuad();

	// Set render states.
	bgfx::setState(0
		| BGFX_STATE_DEFAULT
		| BGFX_STATE_PT_TRISTRIP
	);
	bgfx::submit(viewTexture, objectShader->GetRawHandle());

	bgfx::frame();

	// _textureCountryTest->DumpTexture();

	// build the meshes (we could move this elsewhere)
	for (auto& block : _landBlocks) block.BuildMesh(*this);
	bgfx::frame();
}

float LandIsland::GetHeightAt(glm::vec2 vec) const
{
	return GetCell(vec * 0.1f).altitude * LandIsland::HeightUnit;
}

uint8_t LandIsland::GetNoise(int x, int y)
{
	return _noiseMap[(y & 0xFF) + 256 * (x & 0xFF)];
}

const LandBlock* LandIsland::GetBlock(const glm::u8vec2& coordinates) const
{
	// our blocks can only be between [0-31, 0-31]
	if (coordinates.x > 32 || coordinates.y > 32)
		return nullptr;

	const uint8_t blockIndex = _blockIndexLookup[coordinates.x * 32 + coordinates.y];
	if (blockIndex == 0)
		return nullptr;

	return &_landBlocks[blockIndex - 1];
}

constexpr lnd::LNDCell EmptyCell() noexcept
{
	lnd::LNDCell cell {};
	cell.properties.fullWater = true;
	return cell;
}

constexpr lnd::LNDCell s_EmptyCell = EmptyCell();

const lnd::LNDCell& LandIsland::GetCell(const glm::u16vec2& coordinates) const
{
	if (coordinates.x > 511 || coordinates.y > 511)
	{
		return s_EmptyCell;
	}

	const uint16_t lookupIndex = ((coordinates.x & ~0xFU) << 1U) | (coordinates.y >> 4U);
	const uint16_t cellIndex = (coordinates.x & 0xFU) * 0x11u + (coordinates.y & 0xFU);

	const uint8_t blockIndex = _blockIndexLookup[lookupIndex];

	if (blockIndex == 0)
	{
		return s_EmptyCell;
	}
	assert(_landBlocks.size() >= blockIndex);
	return _landBlocks[blockIndex - 1].GetCells()[cellIndex];
}

void LandIsland::DumpTextures() const
{
	_materialArray->DumpTexture();
}

void LandIsland::DumpMaps() const
{
	const int cellsize = 16;

	// 32x32 block grid with 16x16 cells
	// 512 x 512 pixels
	// lets go with 3 channels for a laugh
	auto* data = new uint8_t[32 * 32 * cellsize * cellsize];

	memset(data, 0x00, 32 * 32 * cellsize * cellsize);

	for (unsigned int b = 0; b < _landBlocks.size(); b++)
	{
		const LandBlock& block = _landBlocks[b];

		int mapx = block.GetBlockPosition().x;
		int mapz = block.GetBlockPosition().y;
		int lineStride = 32 * cellsize;

		for (int x = 0; x < cellsize; x++)
		{
			for (int y = 0; y < cellsize; y++)
			{
				auto cell = block.GetCells()[y * 17 + x];

				int cellX = (mapx * cellsize) + x;
				int cellY = (mapz * cellsize) + y;

				data[(cellY * lineStride) + cellX] = cell.altitude;
			}
		}
	}

	FILE* fptr = fopen("dump.raw", "wb");
	fwrite(data, 32 * 32 * cellsize * cellsize, 1, fptr);
	fclose(fptr);

	delete[] data;
}
