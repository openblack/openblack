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

#include <Graphics/Texture2D.h>

#include <algorithm>
#include <array>

#include <Graphics/OpenGL.h>

namespace openblack::graphics
{
struct TextureFormat
{
  const GLenum _internalFormat;
  const GLenum _format;
  const GLenum _type;
  const bool _compressed;
};

constexpr std::array<TextureFormat, static_cast<size_t>(Format::RGBA4) + 1> textureFormats = {
	TextureFormat { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,  GL_ZERO,                         true,  }, // BlockCompression1
	TextureFormat { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,  GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,  GL_ZERO,                         true,  }, // BlockCompression2
	TextureFormat { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,  GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,  GL_ZERO,                         true,  }, // BlockCompression3
	TextureFormat { GL_DEPTH24_STENCIL8,               GL_DEPTH_STENCIL,                  GL_UNSIGNED_INT_24_8,            false, }, // Depth24Stencil8
	TextureFormat { GL_DEPTH_COMPONENT16,              GL_DEPTH_COMPONENT,                GL_UNSIGNED_SHORT,               false, }, // DepthComponent16
	TextureFormat { GL_DEPTH_COMPONENT24,              GL_DEPTH_COMPONENT,                GL_UNSIGNED_INT,                 false, }, // DepthComponent24
	TextureFormat { GL_R16F,                           GL_RED,                            GL_HALF_FLOAT,                   false, }, // R16F
	TextureFormat { GL_R16I,                           GL_RED,                            GL_SHORT,                        false, }, // R16I
	TextureFormat { GL_R16_SNORM,                      GL_RED,                            GL_SHORT,                        false, }, // R16SNorm
	TextureFormat { GL_R16UI,                          GL_RED,                            GL_UNSIGNED_SHORT,               false, }, // R16UI
	TextureFormat { GL_R32F,                           GL_RED,                            GL_FLOAT,                        false, }, // R32F
	TextureFormat { GL_R32I,                           GL_RED,                            GL_INT,                          false, }, // R32I
	TextureFormat { GL_R32UI,                          GL_RED,                            GL_UNSIGNED_INT,                 false, }, // R32UI
	TextureFormat { GL_R8,                             GL_RED,                            GL_UNSIGNED_BYTE,                false, }, // R8
	TextureFormat { GL_R8I,                            GL_RED,                            GL_BYTE,                         false, }, // R8I
	TextureFormat { GL_R8_SNORM,                       GL_RED,                            GL_BYTE,                         false, }, // R8SNorm
	TextureFormat { GL_R8UI,                           GL_RED,                            GL_UNSIGNED_BYTE,                false, }, // R8UI
	TextureFormat { GL_RG16,                           GL_RG,                             GL_UNSIGNED_SHORT,               false, }, // RG16
	TextureFormat { GL_RG16F,                          GL_RG,                             GL_FLOAT,                        false, }, // RG16F
	TextureFormat { GL_RG16_SNORM,                     GL_RG,                             GL_SHORT,                        false, }, // RG16SNorm
	TextureFormat { GL_RG32F,                          GL_RG,                             GL_FLOAT,                        false, }, // RG32F
	TextureFormat { GL_RG32I,                          GL_RG,                             GL_INT,                          false, }, // RG32I
	TextureFormat { GL_RG32UI,                         GL_RG,                             GL_UNSIGNED_INT,                 false, }, // RG32UI
	TextureFormat { GL_RG8,                            GL_RG,                             GL_UNSIGNED_BYTE,                false, }, // RG8
	TextureFormat { GL_RG8I,                           GL_RG,                             GL_BYTE,                         false, }, // RG8I
	TextureFormat { GL_RG8_SNORM,                      GL_RG,                             GL_BYTE,                         false, }, // RG8SNorm
	TextureFormat { GL_RG8UI,                          GL_RG,                             GL_UNSIGNED_BYTE,                false, }, // RG8UI
	TextureFormat { GL_RGB10_A2,                       GL_RGBA,                           GL_UNSIGNED_INT_2_10_10_10_REV,  false, }, // RGB10A2
	TextureFormat { 0x8D62 /*GL_RGB565*/,              GL_RGB,                            GL_UNSIGNED_SHORT_5_6_5,         false, }, // R5G6B5
	TextureFormat { GL_RGB5_A1,                        GL_BGRA,                           GL_UNSIGNED_SHORT_1_5_5_5_REV,   false, }, // RGB5A1
	TextureFormat { GL_RGB8,                           GL_RGB,                            GL_UNSIGNED_BYTE,                false, }, // RGB8
	TextureFormat { GL_RGB8I,                          GL_RGB,                            GL_BYTE,                         false, }, // RGB8I
	TextureFormat { GL_RGB8UI,                         GL_RGB,                            GL_UNSIGNED_BYTE,                false, }, // RGB8UI
	TextureFormat { GL_RGB9_E5,                        GL_RGB,                            GL_UNSIGNED_INT_5_9_9_9_REV,     false, }, // RGB9E5
	TextureFormat { GL_RGBA8,                          GL_RGBA,                           GL_UNSIGNED_BYTE,                false, }, // RGBA8
	TextureFormat { GL_RGBA8I,                         GL_RGBA,                           GL_BYTE,                         false, }, // RGBA8I
	TextureFormat { GL_RGBA8UI,                        GL_RGBA,                           GL_UNSIGNED_BYTE,                false, }, // RGBA8UI
	TextureFormat { GL_RGBA8_SNORM,                    GL_RGBA,                           GL_BYTE,                         false, }, // RGBA8SNorm
	TextureFormat { GL_RGBA8,                          GL_BGRA,                           GL_UNSIGNED_BYTE,                false, }, // BGRA8
	TextureFormat { GL_RGBA16,                         GL_RGBA,                           GL_UNSIGNED_SHORT,               false, }, // RGBA16
	TextureFormat { GL_RGBA16F,                        GL_RGBA,                           GL_HALF_FLOAT,                   false, }, // RGBA16F
	TextureFormat { GL_RGBA16I,                        GL_RGBA,                           GL_SHORT,                        false, }, // RGBA16I
	TextureFormat { GL_RGBA16UI,                       GL_RGBA,                           GL_UNSIGNED_SHORT,               false, }, // RGBA16UI
	TextureFormat { GL_RGBA16_SNORM,                   GL_RGBA,                           GL_SHORT,                        false, }, // RGBA16SNorm
	TextureFormat { GL_RGBA32F,                        GL_RGBA,                           GL_FLOAT,                        false, }, // RGBA32F
	TextureFormat { GL_RGBA32I,                        GL_RGBA,                           GL_INT,                          false, }, // RGBA32I
	TextureFormat { GL_RGBA32UI,                       GL_RGBA,                           GL_UNSIGNED_INT,                 false, }, // RGBA32UI
	TextureFormat { GL_RGBA4,                          GL_BGRA,                           GL_UNSIGNED_SHORT_4_4_4_4_REV,   false, }, // RGBA4
};

constexpr std::array<GLenum, static_cast<size_t>(Filter::LinearMipmapLinear) + 1> filters {
	GL_NEAREST,
	GL_LINEAR,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_LINEAR
};

constexpr std::array<GLenum, static_cast<size_t>(Wrapping::MirroredRepeat) + 1> wrappingModes {
	GL_CLAMP_TO_EDGE,
	GL_CLAMP_TO_BORDER,
	GL_REPEAT,
	GL_MIRRORED_REPEAT
};

Texture2D::Texture2D()
{
	GLuint texture;
	glGenTextures(1, &texture);
	_handle = static_cast<unsigned int>(texture);
}

Texture2D::~Texture2D()
{
	if (_handle)
	{
		auto texture = static_cast<GLuint>(_handle);
		glDeleteTextures(1, &texture);
	}
}

void Texture2D::Create(uint16_t width, uint16_t height, uint16_t layers, Format format, Wrapping wrapping, const void* data, size_t size)
{
	auto bindPoint = layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	auto textureFormat = textureFormats[static_cast<size_t>(format)];

	glBindTexture(bindPoint, static_cast<GLuint>(_handle));

	if (textureFormat._compressed)
	{
		if (layers == 1)
			glCompressedTexImage2D(bindPoint, 0, textureFormat._internalFormat, width, height, 0, size, data);
		else
			glCompressedTexImage3D(bindPoint, 0, textureFormat._internalFormat, width, height, layers, 0, size, data);
	}
	else
	{
		if (layers==1)
			glTexImage2D(bindPoint, 0, textureFormat._internalFormat, width, height, 0, textureFormat._format, textureFormat._type, data);
		else
			glTexImage3D(bindPoint, 0, textureFormat._internalFormat, width, height, layers, 0, textureFormat._format, textureFormat._type, data);
	}

	glTexParameteri(bindPoint, GL_TEXTURE_WRAP_S, wrappingModes[static_cast<size_t>(wrapping)]);
	glTexParameteri(bindPoint, GL_TEXTURE_WRAP_T, wrappingModes[static_cast<size_t>(wrapping)]);
	glTexParameteri(bindPoint, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(bindPoint, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	_width = width;
	_height = height;
	_layers = layers;
}

void Texture2D::Bind(uint8_t textureBindingPoint) const
{
	glActiveTexture(GL_TEXTURE0 + textureBindingPoint);

	auto bindPoint = _layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	auto texture = static_cast<GLuint>(_handle);
	glBindTexture(bindPoint, texture);
}

void Texture2D::GenerateMipmap()
{
	auto bindPoint = _layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

	auto texture = static_cast<GLuint>(_handle);
	glBindTexture(bindPoint, texture);
	glGenerateMipmap(bindPoint);
}

} // namespace openblack::Graphics
