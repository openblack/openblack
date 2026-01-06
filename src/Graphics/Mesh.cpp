/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Mesh.h"

#include "GraphicsHandleBgfx.h"
#include "IndexBuffer.h"
#include "Locator.h"
#include "RendererInterface.h"
#include "ShaderProgram.h"
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

bool Mesh::IsIndexed() const
{
	return _indexBuffer != nullptr && _indexBuffer->count > 0;
}

Mesh::Topology Mesh::GetTopology() const noexcept
{
	return _topology;
}

void Mesh::Draw(const DrawDesc& desc) const
{
	auto& renderer = Locator::rendererInterface::value();
	if (desc.instanceBuffer && (desc.skip & SkipState::SkipInstanceBuffer) == 0)
	{
		bgfx::setInstanceDataBuffer(toBgfx(*desc.instanceBuffer), desc.instanceStart, desc.instanceCount);
	}
	if (_indexBuffer != nullptr && _indexBuffer->count > 0 && (desc.skip & SkipState::SkipIndexBuffer) == 0)
	{
		renderer.Bind(*_indexBuffer, desc.count, desc.offset);
	}
	if ((desc.skip & SkipState::SkipVertexBuffer) == 0)
	{
		renderer.Bind(*_vertexBuffer);
	}
	if ((desc.skip & SkipState::SkipRenderState) == 0)
	{
		bgfx::setState(desc.state, desc.rgba);
	}

	bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(desc.program.GetRawHandle()), 0,
	             desc.preserveState ? BGFX_DISCARD_NONE : BGFX_DISCARD_ALL);
}
