#pragma once
#ifndef OPENBLACK_GRAPHICS_MESH_H
#define OPENBLACK_GRAPHICS_MESH_H

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <vector>

using namespace OpenBlack::Graphics;

namespace OpenBlack {
namespace Graphics {

struct VertexAttrib {
	GLuint index;          ///< Index of the vertex attribute.
	GLint size;            ///< Number of components per vertex attribute, must be 1, 2, 3, 4.
	GLenum type;           ///< Data type of each attribute component in the array.
	GLsizei stride;        ///< Byte offset between consecutive vertex attributes.
	const GLvoid *offset; ///< Offset of the first component of the first generic vertex attribute.
	bool normalized;
	bool integer;

	VertexAttrib() { }
	VertexAttrib(GLuint i, GLint s, GLenum t, GLsizei st = 0, const GLvoid *of = 0) :
		index(i), size(s), type(t), stride(st), offset(of), normalized(false), integer(false) { }
	VertexAttrib(GLuint i, GLint s, GLenum t, bool intg, GLsizei st = 0, const GLvoid *of = 0) :
		index(i), size(s), type(t), stride(st), offset(of), normalized(false), integer(intg) { }
};

typedef std::vector<VertexAttrib> VertexDecl;

class Mesh
{
public:
	Mesh(std::shared_ptr<VertexBuffer>, const VertexDecl &decl, GLuint type = GL_TRIANGLES);
	Mesh(std::shared_ptr<VertexBuffer>, std::shared_ptr<IndexBuffer>, const VertexDecl &decl, GLuint type = GL_TRIANGLES);
	~Mesh();

	std::shared_ptr<VertexBuffer> GetVertexBuffer();
	std::shared_ptr<IndexBuffer> GetIndexBuffer();

	const GLuint GetType() const noexcept;

	void Draw();
protected:
	std::shared_ptr<VertexBuffer> _vertexBuffer;
	std::shared_ptr<IndexBuffer> _indexBuffer;
private:
	GLuint _vao;
	GLuint _type;

	VertexDecl _vertexDecl;

	void bindVertexDecl();
};

}
}

#endif
