/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

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

	VertexAttrib(Attribute attribute, uint8_t num, Type type, bool normalized = false, bool asInt = false)
	    : _attribute(attribute)
	    , _num(num)
	    , _type(type)
	    , _normalized(normalized)
	    , _asInt(asInt)
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
	[[nodiscard]] uint32_t GetStrideBytes() const noexcept;
	[[nodiscard]] uint32_t GetSizeInBytes() const noexcept;

	void Bind() const;

private:
	std::string _name;
	uint32_t _vertexCount;
	const VertexDecl _vertexDecl;
	uint32_t _strideBytes;
	std::vector<uint32_t> _vertexDeclOffsets;
	bgfx::VertexBufferHandle _handle;
	bgfx::VertexLayoutHandle _layoutHandle;
};

} // namespace openblack::graphics
