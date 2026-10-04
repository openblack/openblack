/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GameFont.h"

#include <cstring>

#include <algorithm>
#include <utility>

#include "TextDatabase.h"

using namespace openblack::gui;

namespace
{
constexpr size_t k_NameOffset = 4;
constexpr size_t k_NameSize = 0x100;
constexpr size_t k_CountOffset = k_NameOffset + k_NameSize;
constexpr size_t k_GlyphsOffset = k_CountOffset + 4;
constexpr size_t k_GlyphSize = 0x1C;
/// Width of the atlas and the clear pixels around each glyph in it
constexpr uint16_t k_AtlasWidth = 1024;
constexpr uint16_t k_Padding = 1;

template <typename T>
T Read(std::span<const uint8_t> data, size_t offset)
{
	T value;
	std::memcpy(&value, data.data() + offset, sizeof(T));
	return value;
}

/// A glyph's bitmap, a byte a pixel, set pixels 1
std::optional<std::vector<uint8_t>> DecodeBitmap(std::span<const uint8_t> runs, size_t pixels)
{
	std::vector<uint8_t> bitmap(pixels, 0);
	size_t position = 0;
	size_t i = 0;
	bool set = false;
	while (position < pixels && i < runs.size())
	{
		size_t run = runs[i++];
		if (run == 0xFF)
		{
			if (i + 2 > runs.size())
			{
				return std::nullopt;
			}
			run = runs[i] | (runs[i + 1] << 8);
			i += 2;
		}
		run = std::min(run, pixels - position);
		if (set)
		{
			std::fill_n(bitmap.begin() + static_cast<ptrdiff_t>(position), run, uint8_t {1});
		}
		position += run;
		set = !set;
	}
	return bitmap;
}

bool IsSpace(char16_t c)
{
	return c == u' ' || c == u'\t' || c == u'　' || c == u'\xA0';
}
} // namespace

std::optional<GameFont> GameFont::Load(std::span<const uint8_t> met, std::span<const uint8_t> fnt)
{
	if (met.size() < k_GlyphsOffset)
	{
		return std::nullopt;
	}
	GameFont font;
	font._height = static_cast<uint16_t>(Read<uint32_t>(met, 0));
	const auto count = Read<uint32_t>(met, k_CountOffset);
	if (font._height == 0 || font._height > 1024 || met.size() < k_GlyphsOffset + (count * k_GlyphSize))
	{
		return std::nullopt;
	}
	std::u16string name;
	for (size_t i = k_NameOffset; i + 1 < k_CountOffset; i += 2)
	{
		const auto c = static_cast<char16_t>(met[i] | (met[i + 1] << 8));
		if (c == u'\0')
		{
			break;
		}
		name.push_back(c);
	}
	font._name = ToUtf8(name);

	// The glyphs at half height, each pixel the average of four, packed in rows into the atlas
	const auto cellHeight = static_cast<uint16_t>((font._height + 1) / 2);
	std::vector<std::vector<uint8_t>> cells;
	glm::u16vec2 pen {0, 0};
	for (uint32_t g = 0; g < count; ++g)
	{
		const auto record = k_GlyphsOffset + (g * k_GlyphSize);
		Glyph glyph {
		    .character = static_cast<char16_t>(Read<uint16_t>(met, record)),
		    .width = Read<uint16_t>(met, record + 2),
		    .left = Read<float>(met, record + 8),
		    .ink = Read<float>(met, record + 0xC),
		    .right = Read<float>(met, record + 0x10),
		    .atlasMin = {},
		    .atlasMax = {},
		};
		const auto offset = Read<uint32_t>(met, record + 0x14);
		const auto length = Read<uint32_t>(met, record + 0x18);
		if (offset > fnt.size() || length > fnt.size() - offset)
		{
			return std::nullopt;
		}
		const auto bitmap = DecodeBitmap(fnt.subspan(offset, length), static_cast<size_t>(glyph.width) * font._height);
		if (!bitmap)
		{
			return std::nullopt;
		}

		const auto cellWidth = static_cast<uint16_t>((glyph.width + 1) / 2);
		std::vector<uint8_t> cell(static_cast<size_t>(cellWidth) * cellHeight, 0);
		for (uint16_t y = 0; y < cellHeight; ++y)
		{
			for (uint16_t x = 0; x < cellWidth; ++x)
			{
				uint32_t sum = 0;
				for (uint16_t sy = y * 2u; sy < std::min<uint32_t>((y * 2u) + 2u, font._height); ++sy)
				{
					for (uint16_t sx = x * 2u; sx < std::min<uint32_t>((x * 2u) + 2u, glyph.width); ++sx)
					{
						sum += (*bitmap)[(static_cast<size_t>(sy) * glyph.width) + sx];
					}
				}
				cell[(static_cast<size_t>(y) * cellWidth) + x] = static_cast<uint8_t>(sum * 255 / 4);
			}
		}

		if (pen.x + cellWidth + (k_Padding * 2) > k_AtlasWidth)
		{
			pen = {0, pen.y + cellHeight + (k_Padding * 2)};
		}
		glyph.atlasMin = {pen.x + k_Padding, pen.y + k_Padding};
		glyph.atlasMax = {glyph.atlasMin.x + cellWidth, glyph.atlasMin.y + cellHeight};
		pen.x += cellWidth + (k_Padding * 2);
		font._glyphs.push_back(glyph);
		cells.push_back(std::move(cell));
	}

	// Power of two heights keep every backend happy
	const auto usedHeight = static_cast<uint32_t>(pen.y + cellHeight + (k_Padding * 2));
	uint16_t atlasHeight = 1;
	while (atlasHeight < usedHeight)
	{
		atlasHeight = static_cast<uint16_t>(atlasHeight * 2);
	}
	font._atlasSize = {k_AtlasWidth, atlasHeight};
	font._atlas.assign(static_cast<size_t>(k_AtlasWidth) * atlasHeight, 0);
	for (size_t g = 0; g < font._glyphs.size(); ++g)
	{
		const auto& glyph = font._glyphs[g];
		const auto cellWidth = static_cast<size_t>(glyph.atlasMax.x - glyph.atlasMin.x);
		// Spaces have nothing to draw
		for (uint16_t y = 0; cellWidth > 0 && y < cellHeight; ++y)
		{
			std::memcpy(font._atlas.data() + (static_cast<size_t>(glyph.atlasMin.y + y) * k_AtlasWidth) + glyph.atlasMin.x,
			            cells[g].data() + (y * cellWidth), cellWidth);
		}
	}

	std::ranges::sort(font._glyphs, {}, &Glyph::character);
	return font;
}

