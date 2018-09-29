#pragma once
#ifndef OPENBLACK_VERTEXBUFFER_H
#define OPENBLACK_VERTEXBUFFER_H

#include "OpenGL.h"
#include <cstdio>

namespace OpenBlack
{
	namespace Graphics
	{
		class VertexBuffer {
		public:
			VertexBuffer();
			~VertexBuffer();

			// copying and assignment
			//VertexBuffer(const VertexBuffer &other);
			// VertexBuffer &operator=(const VertexBuffer &other);

			bool Create(void* data, size_t size);

			void Bind();
			size_t Size() const;
			GLuint GetHandle() const;
		protected:
			GLuint m_handle; /// Vertex Buffer Object
			size_t m_size; /// Size of a buffer element in b ytes
		};

	}
}

#endif // OPENBLACK_VERTEXBUFFER_H
