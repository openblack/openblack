#include "VertexBuffer.h"

#include <cassert>

using namespace OpenBlack::Graphics;

VertexBuffer::VertexBuffer() : m_size(0), m_handle(0) {
}

/*VertexBuffer::VertexBuffer(const VertexBuffer &other) : m_handle(0) {
	*this = other;
}*/

VertexBuffer::~VertexBuffer() {
	if (m_handle != 0)
		glDeleteBuffers(1, &m_handle);
}

bool VertexBuffer::Create(void* data, size_t size) {
	assert(m_handle == 0); // VertexBuffer already initialised

	glGenBuffers(1, &m_handle);
	if (glGetError() != GL_NO_ERROR)
		return false;

	glBindBuffer(GL_ARRAY_BUFFER, m_handle);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

	if (glGetError() != GL_NO_ERROR)
	{
		glDeleteBuffers(1, &m_handle);
		m_handle = 0;
		return false;
	}

	m_size = size;
	return true;
}

void VertexBuffer::Bind() {
	glBindBuffer(GL_ARRAY_BUFFER, m_handle);
}

size_t VertexBuffer::Size() const {
	return m_size;
}

GLuint VertexBuffer::GetHandle() const {
	return m_handle;
}
