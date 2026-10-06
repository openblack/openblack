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

#include <array>
#include <bitset>

#include "GraphicsHandle.h"
#include "ShaderSamplers.h"

namespace openblack::graphics
{

/// Keeps every sampler a draw's program declares bound. The draws set their textures through their programs, which
/// note each stage set here; when a program submits, the samplers it declares that are still unset get a white
/// texture of their kind. A stage set by a draw stays set, as bgfx keeps it, until a submit or a discard drops the
/// bindings, or the frame ends.
class SamplerDefaults
{
public:
	SamplerDefaults();
	~SamplerDefaults();
	SamplerDefaults(const SamplerDefaults&) = delete;
	SamplerDefaults& operator=(const SamplerDefaults&) = delete;

	[[nodiscard]] TextureHandle Texture(shader_samplers::DefaultTexture texture) const;

	/// A texture was bound at this stage
	void Set(uint8_t stage);
	[[nodiscard]] bool IsSet(uint8_t stage) const;
	/// A submit or discard with these bgfx discard flags happened: the bindings went if the flags drop them
	void Discarded(uint8_t flags);
	/// bgfx drops every binding at the end of a frame
	void FrameEnded();

private:
	std::array<TextureHandle, 4> _textures;
	std::bitset<shader_samplers::k_MaxStages> _set;
};

} // namespace openblack::graphics
