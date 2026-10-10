/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cassert>
#include <cstdint>

#include <tuple>
#include <utility>
#include <vector>

#include <SDL3/SDL_gpu.h>

#include "GraphicsHandle.h"
#include "Texture2d.h"

namespace openblack::graphics
{
template <typename T>
inline T* toSdlPointer(uint64_t id)
{
	// NOLINTNEXTLINE(performance-no-int-to-ptr): handles store SDL GPU object pointers
	return reinterpret_cast<T*>(id);
}

inline SDL_GPUBuffer* toSdl(IndexBufferHandle handle)
{
	return toSdlPointer<SDL_GPUBuffer>(handle.id);
}

template <typename T>
inline T fromSdl(SDL_GPUBuffer* buffer)
{
	return {reinterpret_cast<uintptr_t>(buffer)};
}

inline SDL_GPUBuffer* toSdl(VertexBufferHandle handle)
{
	return toSdlPointer<SDL_GPUBuffer>(handle.id);
}

inline SDL_GPUTexture* toSdl(TextureHandle handle)
{
	return toSdlPointer<SDL_GPUTexture>(handle.id);
}

inline TextureHandle fromSdl(SDL_GPUTexture* texture)
{
	return {static_cast<uint64_t>(reinterpret_cast<uintptr_t>(texture))};
}

inline SDL_GPUSampler* toSdl(SamplerHandle handle)
{
	return toSdlPointer<SDL_GPUSampler>(handle.id);
}

inline SamplerHandle fromSdl(SDL_GPUSampler* sampler)
{
	return {reinterpret_cast<uintptr_t>(sampler)};
}

inline SDL_GPUSamplerAddressMode toSdl(Wrapping wrapping)
{
	switch (wrapping)
	{
	case Wrapping::ClampEdge:
		return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	case Wrapping::Repeat:
		return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	case Wrapping::MirroredRepeat:
		return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
	default:
		assert(false);
		std::unreachable();
	}
}

inline Wrapping fromSdl(SDL_GPUSamplerAddressMode addressMode)
{
	switch (addressMode)
	{
	case SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE:
		return Wrapping::ClampEdge;
	case SDL_GPU_SAMPLERADDRESSMODE_REPEAT:
		return Wrapping::Repeat;
	case SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT:
		return Wrapping::MirroredRepeat;
	default:
		assert(false);
		std::unreachable();
	}
}

inline SDL_GPUFilter toSdl(Filter filter)
{
	switch (filter)
	{
	case Filter::Nearest:
		return SDL_GPU_FILTER_NEAREST;
	case Filter::Linear:
		return SDL_GPU_FILTER_LINEAR;
	default:
		assert(false);
		std::unreachable();
	}
}

inline Filter fromSdl(SDL_GPUFilter filter)
{
	switch (filter)
	{
	case SDL_GPU_FILTER_NEAREST:
		return Filter::Nearest;
	case SDL_GPU_FILTER_LINEAR:
		return Filter::Linear;
	default:
		assert(false);
		std::unreachable();
	}
}

inline SDL_GPUTextureFormat toSdl(TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::BlockCompression1:
		return SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM;
	case TextureFormat::BlockCompression2:
		return SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM;
	case TextureFormat::BlockCompression3:
		return SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM;
	case TextureFormat::Depth24Stencil8:
		return SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
	case TextureFormat::Depth32Stencil8:
		return SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
	case TextureFormat::DepthComponent16:
		return SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	case TextureFormat::DepthComponent24:
		return SDL_GPU_TEXTUREFORMAT_D24_UNORM;
	case TextureFormat::BGR5A1:
		return SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM;
	case TextureFormat::BGRA4:
		return SDL_GPU_TEXTUREFORMAT_B4G4R4A4_UNORM;
	case TextureFormat::A8:
		return SDL_GPU_TEXTUREFORMAT_A8_UNORM;
	case TextureFormat::R8:
		return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
	case TextureFormat::R8I:
		return SDL_GPU_TEXTUREFORMAT_R8_INT;
	case TextureFormat::R8UI:
		return SDL_GPU_TEXTUREFORMAT_R8_UINT;
	case TextureFormat::R8SNorm:
		return SDL_GPU_TEXTUREFORMAT_R8_SNORM;
	case TextureFormat::RG8:
		return SDL_GPU_TEXTUREFORMAT_R8G8_UNORM;
	case TextureFormat::RG8I:
		return SDL_GPU_TEXTUREFORMAT_R8G8_INT;
	case TextureFormat::RG8UI:
		return SDL_GPU_TEXTUREFORMAT_R8G8_UINT;
	case TextureFormat::RG8SNorm:
		return SDL_GPU_TEXTUREFORMAT_R8G8_SNORM;
	case TextureFormat::RGB8I:
		return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_INT;
	case TextureFormat::RGB8UI:
		return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT;
	case TextureFormat::RGBA8:
		return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	case TextureFormat::RGBA8I:
		return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_INT;
	case TextureFormat::RGBA8UI:
		return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT;
	case TextureFormat::RGBA8SNorm:
		return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM;
	case TextureFormat::BGRA8:
		return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
	case TextureFormat::RGB10A2:
		return SDL_GPU_TEXTUREFORMAT_R10G10B10A2_UNORM;
	default:
		assert(false);
		std::unreachable();
	}
}

