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
#include <utility>

#include <bgfx/bgfx.h>

#include "GraphicsHandle.h"
#include "Texture2D.h"

namespace openblack::graphics
{

inline bgfx::IndexBufferHandle toBgfx(IndexBufferHandle handle)
{
	return {static_cast<uint16_t>(handle.id)};
}

inline IndexBufferHandle fromBgfx(bgfx::IndexBufferHandle handle)
{
	return {handle.idx};
}

inline bgfx::FrameBufferHandle toBgfx(FrameBufferHandle handle)
{
	return {static_cast<uint16_t>(handle.id)};
}

inline FrameBufferHandle fromBgfx(bgfx::FrameBufferHandle handle)
{
	return {handle.idx};
}

inline bgfx::VertexBufferHandle toBgfx(VertexBufferHandle handle)
{
	return {static_cast<uint16_t>(handle.id)};
}

inline VertexBufferHandle fromBgfx(bgfx::VertexBufferHandle handle)
{
	return {handle.idx};
}

inline bgfx::VertexLayoutHandle toBgfx(VertexLayoutHandle handle)
{
	return {static_cast<uint16_t>(handle.id)};
}

inline VertexLayoutHandle fromBgfx(bgfx::VertexLayoutHandle handle)
{
	return {handle.idx};
}

inline bgfx::DynamicVertexBufferHandle toBgfx(DynamicVertexBufferHandle handle)
{
	return {static_cast<uint16_t>(handle.id)};
}

inline DynamicVertexBufferHandle fromBgfx(bgfx::DynamicVertexBufferHandle handle)
{
	return {handle.idx};
}

inline bgfx::TextureHandle toBgfx(TextureHandle handle)
{
	return {static_cast<uint16_t>(handle.id)};
}
inline TextureHandle fromBgfx(bgfx::TextureHandle handle)
{
	return {handle.idx};
}

inline bgfx::TextureFormat::Enum toBgfx(TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::BlockCompression1:
		return bgfx::TextureFormat::BC1;
	case TextureFormat::BlockCompression2:
		return bgfx::TextureFormat::BC2;
	case TextureFormat::BlockCompression3:
		return bgfx::TextureFormat::BC3;
	case TextureFormat::Depth24Stencil8:
		return bgfx::TextureFormat::D24S8;
	case TextureFormat::DepthComponent16:
		return bgfx::TextureFormat::D16;
	case TextureFormat::DepthComponent24:
		return bgfx::TextureFormat::D24;
	case TextureFormat::R16F:
		return bgfx::TextureFormat::R16F;
	case TextureFormat::R16I:
		return bgfx::TextureFormat::R16I;
	case TextureFormat::R16SNorm:
		return bgfx::TextureFormat::R16S;
	case TextureFormat::R16UI:
		return bgfx::TextureFormat::R16U;
	case TextureFormat::R32F:
		return bgfx::TextureFormat::R32F;
	case TextureFormat::R32I:
		return bgfx::TextureFormat::R32I;
	case TextureFormat::R32UI:
		return bgfx::TextureFormat::R32U;
	case TextureFormat::R8:
		return bgfx::TextureFormat::R8;
	case TextureFormat::R8I:
		return bgfx::TextureFormat::R8I;
	case TextureFormat::R8SNorm:
		return bgfx::TextureFormat::R8S;
	case TextureFormat::R8UI:
		return bgfx::TextureFormat::R8U;
	case TextureFormat::RG16:
		return bgfx::TextureFormat::RG16;
	case TextureFormat::RG16F:
		return bgfx::TextureFormat::RG16F;
	case TextureFormat::RG16SNorm:
		return bgfx::TextureFormat::RG16S;
	case TextureFormat::RG32F:
		return bgfx::TextureFormat::RG32F;
	case TextureFormat::RG32I:
		return bgfx::TextureFormat::RG32I;
	case TextureFormat::RG32UI:
		return bgfx::TextureFormat::RG32U;
	case TextureFormat::RG8:
		return bgfx::TextureFormat::RG8;
	case TextureFormat::RG8I:
		return bgfx::TextureFormat::RG8I;
	case TextureFormat::RG8SNorm:
		return bgfx::TextureFormat::RG8S;
	case TextureFormat::RG8UI:
		return bgfx::TextureFormat::RG8U;
	case TextureFormat::RGB10A2:
		return bgfx::TextureFormat::RGB10A2;
	case TextureFormat::B5G6R5:
		return bgfx::TextureFormat::B5G6R5;
	case TextureFormat::R5G6B5:
		return bgfx::TextureFormat::R5G6B5;
	case TextureFormat::BGR5A1:
		return bgfx::TextureFormat::BGR5A1;
	case TextureFormat::RGB5A1:
		return bgfx::TextureFormat::RGB5A1;
	case TextureFormat::RGB8:
		return bgfx::TextureFormat::RGB8;
	case TextureFormat::RGB8I:
		return bgfx::TextureFormat::RGB8I;
	case TextureFormat::RGB8UI:
		return bgfx::TextureFormat::RGB8U;
	case TextureFormat::RGB9E5:
		return bgfx::TextureFormat::RGB9E5F;
	case TextureFormat::RGBA8:
		return bgfx::TextureFormat::RGBA8;
	case TextureFormat::RGBA8I:
		return bgfx::TextureFormat::RGBA8I;
	case TextureFormat::RGBA8UI:
		return bgfx::TextureFormat::RGBA8U;
	case TextureFormat::RGBA8SNorm:
		return bgfx::TextureFormat::RGBA8S;
	case TextureFormat::BGRA8:
		return bgfx::TextureFormat::BGRA8;
	case TextureFormat::RGBA16:
		return bgfx::TextureFormat::RGBA16;
	case TextureFormat::RGBA16F:
		return bgfx::TextureFormat::RGBA16F;
	case TextureFormat::RGBA16I:
		return bgfx::TextureFormat::RGBA16I;
	case TextureFormat::RGBA16UI:
		return bgfx::TextureFormat::RGBA16U;
	case TextureFormat::RGBA16SNorm:
		return bgfx::TextureFormat::RGBA16S;
	case TextureFormat::RGBA32F:
		return bgfx::TextureFormat::RGBA32F;
	case TextureFormat::RGBA32I:
		return bgfx::TextureFormat::RGBA32I;
	case TextureFormat::RGBA32UI:
		return bgfx::TextureFormat::RGBA32U;
	case TextureFormat::BGRA4:
		return bgfx::TextureFormat::BGRA4;
	case TextureFormat::RGBA4:
		return bgfx::TextureFormat::RGBA4;
	default:
		assert(false);
		std::unreachable();
	};
}

