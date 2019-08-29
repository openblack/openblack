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

#include <Graphics/Texture2DArray.h>
#include <stdio.h>

namespace OpenBlack::Graphics
{

Texture2DArray::Texture2DArray()
{
	GLuint texture;
	glGenTextures(1, &texture);
	_texture = static_cast<unsigned int>(texture);
}

Texture2DArray::~Texture2DArray()
{
	if (_texture)
	{
		GLuint texture = static_cast<GLuint>(_texture);
		glDeleteTextures(1, &_texture);
	}
}

void Texture2DArray::Create(unsigned int width, unsigned int height, unsigned int depth, InternalFormat internalFormat)
{
	_width = width;
	_height = height;
	_depth  = depth;

	glBindTexture(GL_TEXTURE_2D_ARRAY, _texture);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, static_cast<GLenum>(internalFormat), _width, _height, _depth);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Texture2DArray::SetTexture(unsigned int layer, unsigned int width, unsigned int height, Format format, DataType type, const void* textureData)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, _texture);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, width, height, 1, static_cast<GLenum>(format), static_cast<GLenum>(type), textureData);
}

void Texture2DArray::Bind() const
{
	if (_texture)
	{
		GLuint texture = static_cast<GLuint>(_texture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}
}

} // namespace OpenBlack::Graphics
