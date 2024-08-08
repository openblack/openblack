/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Font.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include <stb/stb_rect_pack.h>

namespace openblack
{

// forward declares
namespace graphics
{
class Texture2D;
}

/**
 * FontAtlas packs glyphs from several fonts into a single atlas texture.
 *
 * Each glyph is get/set from a [Font, codepoint_t] pair.
 * If the glyph doesn't exist the bitmap is loaded from the Font and packed
 * into the texture atlas.
 *
 * The returned struct returns the Glyph and UV0, UV1 of the position in the
 * texture atlas.
 *
 * For Black & White 1:
 * We have 3 fonts at size 80 when combined have a total of 684 glyphs.
 * All glyphs should be able to fit in a 2048x2048 texture.
 */
class FontAtlas
{
public:
	class Entry
	{
		friend class FontAtlas;

	public:
		Glyph glyph; // so we don't have to search the Font and FontAtlas
		float u0;
		float v0;
		float u1;
		float v1;

	protected:
		std::unique_ptr<uint8_t[]> bitmap; // todo: maybe this isn't needed
	};

public:
	FontAtlas();
	~FontAtlas();

	/**
	 * AddFont packs all the glyphs from the given font into the atlas.
	 */
	void AddFont(const Font& font);

	bool Build();

	// add a glyph ready to be packed on Build() lets you batch them before packing begins
	// bool AddGlyph(const Font& font, codepoint_t codepoint);
	// AddGlyphs(Font, vector<codepoint_t>);

	/**
	 * Gets the UV coordinates of the given codepoint from a font.
	 * If the glyph doesn't exist, UV coordinates for an invalid glyph will be returned. (Maybe just return first glyph.)
	 */
	Entry Get(const Font& font, codepoint_t codepoint);

	const graphics::Texture2D& GetTexture() const;

private:
	void addGlyph(const Font& font, codepoint_t codepoint);

	void build();
	static std::size_t getKey(const Font& font, codepoint_t codepoint);

private:
	std::size_t _texWidth;
	std::size_t _texHeight;
	std::unique_ptr<uint8_t[]> _texPixelsAlpha8;
	std::unique_ptr<graphics::Texture2D> _texture;

	std::unordered_map<std::size_t, Entry> _entries;
	std::vector<stbrp_rect> _rects;
};

/*class FontAtlas
{
public:
    struct Entry {
        Glyph glyph;
        glm::vec2 uv0;
        glm::vec2 uv1;
    };

public:
    FontAtlas(unsigned int width = 2048, unsigned int height = 2048);

    Entry Get(const Font& font, codepoint_t codepoint);

    const graphics::Texture2D& GetTexture();

private:
    Entry set(std::size_t key, const Glyph& glyph, const uint8_t* bitmap);
    std::size_t getKey(const Font& font, codepoint_t codepoint) const;

    void updateDirty();

private:
    struct Shelf {
        int y;
        int height;
        int width_remaining;
    };

    /*struct Shelf {
        Shelf(int pos, int width, int height);
        bool Fits(int width, int height);
        int Reserve(int width, int height);

        int yPosition;
        int width;
        int height;
        int remainingWidth;
    };*/
/*
    struct Rect {
        unsigned int x1;
        unsigned int y1;
        unsigned int x2;
        unsigned int y2;
    };

private:
    unsigned int _width;
    unsigned int _height;
    bool _isDirty;
    Rect _dirtyRect;

    std::vector<uint8_t> _buffer;

    std::unique_ptr<graphics::Texture2D> _texture;
    std::unordered_map<std::size_t, FontCache::Entry> _glyphs;
    std::vector<Shelf> _shelves;
};*/

/**
 * A glyph atlas is used to pack and manage several font glyphs in to a single atlas texture.
 * A single glyph atlas can be used to store glyphs from multiple fonts.
 */
class FontCache
{
public:
	struct Entry
	{
		Glyph glyph;
		float u0;
		float v0;
		float u1;
		float v1;
	};

public:
	FontCache(unsigned int width = 1024, unsigned int height = 1024);

	FontCache::Entry Get(Font* font, codepoint_t codepoint);
	FontCache::Entry Set(std::size_t key, const Glyph& glyph, const uint8_t* bitmap);

	const graphics::Texture2D& GetTexture();

private:
	std::size_t getKey(Font* font, codepoint_t codepoint) const;

private:
	struct Shelf
	{
		Shelf(int pos, int width, int height);
		bool Fits(int width, int height);
		int Reserve(int width, int height);

		int yPosition;
		int width;
		int height;
		int remainingWidth;
	};

	struct Rect
	{
		unsigned int x1;
		unsigned int y1;
		unsigned int x2;
		unsigned int y2;
	};

private:
	unsigned int _width;
	unsigned int _height;
	bool _isDirty;
	Rect _dirtyRect;

	std::unique_ptr<uint8_t> _buffer;
	std::unique_ptr<graphics::Texture2D> _texture;
	std::unordered_map<std::size_t, FontCache::Entry> _glyphs;
	std::vector<Shelf> _shelves;
};

} // namespace openblack

#endif
