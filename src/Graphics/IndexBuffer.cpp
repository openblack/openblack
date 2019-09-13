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

#include "IndexBuffer.h"

#include <cassert>

#include "OpenGL.h"

using namespace openblack::graphics;

IndexBuffer::IndexBuffer(const void* indices, size_t indicesCount, Type type)
	: _count(indicesCount)
	, _type(type)
	, _ibo(0)
	, _hint(GL_STATIC_DRAW)
	, _bgfxHandle(BGFX_INVALID_HANDLE)
{
	assert(indices != nullptr);
	assert(indicesCount > 0);

	auto mem = bgfx::makeRef(indices, indicesCount * GetTypeSize(_type));
	_bgfxHandle = bgfx::createIndexBuffer(mem, type == Type::Uint32 ? BGFX_BUFFER_INDEX32 : 0);
	bgfx::frame();

	glGenBuffers(1, &_ibo);
	if (glGetError() != GL_NO_ERROR)
		return;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _count * GetTypeSize(_type), indices, _hint);
}

IndexBuffer::~IndexBuffer()
{
	if (_ibo != 0)
		glDeleteBuffers(1, &_ibo);
	if (bgfx::isValid(_bgfxHandle))
		bgfx::destroy(_bgfxHandle);
}

std::size_t IndexBuffer::GetCount() const
{
	return _count;
}

std::size_t IndexBuffer::GetSize() const
{
	return _count * GetStride();
}

std::size_t IndexBuffer::GetStride() const
{
	return GetTypeSize(_type);
}

IndexBuffer::Type IndexBuffer::GetType() const
{
	return _type;
}

std::size_t IndexBuffer::GetTypeSize(Type type)
{
	return type == Type::Uint16 ? sizeof(uint16_t) : sizeof(uint32_t);
}

void IndexBuffer::Bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	bgfx::setIndexBuffer(_bgfxHandle);
}
