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

#include <bgfx/bgfx.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace openblack::graphics
{

struct VertexAttrib
{
	enum class Attribute : uint8_t
	{
		Position,
		Normal,
		Tangent,
		Bitangent,
		Color0,
		Color1,
		Color2,
		Color3,
		Indices,
		Weight,
		TexCoord0,
		TexCoord1,
		TexCoord2,
		TexCoord3,
		TexCoord4,
		TexCoord5,
		TexCoord6,
		TexCoord7,
	};
	enum class Type : uint8_t
	{
		Uint8,
		Int16,
		Float,
	};

	Attribute _attribute; ///< Type of data represented
	uint8_t _num;         ///< Number of components per vertex attribute, must be 1, 2,
	                      ///< 3, 4.
	Type _type;           ///< Data type of each attribute component in the array.
	bool _normalized;     /// < When using fixed point values, range will be
	                      /// normalized to 0.0-1.0 in shader.
	bool _asInt;          /// < Should not be altered. Unpacking will have to be done in
	                      /// vertex shader.

	VertexAttrib() {}
	VertexAttrib(Attribute attribute, uint8_t num, Type type, bool normalized = false, bool asInt = false)
	    : _attribute(attribute), _num(num), _type(type), _normalized(normalized), _asInt(asInt)
	{
	}
};

typedef std::vector<VertexAttrib> VertexDecl;

class VertexBuffer
{
public:
	VertexBuffer(std::string name, const void* vertices, uint32_t vertexCount, VertexDecl decl);
	VertexBuffer(std::string name, const bgfx::Memory* memory, VertexDecl decl);
	~VertexBuffer();

	[[nodiscard]] uint32_t GetCount() const noexcept;
	[[nodiscard]] size_t GetStrideBytes() const noexcept;
	[[nodiscard]] size_t GetSizeInBytes() const noexcept;

	void Bind() const;

private:
	std::string _name;
	uint32_t _vertexCount;
	const VertexDecl _vertexDecl;
	size_t _strideBytes;
	std::vector<const void*> _vertexDeclOffsets;
	bgfx::VertexBufferHandle _handle;
	bgfx::VertexLayoutHandle _layoutHandle;
};

} // namespace openblack::graphics
