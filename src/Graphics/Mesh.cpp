/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Mesh.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"

using namespace openblack::graphics;

Mesh::Mesh(VertexBufferUniquePtr&& vertexBuffer, IndexBufferUniquePtr&& indexBuffer, Topology topology) noexcept
    : _vertexBuffer(std::move(vertexBuffer))
    , _indexBuffer(std::move(indexBuffer))
    , _topology(topology)
{
}

Mesh::~Mesh() noexcept = default;

const VertexBuffer& Mesh::GetVertexBuffer() const
{
	return *_vertexBuffer;
}

const IndexBuffer& Mesh::GetIndexBuffer() const
{
	return *_indexBuffer;
}

bool Mesh::HasVertexBuffer() const
{
	return _vertexBuffer != nullptr;
}

bool Mesh::IsIndexed() const
{
	return _indexBuffer != nullptr && _indexBuffer->count > 0;
}

Mesh::Topology Mesh::GetTopology() const noexcept
{
	return _topology;
}
