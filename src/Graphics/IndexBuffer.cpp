/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include "IndexBuffer.h"

#include <cassert>

using namespace OpenBlack::Graphics;

IndexBuffer::IndexBuffer(const void* indices, size_t indicesCount, GLenum type):
    _ibo(0), _count(indicesCount), _type(type), _hint(GL_STATIC_DRAW)
{
	assert(indices != nullptr);
	assert(indicesCount > 0);

	glGenBuffers(1, &_ibo);
	if (glGetError() != GL_NO_ERROR)
		return;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _count * GetTypeSize(_type), indices, _hint);
}

IndexBuffer::~IndexBuffer()
{
	if (_ibo != 0)
		glDeleteBuffers(1, &_ibo);
}

std::size_t IndexBuffer::GetCount() const
{
	return _count;
}

std::size_t IndexBuffer::GetSize() const
{
	return _count * GetTypeSize(_type);
}

GLenum IndexBuffer::GetType() const
{
	return _type;
}

GLuint IndexBuffer::GetIBO() const
{
	return _ibo;
}

std::size_t IndexBuffer::GetTypeSize(GLenum type)
{
	switch (type)
	{
	case GL_UNSIGNED_BYTE:
		return 1;
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_2_BYTES:
		return 2;
	case GL_3_BYTES:
		return 3;
	case GL_INT:
	case GL_UNSIGNED_INT:
	case GL_FLOAT:
	case GL_4_BYTES:
		return 4;
	case GL_DOUBLE:
		return 8;
	default:
		break;
	}

	return 0;
}
