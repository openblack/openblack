#include "Mesh.h"

#include <stdint.h>
#include <stdexcept>

#include <Game.h>

using namespace OpenBlack::Graphics;

Mesh::Mesh(GLuint type, GLuint hint) : m_type(type), m_hint(hint), m_vao(0) {
}

Mesh::~Mesh()
{
	if (m_vao != NULL)
	{
		glDeleteVertexArrays(1, &m_vao);
	}
}

VertexBuffer* Mesh::GetVertexBuffer() {
	return &m_vertexBuffer;
}

IndexBuffer* Mesh::GetIndexBuffer() {
	return &m_indexBuffer;
}

void Mesh::SetType(GLuint type) {
	m_type = type;
}

GLuint Mesh::GetType() const {
	return m_type;
}

void Mesh::SetVertexDecl(const VertexDecl &decl) {
	m_vertexDecl = decl;
}

void Mesh::Render() {
	glBindVertexArray(m_vao);

	//printf("Rendering Mesh (VAO: %d) w/ %d verts and %d indicies\n", m_vao, m_vertexBuffer.GetSize(), m_indexBuffer.GetCount());

	if (m_indexBuffer.GetCount()) {
		glDrawElements(m_type, m_indexBuffer.GetCount(), m_indexBuffer.GetType(), 0);
	}
	else {
		//glDrawArrays(m_type, 0, m_vertexBuffer.GetCount());
	}

	// not needed really.
	glBindVertexArray(0);
}

void Mesh::Create(void* vertexData, size_t vertexDataSize, void* indicesData, size_t indiciesSize)
{
	m_vertexBuffer.Create(vertexData, vertexDataSize);
	m_indexBuffer.Create(indicesData, indiciesSize, GL_UNSIGNED_SHORT); //  todo: unhardcode

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer.GetVBO());

	for (uint32_t i = 0; i < m_vertexDecl.size(); ++i) {
		glVertexAttribPointer(m_vertexDecl[i].index,
			m_vertexDecl[i].size,
			m_vertexDecl[i].type,
			GL_FALSE,
			m_vertexDecl[i].stride,
			m_vertexDecl[i].offset);
		glEnableVertexAttribArray(m_vertexDecl[i].index);
	}

	if (m_indexBuffer.GetCount()) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer.GetIBO());
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
