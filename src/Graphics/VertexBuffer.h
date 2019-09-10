/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace openblack
{
namespace graphics
{

struct VertexAttrib
{
	uint8_t _index;       ///< Index of the vertex attribute.
	uint8_t _size;        ///< Number of components per vertex attribute, must be 1, 2, 3, 4.
	uint32_t _type;       ///< Data type of each attribute component in the array.
	intptr_t _stride;     ///< Byte offset between consecutive vertex attributes.
	ptrdiff_t _offset;    ///< Offset of the first component of the first generic vertex attribute.
	bool _normalized;     /// < When using fixed point values, range will be normalized to 0.0-1.0 in shader.
	bool _asInt;          /// < Should not be altered. Unpacking will have to be done in vertex shader.

	VertexAttrib() {}
	VertexAttrib(uint8_t index, uint8_t size, uint32_t type, intptr_t stride = 0, const ptrdiff_t offset = 0)
		: _index(index)
		, _size(size)
		, _type(type)
		, _stride(stride)
		, _offset(offset)
		, _normalized(false)
		, _asInt(false)
	{
	}
	VertexAttrib(uint8_t index, uint8_t size, uint32_t type, bool asInt, bool normalized, intptr_t stride = 0, const ptrdiff_t offset = 0)
		: _index(index)
		, _size(size)
		, _type(type)
		, _stride(stride)
		, _offset(offset)
		, _normalized(normalized)
		, _asInt(asInt)
		{
		}
};

typedef std::vector<VertexAttrib> VertexDecl;

class VertexBuffer
{
  public:
	VertexBuffer(const void* vertices, size_t vertexCount, VertexDecl decl);
	~VertexBuffer();

	[[nodiscard]] size_t GetVertexCount() const noexcept;
	[[nodiscard]] size_t GetStrideBytes() const noexcept;
	[[nodiscard]] size_t GetSizeInBytes() const noexcept;

	void Bind();
	void bindVertexDecl();

  private:
	uint32_t _vbo;
	size_t _vertexCount;
	const VertexDecl _vertexDecl;
};

} // namespace graphics
} // namespace openblack
