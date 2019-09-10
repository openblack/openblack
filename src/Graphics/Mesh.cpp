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

#include <stdexcept>
#include <array>

#include "OpenGL.h"

using namespace openblack::graphics;

namespace
{
constexpr std::array<GLenum, 5> topologies = {
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
};
}

Mesh::Mesh(VertexBuffer* vertexBuffer, Topology topology)
	: _vertexBuffer(vertexBuffer)
	, _topology(topology)
{
	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	_vertexBuffer->Bind();
	_vertexBuffer->bindVertexDecl();

	glBindVertexArray(0);
}

Mesh::Mesh(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer, Topology topology)
	: _vertexBuffer(vertexBuffer)
	, _indexBuffer(indexBuffer)
	, _topology(topology)
{
	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	_vertexBuffer->Bind();
	_vertexBuffer->bindVertexDecl();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer->GetIBO());

	glBindVertexArray(0);
}

Mesh::~Mesh()
{
	if (_vao != 0)
		glDeleteVertexArrays(1, &_vao);
}

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

void Mesh::Draw()
{
	if (_indexBuffer != nullptr && _indexBuffer->GetCount() > 0)
	{
		Draw(_indexBuffer->GetCount(), 0);
	}
	else
	{
		Draw(_vertexBuffer->GetVertexCount(), 0);
	}
}

void Mesh::Draw(uint32_t count, uint32_t startIndex)
{
	auto topology = topologies[static_cast<size_t>(_topology)];
	glBindVertexArray(_vao);
	if (_indexBuffer != nullptr && _indexBuffer->GetCount() > 0)
	{
		auto indexBufferOffset = startIndex * _indexBuffer->GetStride();
		auto indexType = _indexBuffer->GetType() == IndexBuffer::Type::Uint16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
		glDrawElements(topology, count, indexType, reinterpret_cast<void*>(indexBufferOffset));
	}
	else
	{
		glDrawArrays(topology, 0, count);
	}
}
