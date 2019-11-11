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

#include <Common/stb_image_write.h>
#include <Graphics/Texture2D.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <spdlog/spdlog.h>

namespace openblack::graphics
{
constexpr std::array<bgfx::TextureFormat::Enum,
                     static_cast<size_t>(Format::RGBA4) + 1>
    textureFormatsBgfx {
        bgfx::TextureFormat::BC1,     // BlockCompression1
        bgfx::TextureFormat::BC2,     // BlockCompression2
        bgfx::TextureFormat::BC3,     // BlockCompression3
        bgfx::TextureFormat::D24S8,   // Depth24Stencil8
        bgfx::TextureFormat::D16,     // DepthComponent16
        bgfx::TextureFormat::D24,     // DepthComponent24
        bgfx::TextureFormat::R16F,    // R16F
        bgfx::TextureFormat::R16I,    // R16I
        bgfx::TextureFormat::R16S,    // R16SNorm
        bgfx::TextureFormat::R16U,    // R16UI
        bgfx::TextureFormat::R32F,    // R32F
        bgfx::TextureFormat::R32I,    // R32I
        bgfx::TextureFormat::R32U,    // R32UI
        bgfx::TextureFormat::R8,      // R8
        bgfx::TextureFormat::R8I,     // R8I
        bgfx::TextureFormat::R8S,     // R8SNorm
        bgfx::TextureFormat::R8U,     // R8UI
        bgfx::TextureFormat::RG16,    // RG16
        bgfx::TextureFormat::RG16F,   // RG16F
        bgfx::TextureFormat::RG16S,   // RG16SNorm
        bgfx::TextureFormat::RG32F,   // RG32F
        bgfx::TextureFormat::RG32I,   // RG32I
        bgfx::TextureFormat::RG32U,   // RG32UI
        bgfx::TextureFormat::RG8,     // RG8
        bgfx::TextureFormat::RG8I,    // RG8I
        bgfx::TextureFormat::RG8S,    // RG8SNorm
        bgfx::TextureFormat::RG8U,    // RG8UI
        bgfx::TextureFormat::RGB10A2, // RGB10A2
        bgfx::TextureFormat::R5G6B5,  // R5G6B5
        bgfx::TextureFormat::RGB5A1,  // RGB5A1
        bgfx::TextureFormat::RGB8,    // RGB8
        bgfx::TextureFormat::RGB8I,   // RGB8I
        bgfx::TextureFormat::RGB8U,   // RGB8UI
        bgfx::TextureFormat::RGB9E5F, // RGB9E5
        bgfx::TextureFormat::RGBA8,   // RGBA8
        bgfx::TextureFormat::RGBA8I,  // RGBA8I
        bgfx::TextureFormat::RGBA8U,  // RGBA8UI
        bgfx::TextureFormat::RGBA8S,  // RGBA8SNorm
        bgfx::TextureFormat::BGRA8,   // BGRA8
        bgfx::TextureFormat::RGBA16,  // RGBA16
        bgfx::TextureFormat::RGBA16F, // RGBA16F
        bgfx::TextureFormat::RGBA16I, // RGBA16I
        bgfx::TextureFormat::RGBA16U, // RGBA16UI
        bgfx::TextureFormat::RGBA16S, // RGBA16SNorm
        bgfx::TextureFormat::RGBA32F, // RGBA32F
        bgfx::TextureFormat::RGBA32I, // RGBA32I
        bgfx::TextureFormat::RGBA32U, // RGBA32UI
        bgfx::TextureFormat::RGBA4,   // RGBA4
    };

bgfx::TextureFormat::Enum getBgfxTextureFormat(Format format)
{
	return textureFormatsBgfx[static_cast<size_t>(format)];
}

Texture2D::Texture2D(std::string name): _name(std::move(name)), _handle(BGFX_INVALID_HANDLE) {}

Texture2D::~Texture2D()
{
	if (bgfx::isValid(_handle))
	{
		bgfx::destroy(_handle);
	}
}

// TODO(bwrsandman): Do something with wrapping... get to sampler?
void Texture2D::Create(uint16_t width, uint16_t height, uint16_t layers, Format format, Wrapping wrapping, const void* data,
                       size_t size)
{
	auto memory = bgfx::makeRef(data, size);
	_handle = bgfx::createTexture2D(width, height, false, layers, getBgfxTextureFormat(format), BGFX_TEXTURE_NONE, memory);
	bgfx::setName(_handle, _name.c_str());
	bgfx::frame();

	bgfx::calcTextureSize(_info, width, height, 1, false, false, layers, getBgfxTextureFormat(format));
	bgfx::frame();
}

void Texture2D::DumpTexture()
{
	assert(!_name.empty());
	std::vector<uint8_t> pixels;
	pixels.resize(_info.storageSize);
	bgfx::readTexture(_handle, pixels.data());
	bgfx::frame();
	const auto stride = _info.width * _info.bitsPerPixel / 8;
	// TODO(bwrsandman): get the number of components from _info.format
	const auto numComponents = 4u;
	for (uint16_t i = 0; i < _info.numLayers; ++i)
	{
		auto filename = "dump/" + _name + "_" + std::to_string(i) + ".png";
		spdlog::info("Writing texture layer {} to {}.", i, filename.c_str());
		auto current_pixels = &pixels[i * stride * _info.height];
		auto writeResult = stbi_write_png(filename.c_str(), _info.width, _info.height, numComponents, current_pixels, stride);
		if (writeResult == 0)
		{
			spdlog::error("Writing texture to {} failed!", filename.c_str());
			break;
		}
	}
}

} // namespace openblack::graphics
