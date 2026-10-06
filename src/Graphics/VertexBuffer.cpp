/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VertexBuffer.h"

#include <cassert>

#include <array>

#include <spdlog/spdlog.h>

#include "GraphicsHandleBgfx.h"

using namespace openblack::graphics;

namespace
{

constexpr std::array<bgfx::AttribType::Enum, 3> k_Types {
    bgfx::AttribType::Uint8,
    bgfx::AttribType::Int16,
    bgfx::AttribType::Float,
};
constexpr std::array<bgfx::Attrib::Enum, 18> k_Attributes {
    bgfx::Attrib::Enum::Position,  bgfx::Attrib::Enum::Normal,    bgfx::Attrib::Enum::Tangent,   bgfx::Attrib::Enum::Bitangent,
    bgfx::Attrib::Enum::Color0,    bgfx::Attrib::Enum::Color1,    bgfx::Attrib::Enum::Color2,    bgfx::Attrib::Enum::Color3,
    bgfx::Attrib::Enum::Indices,   bgfx::Attrib::Enum::Weight,    bgfx::Attrib::Enum::TexCoord0, bgfx::Attrib::Enum::TexCoord1,
    bgfx::Attrib::Enum::TexCoord2, bgfx::Attrib::Enum::TexCoord3, bgfx::Attrib::Enum::TexCoord4, bgfx::Attrib::Enum::TexCoord5,
    bgfx::Attrib::Enum::TexCoord6, bgfx::Attrib::Enum::TexCoord7,
};

} // namespace

VertexBuffer::VertexBuffer(std::string name, const void* mem, VertexDecl decl, bool dynamic) noexcept
    : _name(std::move(name))
    , _vertexCount(0)
    , _vertexDecl(std::move(decl))
    , _strideBytes(0)
    , _handle(BGFX_INVALID_HANDLE)
    , _dynamicHandle(BGFX_INVALID_HANDLE)
    , _dynamic(dynamic)
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
	for (const auto& d : _vertexDecl)
	{
		_vertexDeclOffsets.push_back(_strideBytes);
		_strideBytes += strides.at(static_cast<size_t>(d.type)).at(d.num - 1);
		layout.add(k_Attributes.at(static_cast<size_t>(d.attribute)), d.num, k_Types.at(static_cast<size_t>(d.type)),
		           d.normalized, d.asInt);
	}
	layout.end();
	assert(layout.m_stride == _strideBytes);

	const auto* bgfxMem = reinterpret_cast<const bgfx::Memory*>(mem);

	_vertexCount = bgfxMem->size / _strideBytes;

	if (_dynamic)
	{
		_dynamicHandle = fromBgfx(bgfx::createDynamicVertexBuffer(bgfxMem, layout));
	}
	else
	{
		_handle = fromBgfx(bgfx::createVertexBuffer(bgfxMem, layout));
	}
	_layoutHandle = fromBgfx(bgfx::createVertexLayout(layout));
	if (!IsValid())
	{
		// Out of handles, setting a name would write past bgfx's own arrays; the buffer is left empty and isn't drawn
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "{}: out of bgfx buffer handles, not created", _name);
		return;
	}
	if (!_dynamic)
	{
		bgfx::setName(toBgfx(_handle), _name.c_str());
	}
}

VertexBuffer::~VertexBuffer() noexcept
{
	if (bgfx::isValid(toBgfx(_handle)))
	{
		bgfx::destroy(toBgfx(_handle));
	}
	if (bgfx::isValid(toBgfx(_dynamicHandle)))
	{
		bgfx::destroy(toBgfx(_dynamicHandle));
	}
	if (bgfx::isValid(toBgfx(_layoutHandle)))
	{
		bgfx::destroy(toBgfx(_layoutHandle));
	}
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

bool VertexBuffer::IsValid() const noexcept
{
	return _dynamic ? bgfx::isValid(toBgfx(_dynamicHandle)) : bgfx::isValid(toBgfx(_handle));
}

void VertexBuffer::Bind() const
{
	if (!IsValid())
	{
		return;
	}
	if (_dynamic)
	{
		bgfx::setVertexBuffer(0, toBgfx(_dynamicHandle), 0, _vertexCount, toBgfx(_layoutHandle));
	}
	else
	{
		bgfx::setVertexBuffer(0, toBgfx(_handle), 0, _vertexCount, toBgfx(_layoutHandle));
	}
}

void VertexBuffer::BindStream(uint8_t stream, VertexLayoutHandle layout) const
{
	if (!IsValid())
	{
		return;
	}
	if (_dynamic)
	{
		bgfx::setVertexBuffer(stream, toBgfx(_dynamicHandle), 0, _vertexCount, toBgfx(layout));
	}
	else
	{
		bgfx::setVertexBuffer(stream, toBgfx(_handle), 0, _vertexCount, toBgfx(layout));
	}
}

void VertexBuffer::Update(const void* memory) const
{
	assert(_dynamic);
	if (!IsValid())
	{
		return;
	}
	bgfx::update(toBgfx(_dynamicHandle), 0, reinterpret_cast<const bgfx::Memory*>(memory));
}
