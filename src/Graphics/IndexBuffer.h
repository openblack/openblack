/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

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
