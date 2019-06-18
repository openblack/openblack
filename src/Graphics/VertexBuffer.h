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
#ifndef OPENBLACK_VERTEXBUFFER_H
#define OPENBLACK_VERTEXBUFFER_H

#include <Graphics/OpenGL.h>
#include <memory>

namespace OpenBlack
{
	namespace Graphics
	{
		class VertexBuffer {
		public:
			VertexBuffer() = delete;
			VertexBuffer(const VertexBuffer &other) = delete;
			VertexBuffer(VertexBuffer&&) = default;

			VertexBuffer(const void* vertices, size_t vertexCount, size_t strideBytes, GLuint hint = GL_STATIC_DRAW);

			~VertexBuffer();

			VertexBuffer& operator=(const VertexBuffer&) = delete;
			VertexBuffer& operator=(VertexBuffer&&) = default;

			size_t GetVertexCount() const noexcept;
			size_t GetStrideBytes() const noexcept;
			size_t GetSizeInBytes() const noexcept;
			GLuint GetHint()		const noexcept;
			GLuint GetVBO()			const noexcept;

			//const std::unique_ptr<GLvoid*> GetData() const;

			inline void Bind() { glBindBuffer(GL_ARRAY_BUFFER, _vbo); }
		private:
			GLuint _vbo;
			size_t _vertexCount;
			size_t _strideBytes;
			GLuint _hint;
		};

	}
}

#endif // OPENBLACK_VERTEXBUFFER_H
