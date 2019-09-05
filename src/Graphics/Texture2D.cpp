/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Graphics/OpenGL.h>
#include <Graphics/Texture2D.h>

#include <algorithm>
#include <cassert>

namespace OpenBlack::Graphics
{
Texture2D::Texture2D()
{
	GLuint texture;
	glGenTextures(1, &texture);
	_texture = static_cast<unsigned int>(texture);
}

Texture2D::~Texture2D()
{
	if (_texture)
	{
		GLuint texture = static_cast<GLuint>(_texture);
		glDeleteTextures(1, &_texture);
	}
}

void Texture2D::Create(unsigned int width, unsigned int height)
{
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(_texture));

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture2D::Create(const void* data, DataType type, Format format, unsigned int width, unsigned int height, InternalFormat internalFormat)
{
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(_texture));
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), width, height, 0, static_cast<GLenum>(format), static_cast<GLenum>(type), data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	_width = width;
	_height = height;
}

void Texture2D::CreateCompressed(const void* data, unsigned int width, unsigned int height, InternalFormat internalFormat)
{
	// DXT1 = 8bpp or DXT3 = 16bpp
	int bpp = internalFormat == InternalFormat::CompressedRGBAS3TCDXT3 ? 16 : 8;

	GLsizei size = std::max(1, ((int)width + 3) >> 2) * std::max(1, ((int)height + 3) >> 2) * bpp;

	_width = width;
	_height = height;

	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(_texture));
	glCompressedTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(internalFormat), width, height, 0, size, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture2D::Bind() const
{
	if (_texture)
	{
		GLuint texture = static_cast<GLuint>(_texture);
		glBindTexture(GL_TEXTURE_2D, texture);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void Texture2D::GenerateMipmap()
{
	if (!_texture)
		return;

	GLuint texture = static_cast<GLuint>(_texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glGenerateMipmap(GL_TEXTURE_2D);
}

} // namespace OpenBlack::Graphics
