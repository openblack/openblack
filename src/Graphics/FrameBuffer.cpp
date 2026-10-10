/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FrameBuffer.h"

#include <utility>

using namespace openblack::graphics;

FrameBuffer::FrameBuffer(std::string name, glm::u16vec2 resolution, TextureFormat colorFormat,
                         std::optional<TextureFormat> depthStencilFormat, TextureHandle colorAttachment,
                         SamplerHandle colorSampler, TextureHandle depthStencilAttachment,
                         std::function<void()> release) noexcept
    : name(std::move(name))
    , resolution(resolution)
    , colorFormat(colorFormat)
    , depthStencilFormat(depthStencilFormat)
    , colorAttachment(colorAttachment)
    , colorSampler(colorSampler)
    , depthStencilAttachment(depthStencilAttachment)
    , _release(std::move(release))
{
}

FrameBuffer::~FrameBuffer() noexcept
{
	_release();
}
