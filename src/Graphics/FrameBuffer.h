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

#pragma once

#include <Graphics/OpenGL.h>
#include <Graphics/Texture2D.h>

namespace openblack {
namespace Graphics {

class FrameBuffer {
public:
	FrameBuffer() = delete;
	FrameBuffer(GLsizei width, GLsizei height, GLenum format);
	~FrameBuffer();

	void Bind() { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _handle); }
	void Unbind() { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); }

	Texture2D* GetTexture() { return _texture; }

	//inline void Bind() { glBindTexture(GL_TEXTURE_RECTANGLE, _textureID); }

	GLsizei GetWidth() const { return _width; }
	GLsizei GetHeight() const { return _height; }
private:
	GLuint _handle;

	GLsizei _width;
	GLsizei _height;
	GLenum _format;

	Texture2D* _texture;
};

}
}
