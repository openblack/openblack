/* OpenBlack - A reimplementation of Lionhead's Black & White.
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
			IndexBuffer() = delete;
			IndexBuffer(const IndexBuffer &other) = delete;
			IndexBuffer(IndexBuffer&&) = default;

			IndexBuffer(const void* indices, std::size_t indicesCount, GLenum type);

			~IndexBuffer();

			std::size_t GetCount() const;
			std::size_t GetSize() const;
			GLenum GetType() const;
			GLuint GetIBO() const;
		private:
			std::size_t _count;
			GLenum _type;

			GLuint _ibo;
			GLuint _hint;

			static std::size_t GetTypeSize(GLenum type);
		};

	}
}

#endif // OPENBLACK_INDEXBUFFER_H
