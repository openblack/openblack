#include "IndexBuffer.h"

#include <cassert>

using namespace OpenBlack::Graphics;

IndexBuffer::IndexBuffer() : m_count(0), m_handle(0) {
}

IndexBuffer::~IndexBuffer() {
	if (m_handle != 0)
		glDeleteBuffers(1, &m_handle);
}

bool IndexBuffer::Create(const unsigned int* indices, unsigned int count) {
	assert(m_handle == 0); // VertexBuffer already initialised

	glGenBuffers(1, &m_handle);
	if (glGetError() != GL_NO_ERROR)
		return false;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_handle);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	if (glGetError() != GL_NO_ERROR)
	{
		glDeleteBuffers(1, &m_handle);
		m_handle = 0;
		return false;
	}

	m_count = count;
	return true;
}

void IndexBuffer::Bind() const {
	glBindBuffer(GL_ARRAY_BUFFER, m_handle);
}

unsigned int IndexBuffer::GetCount() const {
	return m_count;
}

GLuint IndexBuffer::GetHandle() const {
	return m_handle;
}
