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

#include <Graphics/OpenGL.h>

#include <cstddef>

namespace openblack
{
namespace graphics
{

enum class InternalFormat : GLint
{
	CompressedRGBAS3TCDXT1   = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	CompressedRGBAS3TCDXT3   = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	DepthStencil             = GL_DEPTH_STENCIL,
	Depth24Stencil8          = GL_DEPTH24_STENCIL8,
	Depth32FStencil8         = GL_DEPTH32F_STENCIL8,
	DepthComponent           = GL_DEPTH_COMPONENT,
	DepthComponent16         = GL_DEPTH_COMPONENT16,
	DepthComponent24         = GL_DEPTH_COMPONENT24,
	DepthComponent32F        = GL_DEPTH_COMPONENT32F,
	R16F                     = GL_R16F,
	R16I                     = GL_R16I,
	R16SNorm                 = GL_R16_SNORM,
	R16UI                    = GL_R16UI,
	R32F                     = GL_R32F,
	R32I                     = GL_R32I,
	R32UI                    = GL_R32UI,
	R3G3B2                   = GL_R3_G3_B2,
	R8                       = GL_R8,
	R8I                      = GL_R8I,
	R8SNorm                  = GL_R8_SNORM,
	R8UI                     = GL_R8UI,
	RG                       = GL_RG,
	RG16                     = GL_RG16,
	RG16F                    = GL_RG16F,
	RG16SNorm                = GL_RG16_SNORM,
	RG32F                    = GL_RG32F,
	RG32I                    = GL_RG32I,
	RG32UI                   = GL_RG32UI,
	RG8                      = GL_RG8,
	RG8I                     = GL_RG8I,
	RG8SNorm                 = GL_RG8_SNORM,
	RG8UI                    = GL_RG8UI,
	RGB                      = GL_RGB,
	RGB10                    = GL_RGB10,
	RGB10A2                  = GL_RGB10_A2,
	RGB12                    = GL_RGB12,
	RGB16                    = GL_RGB16,
	RGB16F                   = GL_RGB16F,
	RGB16I                   = GL_RGB16I,
	RGB16UI                  = GL_RGB16UI,
	RGB32F                   = GL_RGB32F,
	RGB32I                   = GL_RGB32I,
	RGB32UI                  = GL_RGB32UI,
	RGB4                     = GL_RGB4,
	RGB5                     = GL_RGB5,
	RGB5A1                   = GL_RGB5_A1,
	RGB8                     = GL_RGB8,
	RGB8I                    = GL_RGB8I,
	RGB8UI                   = GL_RGB8UI,
	RGB9E5                   = GL_RGB9_E5,
	RGBA                     = GL_RGBA,
	RGBA12                   = GL_RGBA12,
	RGBA16                   = GL_RGBA16,
	RGBA16F                  = GL_RGBA16F,
	RGBA16I                  = GL_RGBA16I,
	RGBA16UI                 = GL_RGBA16UI,
	RGBA2                    = GL_RGBA2,
	RGBA32F                  = GL_RGBA32F,
	RGBA32I                  = GL_RGBA32I,
	RGBA32UI                 = GL_RGBA32UI,
	RGBA4                    = GL_RGBA4,
	RGBA8                    = GL_RGBA8,
	RGBA8UI                  = GL_RGBA8UI,
	SRGB8                    = GL_SRGB8,
	SRGB8A8                  = GL_SRGB8_ALPHA8,
	SRGBA                    = GL_SRGB_ALPHA,
};

enum class Format : GLenum
{
	Red  = GL_RED,
	RGB  = GL_RGB,
	BGR  = GL_BGR,
	RGBA = GL_RGBA,
	BGRA = GL_BGRA
};

enum class DataType : GLenum
{
	Byte          = GL_BYTE,
	UnsignedByte  = GL_UNSIGNED_BYTE,
	Short         = GL_SHORT,
	UnsignedShort = GL_UNSIGNED_SHORT,
	Int           = GL_INT,
	UnsignedInt   = GL_UNSIGNED_INT,
	Float         = GL_FLOAT,
	Double        = GL_DOUBLE,

	UnsignedByte332      = GL_UNSIGNED_BYTE_3_3_2,
	UnsignedByte233Rev   = GL_UNSIGNED_BYTE_2_3_3_REV,
	UnsignedShort565     = GL_UNSIGNED_SHORT_5_6_5,
	UnsignedShort565Rev  = GL_UNSIGNED_SHORT_5_6_5,
	UnsignedShort4444    = GL_UNSIGNED_SHORT_4_4_4_4,
	UnsignedShort4444Rev = GL_UNSIGNED_SHORT_4_4_4_4_REV,
	UnsignedShort5551    = GL_UNSIGNED_SHORT_5_5_5_1,
	UnsignedShort1555Rev = GL_UNSIGNED_SHORT_1_5_5_5_REV,
	UnsignedInt8888      = GL_UNSIGNED_INT_8_8_8_8,
	UnsignedInt8888Rev   = GL_UNSIGNED_INT_8_8_8_8_REV,
	UnsignedInt101010102 = GL_UNSIGNED_INT_10_10_10_2
};

enum class Filter : GLenum
{
	Nearest              = GL_NEAREST,
	Linear               = GL_LINEAR,
	NearestMipmapNearest = GL_NEAREST_MIPMAP_NEAREST,
	LinearMipmapNearest  = GL_LINEAR_MIPMAP_NEAREST,
	NearestMipmapLinear  = GL_NEAREST_MIPMAP_LINEAR,
	LinearMipmapLinear   = GL_LINEAR_MIPMAP_LINEAR
};

enum class Wrapping : GLenum
{
	ClampEdge      = GL_CLAMP_TO_EDGE,
	ClampBorder    = GL_CLAMP_TO_BORDER,
	Repeat         = GL_REPEAT,
	MirroredRepeat = GL_MIRRORED_REPEAT
};

class Texture2D
{
  public:
	Texture2D();
	~Texture2D();

	// No copying or assignment
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	void Create(uint32_t width, uint32_t height, uint32_t layers, InternalFormat internalFormat=InternalFormat::RGBA, DataType type=DataType::UnsignedByte, Format format=Format::RGBA, const void* data=nullptr, size_t size=0);

	[[nodiscard]] uint32_t GetNativeHandle() const { return _handle; }
	[[nodiscard]] uint32_t GetWidth() const { return _width; }
	[[nodiscard]] uint32_t GetHeight() const { return _height; }
	[[nodiscard]] uint32_t GetLayerCount() const { return _layers; }

	void Bind(uint8_t textureBindingPoint) const;

	void GenerateMipmap();

  protected:
	uint32_t _handle;
	uint32_t _width;
	uint32_t _height;
	uint32_t _layers;
};

} // namespace graphics
} // namespace openblack
