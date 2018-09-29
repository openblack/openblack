#pragma once
#ifndef OPENBLACK_INDEXBUFFER_H
#define OPENBLACK_INDEXBUFFER_H

#include "OpenGL.h"
#include <cstdio>

namespace OpenBlack
{
	namespace Graphics
	{
		class IndexBuffer {
		public:
			IndexBuffer();
			~IndexBuffer();

			bool Create(const unsigned int* indices, unsigned int count);

			void Bind() const;
			unsigned int GetCount() const;
			GLuint GetHandle() const;
		protected:
			GLuint m_handle; /// Index Buffer Object
			unsigned int m_count;
		};

	}
}

#endif // OPENBLACK_INDEXBUFFER_H
