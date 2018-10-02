#pragma once
#ifndef OPENBLACK_VERTEXBUFFER_H
#define OPENBLACK_VERTEXBUFFER_H

#include "OpenGL.h"
#include <cstdio>
#include <stdint.h>

namespace OpenBlack
{
	namespace Graphics
	{
		class VertexBuffer {
		public:
			VertexBuffer();
			VertexBuffer(const VertexBuffer &other);
			~VertexBuffer();

			bool Create(void* data, size_t size);

			//GLvoid *GetData();
			//const GLvoid *GetData() const;

			//uint32_t GetCount() const;
			size_t GetSize() const;
			GLuint GetVBO() const;
		private:
			uint32_t m_count;
			size_t m_size;
			//uint8_t* m_data;

			GLuint m_vbo;
			GLuint m_hint;
		};

	}
}

#endif // OPENBLACK_VERTEXBUFFER_H
