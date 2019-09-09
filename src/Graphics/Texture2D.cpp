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

#include <Graphics/OpenGL.h>
#include <Graphics/Texture2D.h>

#include <algorithm>

namespace openblack::graphics
{
Texture2D::Texture2D()
{
	GLuint texture;
	glGenTextures(1, &texture);
	_handle = static_cast<unsigned int>(texture);
}

Texture2D::~Texture2D()
{
	if (_handle)
	{
		auto texture = static_cast<GLuint>(_handle);
		glDeleteTextures(1, &texture);
	}
}

void Texture2D::Create(uint32_t width, uint32_t height, uint32_t layers)
{
	auto bindPoint = layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

	glBindTexture(bindPoint, static_cast<GLuint>(_handle));

	if (layers == 1)
		glTexImage2D(bindPoint, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	else
		glTexImage3D(bindPoint, 0, GL_RGBA, width, height, layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glTexParameteri(bindPoint, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(bindPoint, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture2D::Create(const void* data, DataType type, Format format, uint32_t width, uint32_t height, uint32_t layers, InternalFormat internalFormat)
{
	auto bindPoint = layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

	glBindTexture(bindPoint, static_cast<GLuint>(_handle));

	if (layers == 1)
		glTexImage2D(bindPoint, 0, static_cast<GLint>(internalFormat), width, height, 0, static_cast<GLenum>(format), static_cast<GLenum>(type), data);
	else
		glTexImage3D(bindPoint, 0, static_cast<GLint>(internalFormat), width, height, layers, 0, static_cast<GLenum>(format), static_cast<GLenum>(type), data);

	glTexParameteri(bindPoint, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(bindPoint, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(bindPoint, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(bindPoint, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	_width = width;
	_height = height;
	_layers = layers;
}

void Texture2D::CreateCompressed(const void *data, uint32_t width, uint32_t height, uint32_t layers, InternalFormat internalFormat)
{
	auto bindPoint = layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

	// DXT1 = 8bpp or DXT3 = 16bpp
	int bpp = internalFormat == InternalFormat::CompressedRGBAS3TCDXT3 ? 16 : 8;

	_width = width;
	_height = height;
	_layers = layers;

	GLsizei size = std::max(1, ((int) width + 3) >> 2) * std::max(1, ((int) height + 3) >> 2) * bpp * layers;

	glBindTexture(bindPoint, static_cast<GLuint>(_handle));

	if (layers == 1)
		glCompressedTexImage2D(bindPoint, 0, static_cast<GLenum>(internalFormat), width, height, 0, size, data);
	else
		glCompressedTexImage3D(bindPoint, 0, static_cast<GLenum>(internalFormat), width, height, layers, 0, size, data);

	glTexParameteri(bindPoint, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(bindPoint, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(bindPoint, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(bindPoint, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

}

void Texture2D::Bind() const
{
	auto bindPoint = _layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	auto texture = static_cast<GLuint>(_handle);
	glBindTexture(bindPoint, texture);
}

void Texture2D::GenerateMipmap()
{
	auto bindPoint = _layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

	auto texture = static_cast<GLuint>(_handle);
	glBindTexture(bindPoint, texture);
	glGenerateMipmap(bindPoint);
}

} // namespace openblack::Graphics