const GameFont::Glyph* GameFont::Find(char16_t character) const
{
	// A character the font hasn't got shows as a question mark
	for (const auto wanted : {character, u'?'})
	{
		const auto found = std::ranges::lower_bound(_glyphs, wanted, {}, &Glyph::character);
		if (found != _glyphs.end() && found->character == wanted)
		{
			return &*found;
		}
	}
	return nullptr;
}

float GameFont::GetWidth(std::u16string_view text, float size) const
{
	float width = 0.0f;
	for (const auto c : text)
	{
		if (c == u'\n' || c == u'\r')
		{
			continue;
		}
		if (const auto* glyph = Find(c))
		{
			width += glyph->left + glyph->ink + glyph->right;
		}
	}
	return width * size / static_cast<float>(_height);
}

std::vector<std::u16string_view> GameFont::Wrap(std::u16string_view text, float size, float width) const
{
	std::vector<std::u16string_view> lines;
	while (!text.empty())
	{
		// Up to the next line break
		auto end = text.find_first_of(u"\r\n");
		auto line = text.substr(0, end);
		size_t next = end == std::u16string_view::npos ? text.size() : end + 1;
		if (end != std::u16string_view::npos && text[end] == u'\r' && end + 1 < text.size() && text[end + 1] == u'\n')
		{
			++next;
		}

		if (GetWidth(line, size) > width)
		{
			// The most that fits, then back to the last space or hyphen
			size_t fits = 0;
			while (fits < line.size() && GetWidth(line.substr(0, fits + 1), size) <= width)
			{
				++fits;
			}
			size_t breakAt = fits;
			while (breakAt > 0 && !IsSpace(line[breakAt]) && line[breakAt - 1] != u'-')
			{
				--breakAt;
			}
			if (breakAt == 0)
			{
				breakAt = std::max<size_t>(fits, 1);
			}
			next = breakAt;
			line = line.substr(0, breakAt);
			while (next < text.size() && IsSpace(text[next]))
			{
				++next;
			}
		}
		while (!line.empty() && IsSpace(line.back()))
		{
			line.remove_suffix(1);
		}
		lines.push_back(line);
		text.remove_prefix(next);
	}
	return lines;
}
