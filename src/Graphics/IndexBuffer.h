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

#include <cstddef>
#include <cstdint>

#include <bgfx/bgfx.h>

namespace openblack::graphics
{
class IndexBuffer
{
  public:
	enum class Type : uint8_t
	{
		Uint16,
		Uint32,
	};
	static std::size_t GetTypeSize(Type type);

	IndexBuffer()                         = delete;
	IndexBuffer(const IndexBuffer& other) = delete;
	IndexBuffer(IndexBuffer&&)            = default;

	IndexBuffer(const void* indices, std::size_t indicesCount, Type type);

	~IndexBuffer();

	[[nodiscard]] std::size_t GetCount() const;
	[[nodiscard]] std::size_t GetSize() const;
	[[nodiscard]] std::size_t GetStride() const;
	[[nodiscard]] Type GetType() const;

	void Bind(uint32_t count, uint32_t startIndex=0);

  private:
	std::size_t _count;
	Type _type;
	bgfx::IndexBufferHandle _handle;
};

} // namespace openblack
