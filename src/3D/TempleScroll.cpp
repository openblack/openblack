/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TempleScroll.h"

#include <algorithm>
#include <array>
#include <functional>

#include "Gui/GameFont.h"
#include "Gui/TextDatabase.h"

using namespace openblack;

namespace
{
/// SeeIfAnyCuttingUpOfTheTextNeedsDoing breaks a line once it is 230 texels across
constexpr uint32_t k_WrapWidth = 230;
/// fn_00799DF0 reads a line into a buffer of 64 characters, and stops the text at a longer one
constexpr size_t k_MaxLineLength = 63;
/// GatheringText::DrawChar2Texture draws letters smaller than this from the glyphs cached at a quarter of their height
constexpr float k_SmallGlyphSize = 26.0f;
/// GatheringText's font units: a size is the height of a line, 80 of them
constexpr float k_FontUnit = 0.0125f;

/// iswspace in the C locale
bool IsSpace(char16_t c)
{
	return c == u' ' || (c >= u'\t' && c <= u'\r');
}

/// _wtoi
int32_t ParseNumber(std::u16string_view text)
{
	size_t i = 0;
	while (i < text.size() && IsSpace(text[i]))
	{
		++i;
	}
	bool negative = false;
	if (i < text.size() && (text[i] == u'-' || text[i] == u'+'))
	{
		negative = text[i] == u'-';
		++i;
	}
	int32_t value = 0;
	for (; i < text.size() && text[i] >= u'0' && text[i] <= u'9'; ++i)
	{
		value = (value * 10) + (text[i] - u'0');
	}
	return negative ? -value : value;
}

/// __ftol: towards zero
int32_t Truncate(float value)
{
	return static_cast<int32_t>(value);
}

/// The ARGB4444 of the parchment's RGB888, opaque
uint16_t ParchmentTexel(uint8_t r, uint8_t g, uint8_t b)
{
	return static_cast<uint16_t>(0xF000u | ((r & 0xF0u) << 4) | (g & 0xF0u) | (b >> 4));
}

/// GatheringText::DrawChar2Texture: a glyph into the texels, from x across its bitmap's width at scale and from y down
/// size, sampled from the small glyph and blended over the texels with the colour's ramp by its coverage
void DrawGlyph(std::span<uint16_t> texels, const gui::GameFont::SmallGlyph& small, uint16_t bitmapWidth, float x, float y,
               float scale, float size, const std::array<uint16_t, 16>& ramp, uint16_t colour)
{
	constexpr int32_t k_Size = TempleScrollTexture::k_Size;
	int32_t left = Truncate(x);
	int32_t right = Truncate((static_cast<float>(bitmapWidth) * scale) + x);
	int32_t top = Truncate(y);
	int32_t bottom = Truncate(y + size);
	if (right <= left || bottom <= top)
	{
		return;
	}
	// The glyph at a quarter of its height is a quarter of its bitmap's width, 20 rows high, from its clear column
	const float sourceWidth = static_cast<float>(bitmapWidth) * 0.5f * 0.5f;
	const float stepX = sourceWidth / static_cast<float>(right - left);
	const float stepY = static_cast<float>(gui::GameFont::SmallGlyph::k_Height) / static_cast<float>(bottom - top);
	int32_t column = 0;
	int32_t row = 0;
	if (left < 0)
	{
		column = Truncate(-static_cast<float>(left) * stepX);
		left = 0;
	}
	right = std::min(right, k_Size);
	if (top < 0)
	{
		row = Truncate(-static_cast<float>(top) * stepY);
		top = 0;
	}
	bottom = std::min(bottom, k_Size);
	if (right <= left || bottom <= top)
	{
		return;
	}

	const auto fixedX = Truncate(stepX * 65536.0f);
	const auto fixedY = Truncate(stepY * 65536.0f);
	// The colour's alpha scales the first of the four samples, 16 for opaque
	const auto alphaScale = static_cast<uint32_t>(((colour >> 12) & 0xFu) * 16u / 15u);
	int32_t rowFixed = row << 16;
	for (int32_t ty = top; ty < bottom; ++ty, rowFixed += fixedY)
	{
		const int32_t sy = rowFixed >> 16;
		int32_t columnFixed = column << 16;
		for (int32_t tx = left; tx < right; ++tx, columnFixed += fixedX)
		{
			const int32_t sx = columnFixed >> 16;
			// The first sample is the cache's texel, its colour channels set, scaled by the colour's alpha
			const auto first = static_cast<uint32_t>((((small.At(sx, sy) << 12u) | 0xFFFu) * alphaScale) >> 16u);
			// Its neighbours right and down, or the first again at the texture's last column and row
			uint32_t sum = tx < 0xFF ? first + small.At(sx + 1, sy) : first * 2;
			if (ty < 0xFF)
			{
				sum += small.At(sx, sy + 1);
				sum += tx < 0xFF ? small.At(sx + 1, sy + 1) : first;
			}
			else
			{
				sum *= 2;
			}
			const uint32_t coverage = (sum & 0xFFFFu) >> 2;
			if (coverage == 0)
			{
				continue;
			}
			// What shows of the texel's colour, each channel's nibble times (15 - coverage) / 16, under the ramp's
			auto& texel = texels[(static_cast<size_t>(ty) * k_Size) + tx];
			const auto keep = [coverage](uint32_t nibble) { return (nibble * (15u - (coverage & 0xFu))) >> 4u; };
			const uint32_t kept = (keep((texel >> 8) & 0xFu) << 8) | (keep((texel >> 4) & 0xFu) << 4) | keep(texel & 0xFu);
			texel = static_cast<uint16_t>(kept + (ramp.at(coverage & 0xFu) & 0xFFFu) + (texel & 0xF000u));
		}
	}
}
} // namespace

