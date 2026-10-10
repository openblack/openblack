/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "GraphicsHandle.h"

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

	Attribute attribute; ///< Type of data represented
	uint8_t num;         ///< Number of components per vertex attribute, must be 1, 2,
	                     ///< 3, 4.
	Type type;           ///< Data type of each attribute component in the array.
	bool normalized;     /// < When using fixed point values, range will be
	                     /// normalized to 0.0-1.0 in shader.
	bool asInt;          /// < Should not be altered. Unpacking will have to be done in
	                     /// vertex shader.

	VertexAttrib(Attribute attribute, uint8_t num, Type type, bool normalized = false, bool asInt = false) noexcept
	    : attribute(attribute)
	    , num(num)
	    , type(type)
	    , normalized(normalized)
	    , asInt(asInt)
	{
	}
};

using VertexDecl = std::vector<VertexAttrib>;

struct VertexBuffer
{
	const std::string name;
	const uint32_t vertexCount;
	const uint32_t strideBytes;
	const VertexBufferHandle handle;
	const VertexLayoutHandle layoutHandle; // TODO: Remove
};

using VertexBufferUniquePtr = std::unique_ptr<VertexBuffer, std::function<void(VertexBuffer*)>>;

} // namespace openblack::graphics
