/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Mesh.h"

#include "ShaderProgram.h"

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

Mesh::Topology Mesh::GetTopology() const noexcept
{
	return _topology;
}

void Mesh::Draw(graphics::RenderPass viewId, const openblack::graphics::ShaderProgram &program, uint64_t state, uint32_t rgba) const
{
	if (_indexBuffer != nullptr && _indexBuffer->GetCount() > 0)
	{
		Draw(viewId, program, _indexBuffer->GetCount(), 0, state, rgba);
	}
	else
	{
		Draw(viewId, program, _vertexBuffer->GetCount(), 0, state, rgba);
	}
}

void Mesh::Draw(graphics::RenderPass viewId, const openblack::graphics::ShaderProgram& program, const bgfx::DynamicVertexBufferHandle& instanceBuffer, uint32_t instanceStart, uint32_t instanceCount, uint64_t state, uint32_t rgba) const
{
	bgfx::setInstanceDataBuffer(instanceBuffer, instanceStart, instanceCount);
	Draw(viewId, program, state, rgba);
}

void Mesh::Draw(graphics::RenderPass viewId, const openblack::graphics::ShaderProgram &program, uint32_t count, uint32_t startIndex, uint64_t state, uint32_t rgba) const
{
	if (_indexBuffer != nullptr && _indexBuffer->GetCount() > 0)
	{
		_indexBuffer->Bind(count, startIndex);
	}
	_vertexBuffer->Bind();

	// Set render states.
	bgfx::setState(state, rgba);

	bgfx::submit(static_cast<bgfx::ViewId>(viewId), program.GetRawHandle());
}
