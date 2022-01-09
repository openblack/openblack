/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "IndexBuffer.h"

#include <cassert>
#include <string>
#include <utility>

using namespace openblack::graphics;

IndexBuffer::IndexBuffer(std::string name, const void* indices, uint32_t indexCount, Type type)
    : _name(std::move(name))
    , _count(indexCount)
    , _type(type)
    , _handle(BGFX_INVALID_HANDLE)
{
	assert(indices != nullptr);
	assert(indexCount > 0);

	auto mem = bgfx::makeRef(indices, indexCount * GetTypeSize(_type));
	_handle = bgfx::createIndexBuffer(mem, type == Type::Uint32 ? BGFX_BUFFER_INDEX32 : 0);
	bgfx::setName(_handle, _name.c_str());
}

IndexBuffer::IndexBuffer(std::string name, const bgfx::Memory* mem, Type type)
    : _name(std::move(name))
    , _type(type)
    , _handle(BGFX_INVALID_HANDLE)
{
	_count = mem->size / sizeof(uint16_t);

	_handle = bgfx::createIndexBuffer(mem, type == Type::Uint32 ? BGFX_BUFFER_INDEX32 : 0);
	bgfx::setName(_handle, _name.c_str());
}

IndexBuffer::~IndexBuffer()
{
	if (bgfx::isValid(_handle))
		bgfx::destroy(_handle);
}

uint32_t IndexBuffer::GetCount() const
{
	return _count;
}

uint32_t IndexBuffer::GetSize() const
{
	return _count * GetStride();
}

uint32_t IndexBuffer::GetStride() const
{
	return GetTypeSize(_type);
}

IndexBuffer::Type IndexBuffer::GetType() const
{
	return _type;
}

uint32_t IndexBuffer::GetTypeSize(Type type)
{
	return static_cast<uint32_t>(type == Type::Uint16 ? sizeof(uint16_t) : sizeof(uint32_t));
}

void IndexBuffer::Bind(uint32_t count, uint32_t startIndex) const
{
	bgfx::setIndexBuffer(_handle, startIndex, count);
}
