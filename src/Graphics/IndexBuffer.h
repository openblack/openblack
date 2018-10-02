#pragma once
#ifndef OPENBLACK_INDEXBUFFER_H
#define OPENBLACK_INDEXBUFFER_H

#include "OpenGL.h"
#include <cstdio>
#include <stdint.h>

namespace OpenBlack
{
	namespace Graphics
	{
		class IndexBuffer {
		public:
			IndexBuffer();
			IndexBuffer(const IndexBuffer &other);
			~IndexBuffer();

			bool Create(void* indices, size_t size, GLenum type);

			unsigned int GetCount() const;
			GLenum GetType() const;
			GLuint GetIBO() const;
		private:
			uint32_t m_count;
			uint32_t m_size;
			GLenum m_type;

			GLuint m_ibo;
			GLuint m_hint;

			static uint32_t GetTypeSize(GLenum type);
		};

	}
}

#endif // OPENBLACK_INDEXBUFFER_H
