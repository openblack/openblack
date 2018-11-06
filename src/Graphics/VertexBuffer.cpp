#include "VertexBuffer.h"

#include <cassert>

using namespace OpenBlack::Graphics;

VertexBuffer::VertexBuffer() : m_count(0), m_size(0), m_vbo(0), m_hint(GL_STATIC_DRAW) {
}

VertexBuffer::VertexBuffer(const VertexBuffer &other) : m_vbo(0), m_hint(GL_STATIC_DRAW) {
	*this = other;
}

VertexBuffer::~VertexBuffer() {
	if (m_vbo != 0) {
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
	}

	//delete[] m_data;
}

/*GLvoid *VertexBuffer::GetData() {
	return static_cast<GLvoid *>(m_data);
}

const GLvoid *VertexBuffer::GetData() const {
	return static_cast<const GLvoid *>(m_data);
}*/

size_t VertexBuffer::GetSize() const {
	return m_size;
}

GLuint VertexBuffer::GetVBO() const {
	return m_vbo;
}

bool VertexBuffer::Create(void* data, size_t size) {
	assert(m_vbo == 0); // VertexBuffer already initialised

	glGenBuffers(1, &m_vbo);
	if (glGetError() != GL_NO_ERROR)
		return false;

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

	if (glGetError() != GL_NO_ERROR)
	{
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
		return false;
	}

	m_size = size;
	return true;
}
