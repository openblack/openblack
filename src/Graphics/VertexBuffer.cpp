/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "VertexBuffer.h"

#include <array>
#include <cassert>

using namespace openblack::graphics;

namespace
{

constexpr std::array<bgfx::AttribType::Enum, 3> types {
    bgfx::AttribType::Uint8,
    bgfx::AttribType::Int16,
    bgfx::AttribType::Float,
};
constexpr std::array<bgfx::Attrib::Enum, 18> attributes {
    bgfx::Attrib::Enum::Position,  bgfx::Attrib::Enum::Normal,    bgfx::Attrib::Enum::Tangent,   bgfx::Attrib::Enum::Bitangent,
    bgfx::Attrib::Enum::Color0,    bgfx::Attrib::Enum::Color1,    bgfx::Attrib::Enum::Color2,    bgfx::Attrib::Enum::Color3,
    bgfx::Attrib::Enum::Indices,   bgfx::Attrib::Enum::Weight,    bgfx::Attrib::Enum::TexCoord0, bgfx::Attrib::Enum::TexCoord1,
    bgfx::Attrib::Enum::TexCoord2, bgfx::Attrib::Enum::TexCoord3, bgfx::Attrib::Enum::TexCoord4, bgfx::Attrib::Enum::TexCoord5,
    bgfx::Attrib::Enum::TexCoord6, bgfx::Attrib::Enum::TexCoord7,
};

} // namespace

VertexBuffer::VertexBuffer(std::string name, const void* vertices, uint32_t vertexCount, VertexDecl decl)
    : _name(std::move(name))
    , _vertexCount(vertexCount)
    , _vertexDecl(std::move(decl))
    , _strideBytes(0)
    , _handle(BGFX_INVALID_HANDLE)
    , _layoutHandle(BGFX_INVALID_HANDLE)
{
	// assert(vertices != nullptr);
	assert(vertexCount > 0);
	assert(!_vertexDecl.empty());

	// Extract gl types from decl
	_vertexDeclOffsets.reserve(_vertexDecl.size());
	static const std::array<std::array<uint32_t, 4>, 3> strides = {
	    std::array<uint32_t, 4> {4, 4, 4, 4},   // Uint8
	    std::array<uint32_t, 4> {4, 4, 8, 8},   // Int16
	    std::array<uint32_t, 4> {4, 8, 12, 16}, // Float
	};

	bgfx::VertexLayout layout;
	layout.begin();
	for (auto& d : _vertexDecl)
	{
		_vertexDeclOffsets.push_back(_strideBytes);
		_strideBytes += strides[static_cast<size_t>(d._type)][d._num - 1];
		layout.add(attributes[static_cast<size_t>(d._attribute)], d._num, types[static_cast<size_t>(d._type)], d._normalized,
		           d._asInt);
	}
	layout.end();
	assert(layout.m_stride == _strideBytes);

	auto mem = bgfx::makeRef(vertices, vertexCount * layout.m_stride);
	_handle = bgfx::createVertexBuffer(mem, layout);
	_layoutHandle = bgfx::createVertexLayout(layout);
	bgfx::setName(_handle, _name.c_str());
}

VertexBuffer::VertexBuffer(std::string name, const bgfx::Memory* mem, VertexDecl decl)
    : _name(std::move(name))
    , _vertexCount(0)
    , _vertexDecl(std::move(decl))
    , _strideBytes(0)
    , _handle(BGFX_INVALID_HANDLE)
    , _layoutHandle(BGFX_INVALID_HANDLE)
{
	// assert(vertices != nullptr);
	assert(!_vertexDecl.empty());

	// Extract gl types from decl
	_vertexDeclOffsets.reserve(_vertexDecl.size());
	static const std::array<std::array<uint32_t, 4>, 3> strides = {
	    std::array<uint32_t, 4> {4, 4, 4, 4},   // Uint8
	    std::array<uint32_t, 4> {4, 4, 8, 8},   // Int16
	    std::array<uint32_t, 4> {4, 8, 12, 16}, // Float
	};

	bgfx::VertexLayout layout;
	layout.begin();
	for (auto& d : _vertexDecl)
	{
		_vertexDeclOffsets.push_back(_strideBytes);
		_strideBytes += strides[static_cast<size_t>(d._type)][d._num - 1];
		layout.add(attributes[static_cast<size_t>(d._attribute)], d._num, types[static_cast<size_t>(d._type)], d._normalized,
		           d._asInt);
	}
	layout.end();
	assert(layout.m_stride == _strideBytes);

	_vertexCount = mem->size / static_cast<uint32_t>(_strideBytes);

	_handle = bgfx::createVertexBuffer(mem, layout);
	_layoutHandle = bgfx::createVertexLayout(layout);
	bgfx::setName(_handle, _name.c_str());
}

VertexBuffer::~VertexBuffer()
{
	if (bgfx::isValid(_handle))
		bgfx::destroy(_handle);
	if (bgfx::isValid(_layoutHandle))
		bgfx::destroy(_layoutHandle);
}

uint32_t VertexBuffer::GetCount() const noexcept
{
	return _vertexCount;
}

uint32_t VertexBuffer::GetStrideBytes() const noexcept
{
	return _strideBytes;
}

uint32_t VertexBuffer::GetSizeInBytes() const noexcept
{
	return _vertexCount * _strideBytes;
}

void VertexBuffer::Bind() const
{
	bgfx::setVertexBuffer(0, _handle, 0, _vertexCount, _layoutHandle);
}
