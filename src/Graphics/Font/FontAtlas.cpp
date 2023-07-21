/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#if FALSE

#include "FontAtlas.h"

#include <algorithm>

#include "Graphics/Texture2D.h"
#include "Util/Hash.h"

namespace openblack
{

FontAtlas::FontAtlas()
    : _texWidth(0)
    , _texHeight(0)
{
	_texture = std::make_unique<graphics::Texture2D>("Font Atlas");
}

FontAtlas::~FontAtlas()
{
	// free all our bitmaps
	// free our stb_rect stuff
}

void FontAtlas::AddFont(const Font& font) {}

void FontAtlas::addGlyph(const Font& font, codepoint_t codepoint)
{
	std::size_t key = getKey(font, codepoint);
	// auto it = _entries.find(key);
	// if (it != _entries.end()) {
	//	return false; // entry already exists
	// }

	Glyph glyph;
	std::unique_ptr<uint8_t[]> image = font.LoadGlyph(codepoint, glyph);

	stbrp_rect rect;
	rect.id = key;
	rect.w = glyph.width + 2; // give each glyph a border
	rect.h = font.GetSize() + 2;
	rect.x = 0;
	rect.y = 0;
	rect.was_packed = 0;
	_rects.push_back(rect);

	Entry entry;
	entry.glyph = glyph;
	entry.u0 = 0.0f;
	entry.v0 = 0.0f;
	entry.u1 = 0.0f;
	entry.v1 = 0.0f;
	entry.bitmap = std::move(image);
	_entries.emplace(key, entry);
}

std::size_t FontAtlas::getKey(const Font& font, codepoint_t codepoint)
{
	std::size_t hash = std::hash<std::type_index>()(std::type_index(typeid(FontAtlas)));
	hash_combine(hash, font);
	hash_combine(hash, codepoint);
	return hash;
}

const graphics::Texture2D& FontAtlas::GetTexture() const
{
	// if texture isn't built...................
	// build();

	return *_texture;
}

void FontAtlas::build()
{
	// Calculate our surface area to get a texture width.
	std::size_t totalSurface = 0;
	for (int i = 0; i < _rects.size(); i++)
		totalSurface += (_rects[i].w * _rects[i].h);

	const int surfaceSqrt = (int)sqrt((float)totalSurface) + 1;
	_texWidth = (surfaceSqrt >= 4096 * 0.7f)   ? 4096
	            : (surfaceSqrt >= 2048 * 0.7f) ? 2048
	            : (surfaceSqrt >= 1024 * 0.7f) ? 1024
	                                           : 512;

	// Pack our rects
	const int maxTexHeight = 32768;
	const int nodeCount = 4096 * 2;
	struct stbrp_node nodes[nodeCount];

	stbrp_context spc = {};
	stbrp_init_target(&spc, _texWidth, maxTexHeight, nodes, nodeCount);
	stbrp_pack_rects(&spc, _rects.data(), _rects.size());

	// Allocate our texture
	_texPixelsAlpha8.release();
	_texPixelsAlpha8 = std::unique_ptr<uint8_t[]>(new uint8_t[_texWidth * _texHeight]);
	std::memset(_texPixelsAlpha8.get(), 0x00, _texWidth * _texHeight);

	// Render/rasterize font characters into the texture

	// Finally upload our texture to the GPU
	_texture->Create(_texWidth, _texHeight, 1, graphics::Format::R8, graphics::Wrapping::ClampEdge, _texPixelsAlpha8.get(),
	                 _texWidth * _texHeight);
}

FontCache::Entry FontCache::Set(std::size_t key, const Glyph& glyph, const uint8_t* bitmap)
{
	int required_width = glyph.width;
	int required_height = glyph.height;

	if (_shelves.empty())
	{
		_shelves.emplace_back(0, _width, required_height);
	}

	for (auto& shelf : _shelves)
	{
		if (shelf.Fits(required_width, required_height))
		{
			unsigned int x_pos = shelf.Reserve(required_width, required_height);
			unsigned int y_pos = shelf.yPosition;

			// Create a new entry and insert it
			FontCache::Entry entry;
			entry.glyph = glyph;
			entry.u0 = static_cast<float>(x_pos) / _width;
			entry.v0 = static_cast<float>(y_pos) / _height;
			entry.u1 = static_cast<float>(x_pos + glyph.width) / _width;
			entry.v1 = static_cast<float>(y_pos + glyph.height) / _height;

			for (unsigned int y = 0; y < glyph.height; y++)
			{
				memcpy(_buffer.get() + ((y_pos + y) * _width + x_pos), bitmap + (y * glyph.width),
				       glyph.width * sizeof(unsigned char));
			}

			_dirtyRect = {
			    std::min(_dirtyRect.x1, x_pos),
			    std::min(_dirtyRect.y1, y_pos),
			    std::max(_dirtyRect.x2, x_pos + glyph.width),
			    std::max(_dirtyRect.y2, y_pos + glyph.height),
			};
			_isDirty = true;

			_glyphs.emplace(key, entry);
			return entry;
		}
	}

	FontCache::Shelf last_shelf = _shelves.back();
	if (_height > (last_shelf.yPosition + last_shelf.height + required_height))
	{
		_shelves.emplace_back(last_shelf.yPosition + last_shelf.height, _width, required_height);
		return Set(key, glyph, bitmap);
	}

	// todo: never get here
}

std::size_t FontCache::getKey(Font* font, codepoint_t codepoint) const
{
	std::size_t hash = std::hash<std::type_index>()(std::type_index(typeid(FontCache)));
	hash_combine(hash, *font);
	hash_combine(hash, codepoint);
	return hash;
}

} // namespace openblack

#endif
