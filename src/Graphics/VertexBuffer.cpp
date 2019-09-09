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

#include "VertexBuffer.h"

#include <cassert>

#include <Graphics/OpenGL.h>

using namespace openblack::graphics;

VertexBuffer::VertexBuffer(const void* vertices, size_t vertexCount, size_t strideBytes):
    _vertexCount(vertexCount),
    _strideBytes(strideBytes),
    _vbo()
{
	// assert(vertices != nullptr);
	assert(vertexCount > 0);
	assert(strideBytes > 0);

	glGenBuffers(1, &_vbo);
	if (glGetError() != GL_NO_ERROR)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * strideBytes, vertices, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
	if (_vbo != 0)
		glDeleteBuffers(1, &_vbo);
}

/*const std::unique_ptr<GLvoid*> VertexBuffer::GetData() const
{
	size_t dataSize = _vertexCount * _strideBytes;
	auto data = std::make_unique<GLvoid*>(dataSize);

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glGetBufferSubData(_vbo, 0, dataSize, data.get());

	return std::move(data);
}*/

size_t VertexBuffer::GetVertexCount() const noexcept
{
	return _vertexCount;
}

size_t VertexBuffer::GetStrideBytes() const noexcept
{
	return _strideBytes;
}

size_t VertexBuffer::GetSizeInBytes() const noexcept
{
	return _vertexCount * _strideBytes;
}

void VertexBuffer::Bind()
{
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
}
