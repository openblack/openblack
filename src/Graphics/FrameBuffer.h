/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <memory>
#include <optional>

#include "RenderPass.h"
#include "Texture2D.h"

namespace openblack::graphics
{

class FrameBuffer
{
public:
	FrameBuffer() = delete;
	FrameBuffer(std::string&& name, uint16_t width, uint16_t height, Format colorFormat,
	            std::optional<Format> depthStencilFormat = {});
	~FrameBuffer();

	void Bind(RenderPass viewId) const;

	[[nodiscard]] const Texture2D& GetColorAttachment() const { return _colorAttachment; }
	void GetSize(uint16_t& width, uint16_t& height) const { width = _width, height = _height; }

private:
	std::string _name;
	bgfx::FrameBufferHandle _handle;

	uint16_t _width;
	uint16_t _height;
	Format _colorFormat;
	std::optional<Format> _depthStencilFormat;

	Texture2D _colorAttachment;
	Texture2D _depthStencilAttachment;
};

} // namespace openblack::graphics
