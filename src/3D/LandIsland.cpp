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
#include "Common/stb_image_write.h"
#include "Game.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"

#include <DetourNavMeshBuilder.h>
#include <Recast.h>
#include <spdlog/spdlog.h>

#include <lnd_file.h>
#include <stdexcept>

using namespace openblack;
using namespace openblack::graphics;

const float LandIsland::HeightUnit = 0.67f;
const float LandIsland::CellSize = 10.0f;

LandIsland::LandIsland()
    : _blockIndexLookup {0}
{
	auto file = Game::instance()->GetFileSystem().Open("Data/Textures/smallbumpa.raw", FileMode::Read);
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

void LandIsland::DumpTextures()
{
	_materialArray->DumpTexture();
}

void LandIsland::DumpMaps()
{
	const int cellsize = 16;

	// 32x32 block grid with 16x16 cells
	// 512 x 512 pixels
	// lets go with 3 channels for a laugh
	auto* data = new uint8_t[32 * 32 * cellsize * cellsize];

	memset(data, 0x00, 32 * 32 * cellsize * cellsize);

	for (unsigned int b = 0; b < _landBlocks.size(); b++)
	{
		LandBlock& block = _landBlocks[b];

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

void LandIsland::BuildNavMesh(float cellSize, float cellHeight)
{
	if (_landBlocks.empty() || _landBlocks[0].GetVertices().empty())
	{
		spdlog::error("No land blocks: Can't build navigation mesh without terrain");
		return;
	}
	// Step 1. Initialize build config.

	bool filterLowHangingObstacles = true;
	bool filterLedgeSpans = true;
	bool filterWalkableLowHeightSpans = true;
	float agentMaxSlope = 60;
	float agentHeight = 2.0f / 1000; // TODO: get living height, then creature height
	float agentMaxClimb = 6;
	float agentRadius = 0.6f; // TODO: get living height, then creature height
	float edgeMaxLen = 12;
	float regionMinSize = 8;
	float regionMergeSize = 20;
	float detailSampleDist = 6;
	float detailSampleMaxError = 1;
	int maxVertsPerPoly = 6;

	// create nav mesh,  TODO: Try tiles
	rcConfig config = {};
	config.cs = cellSize;
	config.ch = cellHeight;

	// Get bounds
	config.bmin[0] = std::numeric_limits<float>::max();
	config.bmin[1] = std::numeric_limits<float>::max();
	config.bmin[1] = std::numeric_limits<float>::max();
	config.bmax[0] = std::numeric_limits<float>::min();
	config.bmax[1] = std::numeric_limits<float>::min();
	config.bmax[1] = std::numeric_limits<float>::min();

	std::vector<glm::vec3> vertices;
	std::vector<int> indices;
	for (const auto& block : _landBlocks)
	{
		auto mapPosition = block.GetMapPosition();
		vertices.reserve(vertices.size() + block.GetVertices().size());
		indices.reserve(indices.size() + block.GetVertices().size());
		for (const auto& verts : block.GetVertices())
		{
			config.bmin[0] = std::min(verts.position[0] + mapPosition.x, config.bmin[0]);
			config.bmin[1] = std::min(verts.position[1], config.bmin[1]);
			config.bmin[2] = std::min(verts.position[2] + mapPosition.y, config.bmin[2]);
			config.bmax[0] = std::max(verts.position[0] + mapPosition.x, config.bmax[0]);
			config.bmax[1] = std::max(verts.position[1], config.bmax[1]);
			config.bmax[2] = std::max(verts.position[2] + mapPosition.y, config.bmax[2]);
			vertices.emplace_back(verts.position[0] + mapPosition.x, verts.position[1], verts.position[2] + mapPosition.y);
			indices.push_back(indices.size());
		}
	}
	// Get width and height
	rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

	config.walkableSlopeAngle = agentMaxSlope;
	config.walkableHeight = (int)ceilf(agentHeight / config.ch);
	config.walkableClimb = (int)floorf(agentMaxClimb / config.ch);
	config.walkableRadius = (int)ceilf(agentRadius / config.cs);
	config.maxEdgeLen = (int)(edgeMaxLen / config.cs);
	config.maxSimplificationError = 1.3f;                 // TODO
	config.minRegionArea = (int)rcSqr(regionMinSize);     // Note: area = size*size
	config.mergeRegionArea = (int)rcSqr(regionMergeSize); // Note: area = size*size
	config.maxVertsPerPoly = 6;                           // TODO
	config.detailSampleDist = detailSampleDist < 0.9f ? 0 : config.cs * detailSampleDist;
	config.detailSampleMaxError = config.ch * detailSampleMaxError;

	// Step 2. Rasterize input polygon soup.

	// Allocate voxel heightfield where we rasterize our input data to.
	auto solid = rcAllocHeightfield();
	assert(solid);

	rcContext context(false);
	if (!rcCreateHeightfield(&context, *solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
	{
		assert(false);
	}

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	auto triAreas = std::vector<unsigned char>(indices.size() / 3, 0);

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	rcMarkWalkableTriangles(&context, config.walkableSlopeAngle, reinterpret_cast<const float*>(vertices.data()),
	                        vertices.size(), indices.data(), indices.size() / 3, triAreas.data());
	if (!rcRasterizeTriangles(&context, reinterpret_cast<const float*>(vertices.data()), vertices.size(), indices.data(),
	                          triAreas.data(), indices.size() / 3, *solid, config.walkableClimb))
	{
		assert(false);
	}

	// Step 3. Filter walkables surfaces.
	if (filterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *solid);
	if (filterLedgeSpans)
		rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *solid);
	if (filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *solid);

	// Step 4. Partition walkable surface to simple regions.
	auto chf = rcAllocCompactHeightfield();
	assert(chf);
	if (!rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb, *solid, *chf))
	{
		assert(false);
	}
	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(&context, config.walkableRadius, *chf))
	{
		assert(false);
	}
	// Prepare for region partitioning, by calculating distance field along the walkable surface.
	if (!rcBuildDistanceField(&context, *chf))
	{
		assert(false);
	}

	// Partition the walkable surface into simple regions without holes.
	if (!rcBuildRegions(&context, *chf, 0, config.minRegionArea, config.mergeRegionArea))
	{
		assert(false);
	}

	// Step 5. Trace and simplify region contours.

	// Create contours.
	auto cset = rcAllocContourSet();
	if (!cset)
	{
		assert(false);
	}
	if (!rcBuildContours(&context, *chf, config.maxSimplificationError, config.maxEdgeLen, *cset))
	{
		assert(false);
	}

	// Step 6. Build polygons mesh from contours.

	// Build polygon navmesh from the contours.
	auto p_mesh = rcAllocPolyMesh();
	if (!p_mesh)
	{
		assert(false);
	}
	if (!rcBuildPolyMesh(&context, *cset, maxVertsPerPoly, *p_mesh))
	{
		assert(false);
	}

	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//

	auto d_mesh = rcAllocPolyMeshDetail();
	if (!d_mesh)
	{
		assert(false);
	}

	if (!rcBuildPolyMeshDetail(&context, *p_mesh, *chf, config.detailSampleDist, config.detailSampleMaxError, *d_mesh))
	{
		assert(false);
	}

	if (d_mesh->nverts == 0)
	{
		assert(false);
	}

	const bgfx::Memory* navMeshVerticesMem = bgfx::alloc(sizeof(float) * 3 * d_mesh->nverts);
	const bgfx::Memory* navMeshIndicesMem = bgfx::alloc(sizeof(uint32_t) * 3 * d_mesh->ntris);
	auto navMeshVertices = (float*)navMeshVerticesMem->data;
	auto navMeshIndices = (uint32_t*)navMeshIndicesMem->data;
	uint32_t current_vertex = 0;
	uint32_t current_tri = 0;

	for (int j = 0; j < d_mesh->nmeshes; ++j)
	{
		const unsigned int* m = &d_mesh->meshes[j * 4];
		const unsigned int bverts = m[0];
		const int nverts = (int)m[1];
		const unsigned int btris = m[2];
		const int ntris = (int)m[3];
		const float* verts = &d_mesh->verts[bverts * 3];
		const unsigned char* tris = &d_mesh->tris[btris * 4];

		for (int i = 0; i < nverts; ++i)
		{
			navMeshVertices[(current_vertex + i) * 3] = verts[i * 3];
			navMeshVertices[(current_vertex + i) * 3 + 1] = verts[i * 3 + 1];
			navMeshVertices[(current_vertex + i) * 3 + 2] = verts[i * 3 + 2];
		}

		for (int i = 0; i < ntris; ++i)
		{
			navMeshIndices[(current_tri + i) * 3] = current_vertex + tris[i * 4];
			navMeshIndices[(current_tri + i) * 3 + 1] = current_vertex + tris[i * 4 + 1];
			navMeshIndices[(current_tri + i) * 3 + 2] = current_vertex + tris[i * 4 + 2];
		}

		current_vertex += nverts;
		current_tri += ntris;
	}

	spdlog::info("{} nverts {} tris {} indices", current_vertex, current_tri, current_tri * 3);
	assert(d_mesh->nverts == current_vertex);
	assert(d_mesh->ntris == current_tri);

	VertexDecl navMeshDecl;
	navMeshDecl.reserve(1);
	navMeshDecl.emplace_back(VertexAttrib::Attribute::Position, 3, VertexAttrib::Type::Float);

	auto navMeshVertexBuffer = new VertexBuffer("Navigation Mesh Vertices", navMeshVerticesMem, navMeshDecl);
	auto navMeshIndexBuffer = new IndexBuffer("Navigation Mesh Indices", navMeshIndicesMem, IndexBuffer::Type::Uint32);
	_navMesh = std::make_unique<Mesh>(navMeshVertexBuffer, navMeshIndexBuffer);

	rcFreeHeightField(solid);
	rcFreeCompactHeightfield(chf);
	rcFreeContourSet(cset);
	rcFreePolyMeshDetail(d_mesh);
	rcFreePolyMesh(p_mesh);
}
