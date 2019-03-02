#include "Mesh.h"

#include <stdint.h>
#include <stdexcept>

#include <Game.h>

using namespace OpenBlack::Graphics;

Mesh::Mesh(std::shared_ptr<VertexBuffer> vertexBuffer, const VertexDecl &decl, GLuint type)
	: _vertexBuffer(std::move(vertexBuffer)),
	_vertexDecl(decl),
	_type(type)
{
	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	_vertexBuffer->Bind();
	bindVertexDecl();
}

Mesh::Mesh(std::shared_ptr<VertexBuffer> vertexBuffer, std::shared_ptr<IndexBuffer> indexBuffer, const VertexDecl &decl, GLuint type)
	: _vertexBuffer(std::move(vertexBuffer)),
	_indexBuffer(std::move(indexBuffer)),
	_vertexDecl(decl),
	_type(type)
{
	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	_vertexBuffer->Bind();
	bindVertexDecl();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer->GetIBO());
}	

Mesh::~Mesh()
{
	if (_vao != 0)
		glDeleteVertexArrays(1, &_vao);
}

std::shared_ptr<VertexBuffer> Mesh::GetVertexBuffer() {
	return _vertexBuffer;
}

std::shared_ptr<IndexBuffer> Mesh::GetIndexBuffer() {
	return _indexBuffer;
}

const GLuint Mesh::GetType() const noexcept {
	return _type;
}

void Mesh::Draw() {
	glBindVertexArray(_vao);
	if (_indexBuffer != nullptr && _indexBuffer->GetCount() > 0)
		glDrawElements(_type, _indexBuffer->GetCount(), _indexBuffer->GetType(), 0);
	else
		glDrawArrays(_type, 0, _vertexBuffer->GetVertexCount());
}

void Mesh::bindVertexDecl()
{
	for (size_t i = 0; i < _vertexDecl.size(); i++) {
		if (_vertexDecl[i].integer == true)
		{
			glVertexAttribIPointer(_vertexDecl[i].index,
				_vertexDecl[i].size,
				_vertexDecl[i].type,
				_vertexDecl[i].stride,
				_vertexDecl[i].offset);
		}
		else
		{
			glVertexAttribPointer(_vertexDecl[i].index,
				_vertexDecl[i].size,
				_vertexDecl[i].type,
				GL_FALSE,//_vertexDecl[i].normalized ? GL_TRUE : GL_FALSE,
				_vertexDecl[i].stride,
				_vertexDecl[i].offset);
		}

		glEnableVertexAttribArray(_vertexDecl[i].index);
	}
}
