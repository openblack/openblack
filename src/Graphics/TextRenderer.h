/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <glm/fwd.hpp>

// !!! D:
#include <bgfx/bgfx.h>

struct stbrp_rect;

namespace openblack
{

namespace graphics
{
class ShaderProgram;
class Texture2D;
} // namespace graphics

namespace debug::gui
{
class FontViewer;
}

/**
 * @brief TextRenderer can be used to render text.
 *
 * TextRenderer loads all the fonts for Black & White into a single texture atlas.
 * Font bitmaps are 1 bit run length encoded data, we apply a 3x3 box blur which matches closely to vanilla.
 * The fonts are always sized at 80 and hard coded.
 * There are a total of 684 glyphs which reasonably fit in a 2048x2048 texture.
 *
 * The 3 fonts in Black & White 1 are:
 * Neutral - j0.fnt (Ocean Sans)
 * Good - f1.fnt (Footlight MT)
 * Evil - f3.fnt (Orange LEC)
 *
 * We map these to a Font enum to choose which one to render.
 *
 * @note We use std::u16string here as most the strings in Black & White are UTF-16 encoded.
 */
class TextRenderer
{
public:
	/**
	 * Which font to use
	 */
	enum class Font
	{
		Neutral = 0,
		Good = 1,
		Evil = 2,
		_COUNT
	};

	enum class TextAlignment
	{
		Left,
		Center,
		Right
	};

	TextRenderer();
	~TextRenderer();

	// TODO(Matt): Remove me
	void Draw();

	/**
	 * Draws text on the screen.
	 *
	 * @param text: The text.
	 * @param pos: Screen position.
	 * @param fontSize: Font size.
	 * @param font: Font.
	 */
	void DrawText(const std::u16string& text, glm::vec2 position, float fontSize, Font font = Font::Neutral,
	              TextAlignment align = TextAlignment::Left) const;

	/**
	 * Draws text on the screen.
	 *
	 * @param text: The text.
	 * @param pos: Screen position.
	 * @param fontSize: Font size.
	 * @param font: Font.
	 *
	 * @returns The total width of each character in the text combined.
	 */
	[[nodiscard]] float GetStringWidth(const std::u16string& text, float fontSize, Font font = Font::Neutral) const;

private:
	/**
	 * Holds info about a single glyph.
	 */
	struct Glyph
	{
		// filedata
		char16_t codepoint;
		uint16_t bitmapWidth; // Weird, not used
		int16_t intXoffset;   // Weird, not used
		uint16_t padding;
		float xOffset;
		float width;
		float xAdvance;
		uint32_t dataOffset;
		uint32_t dataSize;

		// renderer data
		float u0, v0, u1, v1;
	};

	void LoadFonts();
	void LoadFont(Font font, const std::filesystem::path& path, std::vector<stbrp_rect>& rects, std::vector<uint8_t*>& bitmaps);
	void BuildTextureAtlas(std::vector<stbrp_rect>& rects, std::vector<uint8_t*>& bitmapData);
	[[nodiscard]] Glyph GetGlyph(Font fontID, char16_t codepoint) const;

	// We're in C++20 and we still have no standard std::hash<std::tuple<...>>
	struct FontCodepointHash
	{
		std::size_t operator()(const std::tuple<Font, char16_t>& t) const
		{
			auto [fontID, codepoint] = t;
			return std::hash<Font> {}(fontID) ^ (std::hash<char16_t> {}(codepoint) << 1);
		}
	};

	std::vector<std::u16string> _fontNames;
	std::unordered_map<std::tuple<Font, char16_t>, Glyph, FontCodepointHash> _glyphs;
	std::unique_ptr<graphics::Texture2D> _atlasTexture;

	// Using bgfx directly because our VertexBuffer abstractions don't do the transient buffers
	bgfx::VertexLayout _vertexLayout;

	// Let the FontViewer show glyphs
	friend debug::gui::FontViewer;
};

} // namespace openblack
