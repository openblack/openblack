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

#include "VertexBuffer.h"

#include <cassert>
#include <array>

#include <Graphics/OpenGL.h>

using namespace openblack::graphics;

namespace {

constexpr std::array<bgfx::AttribType::Enum, 3> types {
	bgfx::AttribType::Uint8,
	bgfx::AttribType::Int16,
	bgfx::AttribType::Float,
};
constexpr std::array<bgfx::Attrib::Enum, 18> attributes {
	bgfx::Attrib::Enum::Position,
	bgfx::Attrib::Enum::Normal,
	bgfx::Attrib::Enum::Tangent,
	bgfx::Attrib::Enum::Bitangent,
	bgfx::Attrib::Enum::Color0,
	bgfx::Attrib::Enum::Color1,
	bgfx::Attrib::Enum::Color2,
	bgfx::Attrib::Enum::Color3,
	bgfx::Attrib::Enum::Indices,
	bgfx::Attrib::Enum::Weight,
	bgfx::Attrib::Enum::TexCoord0,
	bgfx::Attrib::Enum::TexCoord1,
	bgfx::Attrib::Enum::TexCoord2,
	bgfx::Attrib::Enum::TexCoord3,
	bgfx::Attrib::Enum::TexCoord4,
	bgfx::Attrib::Enum::TexCoord5,
	bgfx::Attrib::Enum::TexCoord6,
	bgfx::Attrib::Enum::TexCoord7,
};

}

VertexBuffer::VertexBuffer(const void* vertices, size_t vertexCount, VertexDecl decl)
	: _vbo()
	, _vertexCount(vertexCount)
	, _vertexDecl(std::move(decl))
	, _strideBytes(0)
	, _bgfxHandle(BGFX_INVALID_HANDLE)
{
	// assert(vertices != nullptr);
	assert(vertexCount > 0);
	assert(!_vertexDecl.empty());

	// Extract gl types from decl
	_vertexDeclOffsets.reserve(_vertexDecl.size());
	static const std::array<std::array<size_t, 4>, 3> strides = {
		std::array<size_t, 4>{  4, 4, 4,  4 }, // Uint8
		std::array<size_t, 4>{  4, 4, 8, 8 }, // Int16
		std::array<size_t, 4>{  4,  8, 12, 16 }, // Float
	};

	bgfx::VertexLayout layout;
	layout.begin();
	for (auto& d: _vertexDecl)
	{
		_vertexDeclOffsets.push_back(reinterpret_cast<const void*>(_strideBytes));
		_strideBytes += strides[static_cast<size_t>(d._type)][d._num - 1];
		layout.add(attributes[static_cast<size_t>(d._attribute)], d._num, types[static_cast<size_t>(d._type)], d._normalized, d._asInt);
	}
	layout.end();
	assert(layout.m_stride == _strideBytes);

	auto mem = bgfx::makeRef(vertices, vertexCount * layout.m_stride);
	_bgfxHandle = bgfx::createVertexBuffer(mem, layout);
	bgfx::frame();

	glGenBuffers(1, &_vbo);
	if (glGetError() != GL_NO_ERROR)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * _strideBytes, vertices, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
	if (_vbo != 0)
		glDeleteBuffers(1, &_vbo);
	if (bgfx::isValid(_bgfxHandle))
		bgfx::destroy(_bgfxHandle);
}

/*const std::unique_ptr<GLvoid*> VertexBuffer::GetData() const
{
	size_t dataSize = _vertexCount * _strideBytes;
	auto data = std::make_unique<GLvoid*>(dataSize);

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glGetBufferSubData(_vbo, 0, dataSize, data.get());

	return std::move(data);
}*/

size_t VertexBuffer::GetVertexCount() const noexcept
{
	return _vertexCount;
}

size_t VertexBuffer::GetStrideBytes() const noexcept
{
	return _strideBytes;
}

size_t VertexBuffer::GetSizeInBytes() const noexcept
{
	return _vertexCount * _strideBytes;
}

void VertexBuffer::Bind()
{
	bgfx::setVertexBuffer(0, _bgfxHandle);
}

void VertexBuffer::bindVertexDecl()
{
	static const std::array<GLenum , 3> types = {
		GL_UNSIGNED_BYTE, // Uint8
		GL_SHORT, // Int16
		GL_FLOAT, // Float
	};
	for (size_t i = 0; i < _vertexDecl.size(); ++i)
	{
		auto type = types[static_cast<size_t>(_vertexDecl[i]._type)];
		if (_vertexDecl[i]._asInt)
		{
			glVertexAttribIPointer(i, _vertexDecl[i]._num, type, _strideBytes, _vertexDeclOffsets[i]);
		}
		else
		{
			glVertexAttribPointer(i, _vertexDecl[i]._num, type, _vertexDecl[i]._normalized, _strideBytes, _vertexDeclOffsets[i]);
		}

		glEnableVertexAttribArray(i);
	}
}