inline TextureFormat fromSdl(SDL_GPUTextureFormat format)

{
	switch (format)
	{
	case SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM:
		return TextureFormat::BlockCompression1;
	case SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM:
		return TextureFormat::BlockCompression2;
	case SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM:
		return TextureFormat::BlockCompression3;
	case SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT:
		return TextureFormat::Depth24Stencil8;
	case SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT:
		return TextureFormat::Depth32Stencil8;
	case SDL_GPU_TEXTUREFORMAT_D16_UNORM:
		return TextureFormat::DepthComponent16;
	case SDL_GPU_TEXTUREFORMAT_D24_UNORM:
		return TextureFormat::DepthComponent24;
	case SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM:
		return TextureFormat::BGR5A1;
	case SDL_GPU_TEXTUREFORMAT_B4G4R4A4_UNORM:
		return TextureFormat::BGRA4;
	case SDL_GPU_TEXTUREFORMAT_A8_UNORM:
		return TextureFormat::A8;
	case SDL_GPU_TEXTUREFORMAT_R8_UNORM:
		return TextureFormat::R8;
	case SDL_GPU_TEXTUREFORMAT_R8_INT:
		return TextureFormat::R8I;
	case SDL_GPU_TEXTUREFORMAT_R8_UINT:
		return TextureFormat::R8UI;
	case SDL_GPU_TEXTUREFORMAT_R8_SNORM:
		return TextureFormat::R8SNorm;
	case SDL_GPU_TEXTUREFORMAT_R8G8_UNORM:
		return TextureFormat::RG8;
	case SDL_GPU_TEXTUREFORMAT_R8G8_INT:
		return TextureFormat::RG8I;
	case SDL_GPU_TEXTUREFORMAT_R8G8_UINT:
		return TextureFormat::RG8UI;
	case SDL_GPU_TEXTUREFORMAT_R8G8_SNORM:
		return TextureFormat::RG8SNorm;
	case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM:
		return TextureFormat::RGBA8;
	case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_INT:
		return TextureFormat::RGBA8I;
	case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT:
		return TextureFormat::RGBA8UI;
	case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM:
		return TextureFormat::RGBA8SNorm;
	case SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM:
		return TextureFormat::BGRA8;
	case SDL_GPU_TEXTUREFORMAT_R10G10B10A2_UNORM:
		return TextureFormat::RGB10A2;
	default:
		assert(false);
		std::unreachable();
	}
}
inline SDL_GPUTextureFormat GetSupportedDepthStencilFormat(SDL_GPUDevice* device, TextureFormat format)
{
	const auto preferred = toSdl(format);
	if (format == TextureFormat::Depth24Stencil8 &&
	    !SDL_GPUTextureSupportsFormat(device, preferred, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
	{
		return SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
	}
	return preferred;
}
} // namespace openblack::graphics
