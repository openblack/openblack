/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LandBlock.h"

#include <cassert>

#include <ranges>

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LNDFile.h>
#include <bgfx/bgfx.h>

#include "Dynamics/LandBlockBulletMeshInterface.h"
#include "Graphics/Mesh.h"
#include "Graphics/VertexBuffer.h"

using namespace openblack;
using namespace openblack::graphics;

LandVertex::LandVertex(const glm::vec3& position, const glm::vec3& weight, const std::array<uint32_t, 6>& mat,
                       const glm::uvec3& blend, uint8_t lightLevel, float alpha)
    : position {position}
    , weight {weight}
    , firstMaterialID {static_cast<uint8_t>(mat[0]), static_cast<uint8_t>(mat[1]), static_cast<uint8_t>(mat[2]), 0u}
    , secondMaterialID {static_cast<uint8_t>(mat[3]), static_cast<uint8_t>(mat[4]), static_cast<uint8_t>(mat[5]), 0u}
    , materialBlendCoefficient {blend, 0u}
    , lightLevel {lightLevel}
    , waterAlpha {alpha}
{
}

void LandBlock::BuildMesh(LandIslandInterface& island)
{
	if (_mesh != nullptr)
	{
		_mesh.reset();
	}

	VertexDecl decl;
	decl.reserve(7);
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	// weight
	decl.emplace_back(VertexAttrib::Attribute::TexCoord1, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	// first material id
	decl.emplace_back(VertexAttrib::Attribute::Color1, static_cast<uint8_t>(3), VertexAttrib::Type::Uint8);
	// second material id
	decl.emplace_back(VertexAttrib::Attribute::Color2, static_cast<uint8_t>(3), VertexAttrib::Type::Uint8);
	// material blend coefficient
	decl.emplace_back(VertexAttrib::Attribute::TexCoord2, static_cast<uint8_t>(3), VertexAttrib::Type::Uint8, true);
	// light level, align to 4 bytes
	decl.emplace_back(VertexAttrib::Attribute::Color0, static_cast<uint8_t>(4), VertexAttrib::Type::Uint8, true);
	// water alpha
	decl.emplace_back(VertexAttrib::Attribute::Color3, static_cast<uint8_t>(1), VertexAttrib::Type::Float, true);

	// reserve 16*16 quads of 2 tris with 3 verts = 1536
	const bgfx::Memory* verticesMem = bgfx::alloc(sizeof(LandVertex) * k_VertexCount);
	auto vertices = std::span(reinterpret_cast<LandVertex*>(verticesMem->data), k_VertexCount);

	BuildVertexList(vertices, island);

	// The physics shape keeps every cell: it copies the positions before the open sea's are taken away
	_dynamicsMeshInterface = std::make_unique<dynamics::LandBlockBulletMeshInterface>(vertices);

	// The game draws no land in open sea cells, so only the sea shows there and nothing else is hidden behind them. Their
	// six vertices collapse to a point, which draws nothing.
	constexpr uint8_t k_OpenSeaFlag = 0x02;
	constexpr size_t k_VerticesPerCell = 6;
	for (size_t cell = 0; cell < static_cast<size_t>(k_Resolution.x) * k_Resolution.y; ++cell)
	{
		const size_t x = cell / k_Resolution.y;
		const size_t z = cell % k_Resolution.y;
		if ((_block->cells.at(x * 17 + z).flags & k_OpenSeaFlag) != 0)
		{
			const auto cellVertices = vertices.subspan(cell * k_VerticesPerCell, k_VerticesPerCell);
			const auto point = cellVertices.front().position;
			for (auto& vertex : cellVertices)
			{
				vertex.position = point;
			}
		}
	}

	auto* vertexBuffer = new VertexBuffer("LandBlock", verticesMem, decl);
	_mesh = std::make_unique<Mesh>(vertexBuffer);

	_physicsMesh = std::make_unique<btBvhTriangleMeshShape>(_dynamicsMeshInterface.get(), true);
	_rigidBody = std::make_unique<btRigidBody>(0.0f, nullptr, _physicsMesh.get());
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_block->mapX, 0, _block->mapZ));
	_rigidBody->setWorldTransform(transform);
	_rigidBody->setContactStiffnessAndDamping(300, 10);
	_rigidBody->setUserIndex(-1);
}

