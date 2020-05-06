/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Font.h"

#include <algorithm>
#include <codecvt>
#include <locale>

#include <imgui.h>
#include <spdlog/spdlog.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#include "FileSystem/FileSystemInterface.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"

namespace openblack
{

void Font::LoadFromFile(const std::string& filename)
{
	auto& fileSystem = Locator::filesystem::value();

	auto fnt_filename = filename + ".fnt";
	auto met_filename = filename + ".met";

	auto meta = fileSystem.Open(met_filename, filesystem::Stream::Mode::Read);

	_size = meta->ReadValue<uint32_t>();

	// read utf16 string
	std::u16string u16_name(0x80, '\0');
	meta->Read(reinterpret_cast<uint8_t*>(u16_name.data()), 0x100);

	// convert to utf-8
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
	_name = convert.to_bytes(u16_name);

	uint32_t numGlyphs = meta->ReadValue<uint32_t>();

	_glyphs = std::vector<Glyph>(numGlyphs);
	for (uint32_t i = 0; i < numGlyphs; i++)
	{
		meta->Read(reinterpret_cast<uint8_t*>(&_glyphs[i]), sizeof(Glyph) - 16);
	}

	// build lookup table
	codepoint_t max_codepoint = 0;
	for (uint32_t i = 0; i != _glyphs.size(); i++)
		max_codepoint = std::max(max_codepoint, _glyphs[i].codepoint);

	_glyphLookup.resize(max_codepoint + 1, -1);

	for (uint32_t i = 0; i < _glyphs.size(); i++)
	{
		_glyphLookup[_glyphs[i].codepoint] = i;
	}

	meta.release();

	auto fntFile = fileSystem.Open(fnt_filename, filesystem::Stream::Mode::Read);
	size_t size = fntFile->Size();

	uint8_t* data = new uint8_t[size];
	fntFile->Read(data, size);
	BuildFontAtlas(data);
	delete[] data;

	fntFile.release();
}

const Font::Glyph* Font::FindGlyphNoFallback(codepoint_t codepoint) const
{
	if (codepoint >= _glyphLookup.size())
		return nullptr;

	const auto i = _glyphLookup[codepoint];
	if (i == (char16_t)-1)
		return nullptr;

	return &_glyphs[i];
}

/**
 * render our glyphs into rects, pack them and upload them to opengl
 */
// buildFontAtlas
void Font::BuildFontAtlas(uint8_t* fontData)
{
	std::vector<stbrp_rect> rects(_glyphs.size());
	memset(rects.data(), 0, rects.size() * sizeof(stbrp_rect));

	int total_surface = 0;
	for (size_t i = 0; i < _glyphs.size(); i++)
	{
		const auto& glyph = _glyphs[i];
		rects[i].id = i;
		rects[i].w = glyph.width + 2;
		rects[i].h = _size + 2;
		total_surface += rects[i].w * rects[i].h;
	}

	const int surface_sqrt = (int)sqrt((float)total_surface) + 1;
	const std::size_t tex_width = (surface_sqrt >= 4096 * 0.7f)   ? 4096
	                              : (surface_sqrt >= 2048 * 0.7f) ? 2048
	                              : (surface_sqrt >= 1024 * 0.7f) ? 1024
	                                                              : 512;
	const std::size_t TEX_HEIGHT_MAX = 1024 * 32;

	constexpr int nodeCount = 4096 * 2;
	struct stbrp_node* nodes = new stbrp_node[nodeCount];

	stbrp_context context;
	stbrp_init_target(&context, tex_width, TEX_HEIGHT_MAX, nodes, nodeCount);
	stbrp_pack_rects(&context, rects.data(), rects.size());

	std::size_t tex_height = 0;

	for (size_t i = 0; i < rects.size(); i++)
	{
		if (rects[i].was_packed)
			tex_height = std::max(tex_height, static_cast<std::size_t>(rects[i].y + rects[i].h));
	}

	printf("atlas tex: %zux%zu\n", tex_width, tex_height);

	// todo: do we need to round tex_height to pow2

	// allocate texture
	uint8_t* texPixels = new uint8_t[tex_width * tex_height];
	memset(texPixels, 0x00, tex_width * tex_height);

	float u_scale = 1.0f / tex_width;
	float v_scale = 1.0f / tex_height;

	// render / rasterize font glyphs into the texture
	for (size_t i = 0; i < _glyphs.size(); i++)
	{
		if (!rects[i].was_packed)
			continue;

		size_t offset = tex_width * rects[i].y + rects[i].x;
		size_t stride = tex_width;

		// for (int ry = 0; ry < rects[i].h; ry++)
		// {
		// 	int start_offset = tex_width * (rects[i].y + ry) + rects[i].x;
		// 	memset(&texPixels[start_offset], i, rects[i].w);
		// }

		RenderGlyph(&fontData[_glyphs[i].dataOffset], _glyphs[i].dataSize, _glyphs[i].width, &texPixels[offset], stride);

		// work out uv
		_glyphs[i].u0 = rects[i].x * u_scale;
		_glyphs[i].u1 = (rects[i].x + rects[i].w) * u_scale;
		_glyphs[i].v0 = rects[i].y * v_scale;
		_glyphs[i].v1 = (rects[i].y + rects[i].h) * v_scale;
	}

	// upload to gpu
	_atlasTexture = std::make_unique<graphics::Texture2D>("Font");
	_atlasTexture->Create(tex_width, tex_height, 1, graphics::Format::R8, graphics::Wrapping::ClampEdge,
	                      graphics::Filter::Linear, texPixels, tex_width * tex_height);

	delete[] texPixels;
	delete[] nodes;
}

void Font::RenderGlyph(uint8_t* glyphData, [[maybe_unused]] size_t size, uint16_t width, uint8_t* dest, size_t stride)
{
	int pixels = _size * width;
	uint8_t* bitmap = new uint8_t[pixels];

	int pos = 0;
	bool fill = false;
	while (pos < pixels)
	{
		// get the number of pixels to fill, if byte is 0xFF then get the next short.
		int count = *glyphData++;
		if (count == 0xFF)
		{
			count = *((uint16_t*)glyphData);
			glyphData += 2;
		}

		// if count exceeds our remaining pixels, just fill the remaining
		if (count + pos > pixels)
			count = pixels - pos;

		// fill the number of pixels
		memset(&bitmap[pos], fill ? 0xFF : 0x00, count);

		// add count and alternate fill
		pos += count;
		fill = !fill;
	}

	// copy to dest
	for (uint32_t y = 0; y < _size; y++)
	{
		for (int x = 0; x < width; x++)
		{
			dest[y * stride + x] = bitmap[y * width + x];
		}
	}

	delete[] bitmap;
}

/*
// wip
Font::Font(const std::string& filename)
{
    auto fnt_filename = filename + ".fnt";
    auto met_filename = filename + ".met";

    auto meta = Game::instance()->GetFileSystem().Open(met_filename, FileMode::Read);

    meta->Read(&_size, 4);

    // read utf16 string
    std::u16string u16_name(0x80, '\0');
    meta->Read(u16_name.data(), 0x100);

    // convert to utf-8
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    _name = convert.to_bytes(u16_name);

    uint32_t numGlyphs;
    meta->Read(&numGlyphs, 4);

    printf("font %s has %d glyphs\n", _name.c_str(), numGlyphs);

    //_glyphs = std::vector<FontGlyph>(numGlyphs);
}

std::string Font::GetName() const
{
    return _name;
}

unsigned int Font::GetSize() const
{
    return _size;
}

std::unique_ptr<uint8_t[]> Font::LoadGlyph(codepoint_t codepoint, Glyph& glyph) const
{
    auto glyph_data = std::make_unique<uint8_t[]>(1);
    return glyph_data;
}
*/

} // namespace openblack
