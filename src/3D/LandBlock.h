/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <span>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "LandIslandInterface.h"

namespace openblack
{

namespace lnd
{
struct LNDBlock;
struct LNDCell;
} // namespace lnd

namespace graphics
{
class VertexBuffer;
}

struct LandVertex
{
	glm::vec3 position;
};

class LandIslandInterface;

class LandBlock
{
public:
	// 16*16 quads of 2 tris with 3 verts
	static constexpr auto k_Resolution = glm::u8vec2(16, 16);
	static constexpr uint16_t k_VertexCount = k_Resolution.x * k_Resolution.y * 2 * 3;

	LandBlock() = default;
	/// Fills the block's k_VertexCount vertices
	void BuildMesh(LandIslandInterface& island, std::span<LandVertex> vertices);
	/// Where the block's vertices are on the GPU: a run of the vertex buffer the island's blocks share, so a land of a
	/// thousand blocks takes one of bgfx's buffers rather than a thousand
	void SetVertices(const graphics::VertexBuffer& buffer, uint32_t firstVertex);
	/// Binds the block's vertices for a draw
	void BindVertices() const;
	[[nodiscard]] const lnd::LNDCell* GetCells() const;
	[[nodiscard]] glm::ivec2 GetBlockPosition() const;
	[[nodiscard]] glm::vec2 GetMapPosition() const;
	[[nodiscard]] const std::unique_ptr<lnd::LNDBlock>& GetLndBlock() const { return _block; };
	void SetLndBlock(const lnd::LNDBlock& block);

private:
	std::unique_ptr<lnd::LNDBlock> _block;
	const graphics::VertexBuffer* _vertices {nullptr};
	uint32_t _firstVertex {0};

	void BuildVertexList(std::span<LandVertex> vertices, LandIslandInterface& island);
};
} // namespace openblack
