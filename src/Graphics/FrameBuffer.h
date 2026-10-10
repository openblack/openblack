/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <functional>
#include <memory>
#include <optional>

#include "GraphicsHandle.h"
#include "Texture2d.h"

namespace openblack::graphics
{

struct FrameBuffer
{
	FrameBuffer(std::string name, glm::u16vec2 resolution, TextureFormat colorFormat,
	            std::optional<TextureFormat> depthStencilFormat, TextureHandle colorAttachment, SamplerHandle colorSampler,
	            TextureHandle depthStencilAttachment, std::function<void()> release) noexcept;
	~FrameBuffer() noexcept;

	FrameBuffer(const FrameBuffer&) = delete;
	FrameBuffer& operator=(const FrameBuffer&) = delete;
	FrameBuffer(FrameBuffer&&) = delete;
	FrameBuffer& operator=(FrameBuffer&&) = delete;

	const std::string name;
	const glm::u16vec2 resolution;
	const TextureFormat colorFormat;
	const std::optional<TextureFormat> depthStencilFormat;
	const TextureHandle colorAttachment;
	const SamplerHandle colorSampler;
	const TextureHandle depthStencilAttachment;

private:
	std::function<void()> _release;
};

} // namespace openblack::graphics
