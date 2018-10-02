#pragma once
#ifndef OPENBLACK_GRAPHICS_MESH_H
#define OPENBLACK_GRAPHICS_MESH_H

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <vector>

using namespace OpenBlack::Graphics;

namespace OpenBlack
{
	namespace Graphics
	{
		struct VertexAttrib {
			GLuint index;          ///< Index of the vertex attribute.
			GLint size;            ///< Number of components per vertex attribute, must be 1, 2, 3, 4.
			GLenum type;           ///< Data type of each attribute component in the array.
			GLsizei stride;        ///< Byte offset between consecutive vertex attributes.
			const GLvoid *offset; ///< Offset of the first component of the first generic vertex attribute.

			VertexAttrib() { }
			VertexAttrib(GLuint i, GLint s, GLenum t, GLsizei st = 0, const GLvoid *of = 0) :
				index(i), size(s), type(t), stride(st), offset(of) { }
		};

		typedef std::vector<VertexAttrib> VertexDecl;

		class Mesh
		{
		public:
			Mesh(GLuint type = GL_TRIANGLES, GLuint hint = GL_STATIC_DRAW);
			~Mesh();

			void Create(void* vertexData, size_t vertexDataSize, void* indicesData, size_t indiciesSize);

			VertexBuffer* GetVertexBuffer();
			IndexBuffer* GetIndexBuffer();

			void SetType(GLuint type);
			GLuint GetType() const;

			void SetVertexDecl(const VertexDecl &decl);

			void Render();
		protected:
			VertexBuffer m_vertexBuffer;
			IndexBuffer m_indexBuffer;
		private:
			GLuint m_vao;
			GLuint m_type;
			GLuint m_hint;
			VertexDecl m_vertexDecl;
		};
	}
}

#endif
