/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <bgfx/bgfx.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace openblack::graphics
{

enum class Format : uint8_t
{
	BlockCompression1,
	BlockCompression2,
	BlockCompression3,
	Depth24Stencil8,
	DepthComponent16,
	DepthComponent24,
	R16F,
	R16I,
	R16SNorm,
	R16UI,
	R32F,
	R32I,
	R32UI,
	R8,
	R8I,
	R8SNorm,
	R8UI,
	RG16,
	RG16F,
	RG16SNorm,
	RG32F,
	RG32I,
	RG32UI,
	RG8,
	RG8I,
	RG8SNorm,
	RG8UI,
	RGB10A2,
	R5G6B5,
	RGB5A1,
	RGB8,
	RGB8I,
	RGB8UI,
	RGB9E5,
	RGBA8,
	RGBA8I,
	RGBA8UI,
	RGBA8SNorm,
	BGRA8,
	RGBA16,
	RGBA16F,
	RGBA16I,
	RGBA16UI,
	RGBA16SNorm,
	RGBA32F,
	RGBA32I,
	RGBA32UI,
	RGBA4,
};

enum class Filter : uint8_t
{
	Nearest,
	Linear,
	NearestMipmapNearest,
	LinearMipmapNearest,
	NearestMipmapLinear,
	LinearMipmapLinear,
};

enum class Wrapping : uint8_t
{
	ClampEdge,
	ClampBorder,
	Repeat,
	MirroredRepeat,
};

bgfx::TextureFormat::Enum getBgfxTextureFormat(Format format);

class FrameBuffer;

class Texture2D
{
public:
	explicit Texture2D(std::string name);
	~Texture2D();

	// No copying or assignment
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	void Create(uint16_t width, uint16_t height, uint16_t layers, Format format = Format::RGBA8,
	            Wrapping wrapping = Wrapping::ClampEdge, const void* data = nullptr, uint32_t size = 0);

	[[nodiscard]] const bgfx::TextureHandle& GetNativeHandle() const { return _handle; }
	[[nodiscard]] uint16_t GetWidth() const { return _info.width; }
	[[nodiscard]] uint16_t GetHeight() const { return _info.height; }
	[[nodiscard]] uint16_t GetLayerCount() const { return _info.numLayers; }

	void DumpTexture() const;

protected:
	std::string _name;
	bgfx::TextureHandle _handle;
	bgfx::TextureInfo _info;

	friend FrameBuffer;
};

} // namespace openblack::graphics
