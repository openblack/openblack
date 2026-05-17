/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FrameBuffer.h"

#include <cassert>

#include <array>

#include <bgfx/bgfx.h>

#include "GraphicsHandleBgfx.h"

using namespace openblack::graphics;

FrameBuffer::FrameBuffer(std::string&& name, uint16_t width, uint16_t height, TextureFormat colorFormat,
                         std::optional<TextureFormat> depthStencilFormat)
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
	_colorAttachment._resolution = glm::u16vec2(width, height);
	_colorAttachment._numLayers = 1;

	_depthStencilAttachment._handle = BGFX_INVALID_HANDLE;
	_depthStencilAttachment._resolution = glm::u16vec2(width, height);
	_depthStencilAttachment._numLayers = 1;

	if (depthStencilFormat)
	{
		std::array<bgfx::TextureHandle, 2> textures = {
		    bgfx::createTexture2D(width, height, false, 1, toBgfx(colorFormat), BGFX_TEXTURE_RT),
		    bgfx::createTexture2D(width, height, false, 1, toBgfx(depthStencilFormat.value()), BGFX_TEXTURE_RT),
		};
		_handle = fromBgfx(bgfx::createFrameBuffer(static_cast<uint8_t>(textures.size()), textures.data()));
		_colorAttachment._handle = fromBgfx(bgfx::getTexture(toBgfx(_handle), 0));
		_depthStencilAttachment._handle = fromBgfx(bgfx::getTexture(toBgfx(_handle), 1));
	}
	else
	{
		_handle = fromBgfx(bgfx::createFrameBuffer(_width, _height, toBgfx(colorFormat), BGFX_TEXTURE_RT));
		_colorAttachment._handle = fromBgfx(bgfx::getTexture(toBgfx(_handle), 0));
	}

	assert(bgfx::isValid(toBgfx(_handle)));
}

FrameBuffer::~FrameBuffer()
{
	if (bgfx::isValid(toBgfx(_handle)))
	{
		bgfx::destroy(toBgfx(_handle));
	}
}

void FrameBuffer::Bind(RenderPass viewId) const
{
	bgfx::setViewFrameBuffer(static_cast<uint8_t>(viewId), toBgfx(_handle));
}
