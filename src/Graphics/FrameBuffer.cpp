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

using namespace openblack::graphics;

FrameBuffer::FrameBuffer(uint16_t width, uint16_t height, Format colorFormat, std::optional<Format> depthStencilFormat)
	: _handle(BGFX_INVALID_HANDLE)
	, _width(width)
	, _height(height)
	, _colorFormat(colorFormat)
	, _depthStencilFormat(depthStencilFormat)
{
	_colorAttachment._bgfxHandle = BGFX_INVALID_HANDLE;
	_colorAttachment._width = width;
	_colorAttachment._height = height;
	_colorAttachment._layers = 1;

	_depthStencilAttachment._bgfxHandle = BGFX_INVALID_HANDLE;
	_depthStencilAttachment._width = width;
	_depthStencilAttachment._height = height;
	_depthStencilAttachment._layers = 1;

	if (depthStencilFormat)
	{
		_handle = bgfx::createFrameBuffer(_width, _height, getBgfxTextureFormat(colorFormat), getBgfxTextureFormat(depthStencilFormat.value()));
		_colorAttachment._bgfxHandle = bgfx::getTexture(_handle, 0);
		_depthStencilAttachment._bgfxHandle = bgfx::getTexture(_handle, 1);
	}
	else
	{
		_handle = bgfx::createFrameBuffer(_width, _height, getBgfxTextureFormat(colorFormat));
		_colorAttachment._bgfxHandle = bgfx::getTexture(_handle, 0);
	}

	assert(bgfx::isValid(_handle));
}

FrameBuffer::~FrameBuffer()
{
	if (bgfx::isValid(_handle))
	{
		bgfx::destroy(_handle);
	}
}

void FrameBuffer::Bind()
{
	bgfx::setViewFrameBuffer(0, _handle);
}

void FrameBuffer::Unbind()
{
	bgfx::setViewFrameBuffer(0, BGFX_INVALID_HANDLE);
}