void LandBlock::BuildVertexList(std::span<LandVertex> vertices, LandIslandInterface& island)
{
	auto countries = island.GetCountries();

	// auto neighbourBlockR = island.GetBlock(glm::u8vec2(_block->blockX + 1, _block->blockZ));
	// auto neighbourBlockUp = island.GetBlock(glm::u8vec2(_block->blockX, _block->blockZ + 1));

	// we'll loop through each cell, 16x16
	// (the array is 17x17 but the 17th block is questionable data)

	const auto blockOffset = static_cast<glm::u16vec2>(GetBlockPosition() * 16);

	uint16_t index = 0;
	for (int x = 0; x < 16; x++)
	{
		for (int z = 0; z < 16; z++)
		{
			enum class Corner
			{
				TopLeft,
				TopRight,
				BottomLeft,
				BottomRight,

				_COUNT
			};

			std::array<glm::u16vec2, static_cast<size_t>(Corner::_COUNT)> offsets;
			offsets[static_cast<size_t>(Corner::TopLeft)] = glm::u16vec2(x, z);
			offsets[static_cast<size_t>(Corner::TopRight)] = glm::u16vec2(x + 1, z);
			offsets[static_cast<size_t>(Corner::BottomLeft)] = glm::u16vec2(x, z + 1);
			offsets[static_cast<size_t>(Corner::BottomRight)] = glm::u16vec2(x + 1, z + 1);

			std::array<const lnd::LNDCell*, static_cast<size_t>(Corner::_COUNT)> cells;
			// construct positions from cell altitudes
			std::array<glm::vec3, static_cast<size_t>(Corner::_COUNT)> pos;
			std::array<const lnd::LNDMapMaterial*, static_cast<size_t>(Corner::_COUNT)> materials;
			for (auto [position, cell, material, offset] : std::views::zip(pos, cells, materials, offsets))
			{
				cell = &island.GetCell(blockOffset + offset);
				position =
				    glm::vec3(offset.x * LandIslandInterface::k_CellSize, LandIslandInterface::GetDrawnAltitude(cell->altitude),
				              offset.y * LandIslandInterface::k_CellSize);

				const auto& country = countries.at(cell->properties.country);
				const auto noise = island.GetNoise(blockOffset + offset);

				material = &country.materials.at((cell->altitude + noise) % country.materials.size());
			}

			// TODO(470): This is temporary way for drawing landscape, should be moved to a shader in the renderer
			// Using a lambda so we're not repeating ourselves
			auto getAlpha = [](lnd::LNDCell::Properties properties) {
				if (properties.hasWater || properties.fullWater)
				{
					return 0.0f;
				}
				if (properties.coastLine)
				{
					return 0.5f;
				}
				return 1.0f;
			};
			auto makeVert = [&getAlpha, &pos, &cells, &materials](Corner corner, const glm::vec3& weight,
			                                                      const std::array<Corner, 3>& m) -> LandVertex {
				const std::array<uint32_t, 6> mat = {
				    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				    materials[static_cast<size_t>(m[0])]->indices[0],
				    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				    materials[static_cast<size_t>(m[1])]->indices[0],
				    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				    materials[static_cast<size_t>(m[2])]->indices[0],

				    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				    materials[static_cast<size_t>(m[0])]->indices[1],
				    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				    materials[static_cast<size_t>(m[1])]->indices[1],
				    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				    materials[static_cast<size_t>(m[2])]->indices[1],
				};
				const glm::u32vec3 blend = {
				    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				    materials[static_cast<size_t>(m[0])]->coefficient,
				    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				    materials[static_cast<size_t>(m[1])]->coefficient,
				    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				    materials[static_cast<size_t>(m[2])]->coefficient,
				};
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				const auto& cell = *cells[static_cast<size_t>(corner)];
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				return {pos[static_cast<size_t>(corner)], weight, mat, blend, cell.luminosity, getAlpha(cell.properties)};
			};

			auto makeTriangle = [&makeVert, &vertices, &index](const std::array<Corner, 3>& corners, bool forward) {
				if (forward)
				{
					vertices[index++] = makeVert(corners[0], glm::vec3(1, 0, 0), corners);
					vertices[index++] = makeVert(corners[1], glm::vec3(0, 1, 0), corners);
					vertices[index++] = makeVert(corners[2], glm::vec3(0, 0, 1), corners);
				}
				else
				{
					vertices[index++] = makeVert(corners[2], glm::vec3(0, 0, 1), corners);
					vertices[index++] = makeVert(corners[1], glm::vec3(0, 1, 0), corners);
					vertices[index++] = makeVert(corners[0], glm::vec3(1, 0, 0), corners);
				}
			};

			// cell splitting
			// winding order = clockwise
			if (!cells[static_cast<size_t>(Corner::TopLeft)]->properties.split)
			{
				makeTriangle({Corner::TopLeft, Corner::TopRight, Corner::BottomRight}, true);    //  ┐
				makeTriangle({Corner::TopLeft, Corner::BottomLeft, Corner::BottomRight}, false); // └
			}
			else
			{
				makeTriangle({Corner::BottomLeft, Corner::TopLeft, Corner::TopRight}, true);      // ┌
				makeTriangle({Corner::BottomLeft, Corner::BottomRight, Corner::TopRight}, false); //  ┘
			}
		}
	}
}

const lnd::LNDCell* LandBlock::GetCells() const
{
	assert(_block);
	return _block ? _block->cells.data() : nullptr;
}

glm::ivec2 LandBlock::GetBlockPosition() const
{
	assert(_block);
	return {_block ? _block->blockX : -1, _block ? _block->blockZ : -1};
}

glm::vec2 LandBlock::GetMapPosition() const
{
	assert(_block);
	return {_block->mapX, _block->mapZ};
}

void LandBlock::SetLndBlock(const lnd::LNDBlock& block)
{
	_block = std::make_unique<lnd::LNDBlock>(block);
}
