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

#include <memory>

#include "Graphics/IndexBuffer.h"
#include "VertexBuffer.h"

namespace openblack::graphics
{

struct IndexBuffer;

class Mesh
{
public:
	enum class Topology
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
	};

	explicit Mesh(VertexBufferUniquePtr&& vertexBuffer, IndexBufferUniquePtr&& indexBuffer = nullptr,
	              Topology topology = Topology::TriangleList) noexcept;
	~Mesh() noexcept;

	[[nodiscard]] const VertexBuffer& GetVertexBuffer() const;
	[[nodiscard]] const IndexBuffer& GetIndexBuffer() const;
	[[nodiscard]] bool HasVertexBuffer() const;
	[[nodiscard]] bool IsIndexed() const;

	[[nodiscard]] Topology GetTopology() const noexcept;

private:
	VertexBufferUniquePtr _vertexBuffer;
	IndexBufferUniquePtr _indexBuffer;
	Topology _topology;
};

} // namespace openblack::graphics
