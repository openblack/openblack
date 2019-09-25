/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <cstddef>

#include <bgfx/bgfx.h>
#include <string>

namespace openblack
{
namespace graphics
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

	void Create(uint16_t width, uint16_t height, uint16_t layers, Format format=Format::RGBA8, Wrapping wrapping=Wrapping::ClampEdge, const void* data=nullptr, size_t size=0);

	[[nodiscard]] uint32_t GetNativeHandle() const { return _handle; }
	[[nodiscard]] bgfx::TextureHandle GetBgfxHandle() const { return _bgfxHandle; }
	[[nodiscard]] uint16_t GetWidth() const { return _width; }
	[[nodiscard]] uint16_t GetHeight() const { return _height; }
	[[nodiscard]] uint16_t GetLayerCount() const { return _layers; }

	void Bind(uint8_t textureBindingPoint) const;

	void GenerateMipmap();

  protected:
	std::string _name;
	bgfx::TextureHandle _bgfxHandle;
	uint32_t _handle;
	uint16_t _width;
	uint16_t _height;
	uint16_t _layers;

	friend FrameBuffer;
};

} // namespace graphics
} // namespace openblack
