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

#include <algorithm>
#include <ranges>

#include <LNDFile.h>
#include <bgfx/bgfx.h>

#include "Graphics/VertexBuffer.h"

using namespace openblack;
using namespace openblack::graphics;

void LandBlock::BuildMesh(LandIslandInterface& island, std::span<LandVertex> vertices)
{
	assert(vertices.size() == k_VertexCount);
	BuildVertexList(vertices, island);

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
}

void LandBlock::SetVertices(const VertexBuffer& buffer, uint32_t firstVertex)
{
	_vertices = &buffer;
	_firstVertex = firstVertex;
}

void LandBlock::BindVertices() const
{
	if (_vertices != nullptr)
	{
		_vertices->Bind(_firstVertex, k_VertexCount);
	}
}

void LandBlock::BuildVertexList(std::span<LandVertex> vertices, LandIslandInterface& island)
{
	// The land is coloured by its block texture (see block_texture) and lit by its luminosity of the frame: its vertices
	// carry only where they are
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
			for (auto [position, cell, offset] : std::views::zip(pos, cells, offsets))
			{
				cell = &island.GetCell(blockOffset + offset);
				position =
				    glm::vec3(offset.x * LandIslandInterface::k_CellSize, LandIslandInterface::GetDrawnAltitude(cell->altitude),
				              offset.y * LandIslandInterface::k_CellSize);
			}

			auto makeVert = [&pos](Corner corner) -> LandVertex {
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				return {pos[static_cast<size_t>(corner)]};
			};

			auto makeTriangle = [&makeVert, &vertices, &index](const std::array<Corner, 3>& corners, bool forward) {
				if (forward)
				{
					vertices[index++] = makeVert(corners[0]);
					vertices[index++] = makeVert(corners[1]);
					vertices[index++] = makeVert(corners[2]);
				}
				else
				{
					vertices[index++] = makeVert(corners[2]);
					vertices[index++] = makeVert(corners[1]);
					vertices[index++] = makeVert(corners[0]);
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