TempleScrollText::TempleScrollText(const gui::GameFont& font)
    : _font(font)
{
}

void TempleScrollText::AddNewLine()
{
	if (_text.size() < k_MaxLength)
	{
		_text += u"<N>";
	}
}

void TempleScrollText::End()
{
	_text += u"<E>";
}

void TempleScrollText::Add(std::u16string_view source)
{
	if (_text.size() + source.size() >= k_MaxLength)
	{
		return;
	}

	// SeeIfAnyCuttingUpOfTheTextNeedsDoing works on a copy. "$g" and a number, or "$m" and one, ends the text there
	// and names a gesture or a control to picture after it; a word that starts with any other "$", "/" or "\" is
	// hidden, every character of it up to the next space.
	std::u16string text(source);
	const auto length = text.size();
	int32_t gesture = -1;
	int32_t control = -1;
	for (size_t i = 0; i < length;)
	{
		size_t end = i;
		const auto c = text[i];
		if (c == u'$' || c == u'/' || c == u'\\')
		{
			const auto kind = i + 1 < length ? text[i + 1] : u'\0';
			if (kind == u'g' || kind == u'G' || kind == u'm' || kind == u'M')
			{
				text[i] = u'\0';
				const auto start = i + 2;
				end = start;
				while (end < length && !IsSpace(text[end]))
				{
					++end;
				}
				const auto number =
				    ParseNumber(std::u16string_view(text).substr(std::min(start, length), end - std::min(start, length)));
				if (kind == u'g' || kind == u'G')
				{
					gesture = number;
				}
				else
				{
					// TODO(raffclar): For the controls 0 to 32 the line goes on "(" with the keys and mouse buttons the
					// control is bound to (fn_0046F260, fn_0046F2B0), a space between them, then ")"
					control = number;
				}
			}
			else
			{
				while (end < length && !IsSpace(text[end]))
				{
					text[end] = k_Hidden;
					++end;
				}
			}
		}
		i = end + 1;
	}

	// Lines 230 texels across or wider break at the last space or hidden character before that far through them, by
	// the number of characters, or there if there is none
	std::u16string line(text.c_str());
	uint32_t width = 0;
	do
	{
		width = static_cast<uint32_t>(Truncate(_font.GetWidth(line, static_cast<float>(TempleScrollTexture::k_LineHeight)) *
		                                       TempleScrollTexture::k_Stretch));
		if (width < k_WrapWidth)
		{
			_text += line;
			AddNewLine();
		}
		else
		{
			const float ratio = static_cast<float>(k_WrapWidth) / static_cast<float>(width);
			auto breakAt = static_cast<size_t>(Truncate(static_cast<float>(line.size()) * ratio));
			for (size_t j = 0; j < line.size(); ++j)
			{
				if ((line[j] == u' ' || line[j] == k_Hidden) && static_cast<float>(j) / static_cast<float>(line.size()) < ratio)
				{
					breakAt = j;
				}
			}
			// The game would go on breaking a line it can't shorten for ever
			breakAt = std::clamp<size_t>(breakAt, 1, line.size());
			_text += line.substr(0, breakAt);
			AddNewLine();
			if (breakAt < line.size() && (line[breakAt] == u' ' || line[breakAt] == k_Hidden))
			{
				++breakAt;
			}
			line.erase(0, breakAt);
		}
	} while (width >= k_WrapWidth);

	// A gesture's or a control's picture goes on a line of its own, with a blank line after it
	for (const auto& [number, marker] : {std::pair {gesture, k_Gesture}, std::pair {control, k_Control}})
	{
		if (number >= 0)
		{
			_text += marker;
			_text += static_cast<char16_t>(number + k_Hidden);
			AddNewLine();
			AddNewLine();
		}
	}
	AddNewLine();
}

