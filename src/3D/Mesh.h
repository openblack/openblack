#pragma once
#ifndef OPENBLACK_MESH_H
#define OPENBLACK_MESH_H

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

using namespace OpenBlack::Graphics;

namespace OpenBlack
{
	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		void LoadFromL3D(void* data, size_t size);
		void Draw();

	protected:
		unsigned int m_submeshCount;

		GLuint m_vao;

		GLuint* m_vertexBuffers;
		GLuint* m_indexBuffers;

		GLsizei* m_submeshSizes;
		GLsizei* m_submeshSizesV;
	};
}

#endif
