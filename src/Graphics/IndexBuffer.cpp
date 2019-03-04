/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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

IndexBuffer::IndexBuffer() : m_count(0), m_size(0), m_type(GL_UNSIGNED_INT), m_ibo(0), m_hint(GL_STATIC_DRAW) {
}

IndexBuffer::IndexBuffer(const IndexBuffer &other) : m_ibo(0), m_hint(GL_STATIC_DRAW) {
	*this = other;
}

IndexBuffer::~IndexBuffer() {
	if (m_ibo != 0) {
		glDeleteBuffers(1, &m_ibo);
	}
}

bool IndexBuffer::Create(void* indices, size_t size, GLenum type) {
	assert(m_ibo == 0); // VertexBuffer already initialised

	glGenBuffers(1, &m_ibo);
	if (glGetError() != GL_NO_ERROR)
		return false;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, m_hint);

	if (glGetError() != GL_NO_ERROR)
	{
		glDeleteBuffers(1, &m_ibo);
		m_ibo = 0;
		return false;
	}

	m_size = size;
	m_count = size / GetTypeSize(type);
	m_type = type;
	return true;
}

unsigned int IndexBuffer::GetCount() const {
	return m_count;
}

GLenum IndexBuffer::GetType() const {
	return m_type;
}

GLuint IndexBuffer::GetIBO() const {
	return m_ibo;
}

uint32_t IndexBuffer::GetTypeSize(GLenum type) {
	switch (type) {
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
