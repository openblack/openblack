/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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

#pragma once
#ifndef OPENBLACK_GRAPHICS_TEXTURE2DARRAY_H
#define OPENBLACK_GRAPHICS_TEXTURE2DARRAY_H

#include <Graphics/Texture2D.h>
#include <vector>

namespace OpenBlack {
namespace Graphics {

class Texture2DArray : public Texture {
public:
	Texture2DArray() = delete;
	Texture2DArray(GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat);

	void SetTexture(GLsizei layer, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* textureData);

	inline void Bind() {
		glBindTexture(GL_TEXTURE_2D_ARRAY, _textureID);
	}

	inline void Bind(unsigned char slot) {
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D_ARRAY, _textureID);
	}

	const GLsizei GetWidth() const { return _width; }
	const GLsizei GetHeight() const { return _height; }
private:
	GLsizei _width;
	GLsizei _height;
	GLsizei _depth;
	GLenum _internalFormat;
};

}
}

#endif
