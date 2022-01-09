/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "FrameBuffer.h"

#include <array>
#include <cassert>

using namespace openblack::graphics;

FrameBuffer::FrameBuffer(const std::string& name, uint16_t width, uint16_t height, Format colorFormat,
                         std::optional<Format> depthStencilFormat)
    : _name(std::move(name))
    , _handle(BGFX_INVALID_HANDLE)
    , _width(width)
    , _height(height)
    , _colorFormat(colorFormat)
    , _depthStencilFormat(depthStencilFormat)
    , _colorAttachment(_name + "_color")
    , _depthStencilAttachment(_name + "_depthStencil")
{
	_colorAttachment._handle = BGFX_INVALID_HANDLE;
	_colorAttachment._info.width = width;
	_colorAttachment._info.height = height;
	_colorAttachment._info.numLayers = 1;

	_depthStencilAttachment._handle = BGFX_INVALID_HANDLE;
	_depthStencilAttachment._info.width = width;
	_depthStencilAttachment._info.height = height;
	_depthStencilAttachment._info.numLayers = 1;

	if (depthStencilFormat)
	{
		std::array<bgfx::TextureHandle, 2> textures = {
		    bgfx::createTexture2D(width, height, false, 1, getBgfxTextureFormat(colorFormat), BGFX_TEXTURE_RT),
		    bgfx::createTexture2D(width, height, false, 1, getBgfxTextureFormat(depthStencilFormat.value()), BGFX_TEXTURE_RT),
		};
		_handle = bgfx::createFrameBuffer(static_cast<uint8_t>(textures.size()), textures.data());
		_colorAttachment._handle = bgfx::getTexture(_handle, 0);
		_depthStencilAttachment._handle = bgfx::getTexture(_handle, 1);
	}
	else
	{
		_handle = bgfx::createFrameBuffer(_width, _height, getBgfxTextureFormat(colorFormat), BGFX_TEXTURE_RT);
		_colorAttachment._handle = bgfx::getTexture(_handle, 0);
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

void FrameBuffer::Bind(RenderPass viewId) const
{
	bgfx::setViewFrameBuffer(static_cast<uint8_t>(viewId), _handle);
}
