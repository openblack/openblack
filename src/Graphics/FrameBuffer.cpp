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

#include <Graphics/FrameBuffer.h>
#include <cassert>
#include <stdexcept>

namespace openblack::Graphics
{

FrameBuffer::FrameBuffer(GLsizei width, GLsizei height, GLenum format) :
    _handle(0), _width(width), _height(height), _format(format)
{
	glGenFramebuffers(1, &_handle);
	assert(_handle);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _handle);

	_texture = new Texture2D();
	_texture->Create(_width, _height);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture->GetNativeHandle(), 0);

	if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("failed to create framebuffer");

	// unbind it (restore state in future?)
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer()
{
	if (_handle != 0)
		glDeleteFramebuffers(1, &_handle);

	if (_texture != nullptr)
		delete _texture;
}

}