inline TextureFormat fromBgfx(bgfx::TextureFormat::Enum format)
{
	switch (format)
	{
	case bgfx::TextureFormat::BC1:
		return TextureFormat::BlockCompression1;
	case bgfx::TextureFormat::BC2:
		return TextureFormat::BlockCompression2;
	case bgfx::TextureFormat::BC3:
		return TextureFormat::BlockCompression3;
	case bgfx::TextureFormat::D24S8:
		return TextureFormat::Depth24Stencil8;
	case bgfx::TextureFormat::D16:
		return TextureFormat::DepthComponent16;
	case bgfx::TextureFormat::D24:
		return TextureFormat::DepthComponent24;
	case bgfx::TextureFormat::R16F:
		return TextureFormat::R16F;
	case bgfx::TextureFormat::R16I:
		return TextureFormat::R16I;
	case bgfx::TextureFormat::R16S:
		return TextureFormat::R16SNorm;
	case bgfx::TextureFormat::R16U:
		return TextureFormat::R16UI;
	case bgfx::TextureFormat::R32F:
		return TextureFormat::R32F;
	case bgfx::TextureFormat::R32I:
		return TextureFormat::R32I;
	case bgfx::TextureFormat::R32U:
		return TextureFormat::R32UI;
	case bgfx::TextureFormat::A8:
	case bgfx::TextureFormat::R8:
		return TextureFormat::R8;
	case bgfx::TextureFormat::R8I:
		return TextureFormat::R8I;
	case bgfx::TextureFormat::R8S:
		return TextureFormat::R8SNorm;
	case bgfx::TextureFormat::R8U:
		return TextureFormat::R8UI;
	case bgfx::TextureFormat::RG16:
		return TextureFormat::RG16;
	case bgfx::TextureFormat::RG16F:
		return TextureFormat::RG16F;
	case bgfx::TextureFormat::RG16S:
		return TextureFormat::RG16SNorm;
	case bgfx::TextureFormat::RG32F:
		return TextureFormat::RG32F;
	case bgfx::TextureFormat::RG32I:
		return TextureFormat::RG32I;
	case bgfx::TextureFormat::RG32U:
		return TextureFormat::RG32UI;
	case bgfx::TextureFormat::RG8:
		return TextureFormat::RG8;
	case bgfx::TextureFormat::RG8I:
		return TextureFormat::RG8I;
	case bgfx::TextureFormat::RG8S:
		return TextureFormat::RG8SNorm;
	case bgfx::TextureFormat::RG8U:
		return TextureFormat::RG8UI;
	case bgfx::TextureFormat::RGB10A2:
		return TextureFormat::RGB10A2;
	case bgfx::TextureFormat::B5G6R5:
		return TextureFormat::B5G6R5;
	case bgfx::TextureFormat::R5G6B5:
		return TextureFormat::R5G6B5;
	case bgfx::TextureFormat::BGR5A1:
		return TextureFormat::BGR5A1;
	case bgfx::TextureFormat::RGB5A1:
		return TextureFormat::RGB5A1;
	case bgfx::TextureFormat::RGB8:
		return TextureFormat::RGB8;
	case bgfx::TextureFormat::RGB8I:
		return TextureFormat::RGB8I;
	case bgfx::TextureFormat::RGB8U:
		return TextureFormat::RGB8UI;
	case bgfx::TextureFormat::RGB9E5F:
		return TextureFormat::RGB9E5;
	case bgfx::TextureFormat::RGBA8:
		return TextureFormat::RGBA8;
	case bgfx::TextureFormat::RGBA8I:
		return TextureFormat::RGBA8I;
	case bgfx::TextureFormat::RGBA8U:
		return TextureFormat::RGBA8UI;
	case bgfx::TextureFormat::RGBA8S:
		return TextureFormat::RGBA8SNorm;
	case bgfx::TextureFormat::BGRA8:
		return TextureFormat::BGRA8;
	case bgfx::TextureFormat::RGBA16:
		return TextureFormat::RGBA16;
	case bgfx::TextureFormat::RGBA16F:
		return TextureFormat::RGBA16F;
	case bgfx::TextureFormat::RGBA16I:
		return TextureFormat::RGBA16I;
	case bgfx::TextureFormat::RGBA16U:
		return TextureFormat::RGBA16UI;
	case bgfx::TextureFormat::RGBA16S:
		return TextureFormat::RGBA16SNorm;
	case bgfx::TextureFormat::RGBA32F:
		return TextureFormat::RGBA32F;
	case bgfx::TextureFormat::RGBA32I:
		return TextureFormat::RGBA32I;
	case bgfx::TextureFormat::RGBA32U:
		return TextureFormat::RGBA32UI;
	case bgfx::TextureFormat::BGRA4:
		return TextureFormat::BGRA4;
	case bgfx::TextureFormat::RGBA4:
		return TextureFormat::RGBA4;
	default:
		assert(false);
		std::unreachable();
	}
}

} // namespace openblack::graphics
