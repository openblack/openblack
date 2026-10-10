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
#include <functional>
#include <memory>
#include <string>

#include <glm/vec2.hpp>

#include "GraphicsHandle.h"

namespace openblack::graphics
{

enum class TextureFormat : uint8_t
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
	A8,
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
	B5G6R5,
	R5G6B5,
	BGR5A1,
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
	BGRA4,
	RGBA4,
	_COUNT
};

constexpr std::array<const char*, static_cast<size_t>(TextureFormat::_COUNT)> k_TextureFormatStrings = {{
    "BlockCompression1",
    "BlockCompression2",
    "BlockCompression3",
    "Depth24Stencil8",
    "DepthComponent16",
    "DepthComponent24",
    "R16F",
    "R16I",
    "R16SNorm",
    "R16UI",
    "R32F",
    "R32I",
    "R32UI",
    "A8",
    "R8",
    "R8I",
    "R8SNorm",
    "R8UI",
    "RG16",
    "RG16F",
    "RG16SNorm",
    "RG32F",
    "RG32I",
    "RG32UI",
    "RG8",
    "RG8I",
    "RG8SNorm",
    "RG8UI",
    "RGB10A2",
    "B5G6R5",
    "R5G6B5",
    "BGR5A1",
    "RGB5A1",
    "RGB8",
    "RGB8I",
    "RGB8UI",
    "RGB9E5",
    "RGBA8",
    "RGBA8I",
    "RGBA8UI",
    "RGBA8SNorm",
    "BGRA8",
    "RGBA16",
    "RGBA16F",
    "RGBA16I",
    "RGBA16UI",
    "RGBA16SNorm",
    "RGBA32F",
    "RGBA32I",
    "RGBA32UI",
    "BGRA4",
    "RGBA4",
}};

enum class Filter : uint8_t
{
	Nearest,
	Linear,
};

enum class Wrapping : uint8_t
{
	ClampEdge,
	Repeat,
	MirroredRepeat,
};

struct Texture2d
{
	const std::string name;
	const TextureHandle handle;
	const SamplerHandle samplerHandle;
	const glm::u16vec2 resolution;
	const uint16_t numLayers;
	const TextureFormat format;
};

using Texture2dUniquePtr = std::unique_ptr<Texture2d, std::function<void(Texture2d*)>>;

} // namespace openblack::graphics