std::u16string TempleScrollText::WithNumber(std::u16string_view text, int32_t number)
{
	const auto at = std::min(text.find(u"$I"), text.size());
	auto result = std::u16string(text.substr(0, at)) + gui::ToUtf16(std::to_string(number));
	if (at + 2 <= text.size())
	{
		result += text.substr(at + 2);
	}
	return result;
}

std::u16string TempleScrollText::WithString(std::u16string_view text, std::u16string_view value)
{
	const auto at = std::min(text.find(u"$s"), text.size());
	auto result = std::u16string(text.substr(0, at)) + std::u16string(value);
	if (at + 2 <= text.size())
	{
		result += text.substr(at + 2);
	}
	return result;
}

int32_t TempleScrollTexture::ClampPosition(int32_t position, uint32_t textHeight)
{
	const auto shown = k_LineHeight * k_VisibleLines;
	const auto last = static_cast<int32_t>(textHeight > shown ? textHeight - shown : 0);
	return std::min(std::max(position, 0), last);
}

void TempleScrollTexture::DrawText(std::span<uint16_t> texels, std::u16string_view text, float x, float y, float size,
                                   float stretch, uint16_t colour, const gui::GameFont& font)
{
	if (text.empty() || texels.size() < static_cast<size_t>(k_Size) * k_Size)
	{
		return;
	}
	// The colour by each coverage from 0 to 15, its alpha kept
	std::array<uint16_t, 16> ramp {};
	for (uint32_t i = 0; i < ramp.size(); ++i)
	{
		const uint32_t r = (colour >> 8) & 0xFu;
		const uint32_t g = (colour >> 4) & 0xFu;
		const uint32_t b = colour & 0xFu;
		ramp.at(i) = static_cast<uint16_t>((colour & 0xF000u) | (((r * i) >> 4) << 8) | (((g * i) >> 4) << 4) | ((b * i) >> 4));
	}
	const float scale = size * stretch * k_FontUnit;
	const auto* space = font.Find(u' ');
	for (const auto c : text)
	{
		if (c == u'\r' || c == u'\n' || c == TempleScrollText::k_Hidden)
		{
			continue;
		}
		if (c == u' ')
		{
			if (space != nullptr)
			{
				x = ((space->right + space->ink + space->left) * scale) + x;
			}
			continue;
		}
		const auto* glyph = font.Find(c);
		if (glyph == nullptr)
		{
			continue;
		}
		x = (scale * glyph->left) + x;
		if (size < k_SmallGlyphSize)
		{
			DrawGlyph(texels, font.GetSmallGlyph(*glyph), glyph->width, x, y, scale, size, ramp, colour);
		}
		// TODO(raffclar): Letters 26 texels and higher are drawn from the half height glyphs; the scrolls' are 10
		x = (scale * glyph->right) + (scale * glyph->ink) + x;
	}
}

