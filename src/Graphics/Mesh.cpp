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

	bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), desc.program.GetRawHandle(), 0, desc.preserveState);
}
