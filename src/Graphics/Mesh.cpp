/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Mesh.h"

#include "IndexBuffer.h"
#include "ShaderProgram.h"
#include "VertexBuffer.h"

using namespace openblack::graphics;

Mesh::Mesh(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer, Topology topology)
    : _vertexBuffer(vertexBuffer)
    , _indexBuffer(indexBuffer)
    , _topology(topology)
{
}

Mesh::~Mesh() = default;

const VertexBuffer& Mesh::GetVertexBuffer() const
{
	return *_vertexBuffer;
}

const IndexBuffer& Mesh::GetIndexBuffer() const
{
	return *_indexBuffer;
}

bool Mesh::isIndexed() const
{
	return _indexBuffer != nullptr && _indexBuffer->GetCount() > 0;
}

Mesh::Topology Mesh::GetTopology() const noexcept
{
	return _topology;
}

void Mesh::Draw(const DrawDesc& desc) const
{
	if (desc.instanceBuffer && (desc.skip & SkipState::SkipInstanceBuffer) == 0)
	{
		bgfx::setInstanceDataBuffer(*desc.instanceBuffer, desc.instanceStart, desc.instanceCount);
	}
	if (_indexBuffer != nullptr && _indexBuffer->GetCount() > 0 && (desc.skip & SkipState::SkipIndexBuffer) == 0)
	{
		_indexBuffer->Bind(desc.count, desc.offset);
	}
	if ((desc.skip & SkipState::SkipVertexBuffer) == 0)
	{
		_vertexBuffer->Bind();
	}
	if ((desc.skip & SkipState::SkipRenderState) == 0)
	{
		bgfx::setState(desc.state, desc.rgba);
	}

	bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), desc.program.GetRawHandle(), 0,
	             desc.preserveState ? BGFX_DISCARD_NONE : BGFX_DISCARD_ALL);
}