uint32_t TempleScrollTexture::LayOut(std::u16string_view text, uint32_t position, const gui::GameFont& font,
                                     const std::function<void(std::u16string_view line, float x, float y)>& line)
{
	// The lines up to the one at the position are passed over, and those after go down from the top, the first of them
	// as far up as the position is through it
	const uint32_t passedOver = position / k_LineHeight;
	const auto offset = static_cast<float>(position % k_LineHeight);
	const auto size = static_cast<float>(k_LineHeight);
	uint32_t lines = 0;
	uint32_t laidOut = 0;
	size_t at = 0;
	while (at < text.size())
	{
		// fn_00799DF0: up to the next "<N>", or "<E>" which ends the text, as long as the line fits its buffer
		size_t length = 0;
		bool ended = false;
		bool found = false;
		while (!found)
		{
			if (length > k_MaxLineLength || at + length >= text.size())
			{
				return lines * k_LineHeight;
			}
			if (text[at + length] == u'<' && at + length + 1 < text.size())
			{
				const auto tag = text[at + length + 1];
				if (tag == u'E' || tag == u'N')
				{
					ended = tag == u'E';
					found = true;
					break;
				}
			}
			++length;
		}
		if (ended)
		{
			break;
		}
		const auto words = text.substr(at, length);
		at += length + 3;
		++lines;
		if (lines <= passedOver)
		{
			continue;
		}
		const float y = (static_cast<float>(k_LineHeight) * static_cast<float>(laidOut)) - offset;
		++laidOut;
		// A gesture's or a control's picture is drawn over the scroll only when the camera is close to it
		if (!words.empty() && (words.front() == TempleScrollText::k_Gesture || words.front() == TempleScrollText::k_Control))
		{
			// TODO(raffclar): The picture, fn_005760C0, a gesture's glyph or the keys and buttons of a control
			continue;
		}
		const float x = (static_cast<float>(k_Size) - (font.GetWidth(words, size) * k_Stretch)) * 0.5f;
		line(words, x, y);
	}
	return lines * k_LineHeight;
}

uint32_t TempleScrollTexture::Draw(std::span<uint16_t> texels, std::span<const uint8_t> parchment, std::u16string_view text,
                                   uint32_t position, const gui::GameFont& font, bool writeText)
{
	constexpr size_t k_Texels = static_cast<size_t>(k_Size) * k_Size;
	if (texels.size() < k_Texels)
	{
		return 0;
	}
	// fn_0079A2C0: the parchment, its rows turned up by the position so it rolls with the text
	const uint32_t roll = position & 0xFFu;
	for (uint32_t y = 0; y < k_Size; ++y)
	{
		const size_t source = static_cast<size_t>((y + roll) & 0xFFu) * k_Size * 3;
		for (uint32_t x = 0; x < k_Size; ++x)
		{
			const size_t at = source + (static_cast<size_t>(x) * 3);
			texels[(static_cast<size_t>(y) * k_Size) + x] =
			    at + 2 < parchment.size() ? ParchmentTexel(parchment[at], parchment[at + 1], parchment[at + 2])
			                              : uint16_t {0xF000};
		}
	}
	const auto size = static_cast<float>(k_LineHeight);
	return LayOut(text, position, font, [&](std::u16string_view line, float x, float y) {
		if (writeText)
		{
			DrawText(texels, line, x + 1.0f, y + 1.0f, size, k_Stretch, k_ShadowColour, font);
			DrawText(texels, line, x, y, size, k_Stretch, k_TextColour, font);
		}
	});
}
