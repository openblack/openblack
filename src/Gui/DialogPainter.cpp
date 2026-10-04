/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "DialogPainter.h"

#include <cmath>

#include <algorithm>
#include <array>

#include <glm/common.hpp>

#include "Canvas.h"
#include "GameFont.h"
#include "Graphics/Texture2D.h"

using namespace openblack::gui;

namespace
{
// The front end atlas is 256 pixels square: its cells are 16 pixels, its frame and arrows 32
constexpr float k_Cell = 1.0f / 16.0f;
constexpr float k_HalfTexel = 1.0f / 512.0f;
constexpr float k_Texel = 1.0f / 256.0f;

// SetupThing::DrawBg's frame: corners and edges of a soft shadow eight pixels wide
constexpr float k_FrameLeft = 0.1875f;
constexpr float k_FrameInnerLeft = 0.2109375f;
constexpr float k_FrameInnerRight = 0.2890625f;
constexpr float k_FrameRight = 0.3125f;
constexpr float k_FrameTop = 0.0f;
constexpr float k_FrameInnerTop = 0.0234375f;
constexpr float k_FrameInnerBottom = 0.1015625f;
constexpr float k_FrameBottom = 0.125f;
constexpr int k_ShadowWidth = 8;

// This comes from SetupThing::DrawBg and DrawTab: see-through backgrounds and tabs, and unselected tabs drawn fainter
constexpr float k_SeeThroughAlpha = 0.8333333f;
constexpr float k_UnselectedTabAlpha = 0.5f;

// The edge colours of SetupThing::DrawBevBox's styles
constexpr std::array<glm::vec3, 4> k_EdgeColours = {{
    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, 128.0f / 255.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
}};

// SetupThing::DrawBigButton's arrows: grey, orange when hovered, and their shadow
constexpr float k_ArrowU = 0.3125f;
constexpr float k_ArrowHoveredU = 0.4375f;
constexpr float k_ArrowShadowU = 0.5625f;
constexpr float k_ArrowSize = 0.125f;

// FrontEnd's pointer: the cells of the atlas, 8 to a row, it turns through, one every 32 milliseconds
constexpr std::array<uint8_t, 8> k_PointerFrames = {9, 10, 11, 12, 41, 49, 57, 23};
constexpr float k_PointerCell = 0.125f;
constexpr float k_PointerExtent = 0.12109375f;
} // namespace

DialogPainter::DialogPainter(Canvas& canvas, const GameFont& font, const graphics::Texture2D& atlas,
                             const graphics::Texture2D& fontTexture)
    : _canvas(canvas)
    , _font(font)
    , _atlas(atlas)
    , _fontTexture(fontTexture)
{
}

void DialogPainter::Begin(glm::u16vec2 resolution)
{
	const auto screen = glm::vec2(resolution);
	const auto size = glm::vec2(k_Size);
	if (screen.x >= size.x && screen.y >= size.y)
	{
		// SetupThing::adjust: a pixel a unit, in the middle of the screen
		_scale = 1.0f;
		_offset = glm::vec2((glm::ivec2(resolution) - k_Size) / 2);
	}
	else
	{
		_scale = std::min(screen.x / size.x, screen.y / size.y);
		_offset = glm::floor((screen - size * _scale) * 0.5f);
	}
	_alpha = 1.0f;
}

glm::ivec2 DialogPainter::ToDialog(glm::ivec2 screen) const
{
	return {glm::floor((glm::vec2(screen) - _offset) / _scale)};
}

glm::vec2 DialogPainter::ToScreen(glm::vec2 dialog) const
{
	return dialog * _scale + _offset;
}

void DialogPainter::DrawBox(const DialogRect& rect, glm::vec2 uvMin, glm::vec2 uvMax, glm::vec3 tint) const
{
	_canvas.DrawQuad(ToScreen(rect.min), ToScreen(rect.max), uvMin, uvMax, glm::vec4(tint, _alpha), &_atlas);
}

void DialogPainter::DrawLine(glm::ivec2 from, glm::ivec2 to, glm::vec4 colour) const
{
	_canvas.DrawLine(glm::ivec2(glm::floor(ToScreen(from))), glm::ivec2(glm::floor(ToScreen(to))),
	                 glm::vec4(glm::vec3(colour), colour.a * _alpha));
}

void DialogPainter::DrawBevelBox(const DialogRect& rect, int style, uint8_t edges, glm::vec4 tint, int cellsToARow) const
{
	const auto cell = glm::vec2(static_cast<float>(style & 0xF), static_cast<float>(style >> 4)) * k_Cell;
	if (cellsToARow != 16)
	{
		// The whole of a bigger cell. Only the squares of SetupThing::DrawBigButton are edged.
		DrawBox(rect, cell, cell + 1.0f / static_cast<float>(cellsToARow), glm::vec3(tint));
		if ((style & 0x1F) != 0xB && (style & 0x1F) != 0xD)
		{
			return;
		}
		const auto colour = glm::vec4(((style & 0x1F) == 0xD ? k_EdgeColours[2] : k_EdgeColours[0]) * glm::vec3(tint), tint.a);
		DrawLine({rect.min.x + 2, rect.min.y + 2}, {rect.max.x - 2, rect.min.y + 2}, colour);
		DrawLine({rect.max.x - 2, rect.max.y - 2}, {rect.min.x + 2, rect.max.y - 2}, colour);
		DrawLine({rect.min.x + 2, rect.max.y - 2}, {rect.min.x + 2, rect.min.y + 2}, colour);
		DrawLine({rect.max.x - 2, rect.min.y + 2}, {rect.max.x - 2, rect.max.y - 2}, colour);
		return;
	}

	// The box takes the colour at the centre of its cell
	const auto centre = cell + k_Cell * 0.5f;
	DrawBox(rect, centre, centre, glm::vec3(tint));

	const auto colour = glm::vec4(k_EdgeColours.at(static_cast<size_t>(style & 3)) * glm::vec3(tint), tint.a);
	const auto left = rect.min.x + 2;
	const auto right = rect.max.x - 2;
	const auto top = rect.min.y + 2;
	const auto bottom = rect.max.y - 2;
	if ((edges & Top) != 0)
	{
		DrawLine({left, top}, {right, top}, colour);
	}
	else
	{
		DrawLine({left, top}, {left + 2, top}, colour);
		DrawLine({right - 2, top}, {right, top}, colour);
	}
	if ((edges & Bottom) != 0)
	{
		DrawLine({right, bottom}, {left, bottom}, colour);
	}
	DrawLine({left, bottom}, {left, top}, colour);
	DrawLine({right, top}, {right, bottom}, colour);
}

void DialogPainter::DrawBackground(const DialogRect& rect, glm::vec3 tint, bool opaque, uint8_t edges) const
{
	const auto alpha = _alpha;
	if (!opaque)
	{
		_alpha *= k_SeeThroughAlpha;
	}
	DrawBevelBox(rect, opaque ? 0x10 : 0, edges, glm::vec4(tint, 1.0f));

	const auto& [min, max] = rect;
	const auto outer = k_ShadowWidth;
	DrawBox({.min = {min.x - outer, min.y - outer}, .max = {min.x, min.y}}, {k_FrameLeft, k_FrameTop},
	        {k_FrameInnerLeft, k_FrameInnerTop});
	DrawBox({.min = {max.x, min.y - outer}, .max = {max.x + outer, min.y}}, {k_FrameInnerRight, k_FrameTop},
	        {k_FrameRight, k_FrameInnerTop});
	DrawBox({.min = {min.x - outer, max.y}, .max = {min.x, max.y + outer}}, {k_FrameLeft, k_FrameInnerBottom},
	        {k_FrameInnerLeft, k_FrameBottom});
	DrawBox({.min = {max.x, max.y}, .max = {max.x + outer, max.y + outer}}, {k_FrameInnerRight, k_FrameInnerBottom},
	        {k_FrameRight, k_FrameBottom});
	if ((edges & Top) != 0)
	{
		DrawBox({.min = {min.x, min.y - outer}, .max = {max.x, min.y}}, {k_FrameInnerLeft, k_FrameTop},
		        {k_FrameInnerRight, k_FrameInnerTop});
	}
	DrawBox({.min = {min.x, max.y}, .max = {max.x, max.y + outer}}, {k_FrameInnerLeft, k_FrameInnerBottom},
	        {k_FrameInnerRight, k_FrameBottom});
	DrawBox({.min = {min.x - outer, min.y}, .max = {min.x, max.y}}, {k_FrameLeft, k_FrameInnerTop},
	        {k_FrameInnerLeft, k_FrameInnerBottom});
	DrawBox({.min = {max.x, min.y}, .max = {max.x + outer, max.y}}, {k_FrameInnerRight, k_FrameInnerTop},
	        {k_FrameRight, k_FrameInnerBottom});
	_alpha = alpha;
}

void DialogPainter::DrawTab(const DialogRect& rect, std::u16string_view label, bool selected, bool first, bool last,
                            bool hovered) const
{
	const auto alpha = _alpha;
	_alpha *= k_SeeThroughAlpha;
	const auto& [min, max] = rect;
	const auto white = glm::vec4(1.0f);
	const auto lineY = max.y + 2;
	const auto lineLeft = min.x + (first ? 2 : 0);
	const auto lineRight = max.x - (last ? 2 : 0);

	// The dialog's top edge runs along the bottom of a tab, but opens into the selected one
	if (selected)
	{
		DrawLine({lineLeft, lineY}, {min.x + 10, lineY}, white);
		DrawLine({max.x - 10, lineY}, {lineRight, lineY}, white);
	}
	else
	{
		DrawLine({lineLeft, lineY}, {lineRight, lineY}, white);
		DrawBox({.min = {min.x, max.y - k_ShadowWidth}, .max = {max.x, max.y}}, {k_FrameInnerLeft, k_FrameTop},
		        {k_FrameInnerRight, k_FrameInnerTop});
		_alpha *= k_UnselectedTabAlpha;
	}

	if (!label.empty())
	{
		// The shadow around the tab, and the corners where it meets the dialog
		const auto inner = k_ShadowWidth;
		const auto joinTop = selected ? 0.125f : 0.15625f;
		const auto joinBottom = joinTop + k_FrameInnerTop;
		DrawBox({.min = {min.x + inner, min.y}, .max = {max.x - inner, min.y + inner}}, {k_FrameInnerLeft, k_FrameTop},
		        {k_FrameInnerRight, k_FrameInnerTop});
		DrawBox({.min = {min.x, min.y + inner}, .max = {min.x + inner, max.y - inner}}, {k_FrameLeft, k_FrameInnerTop},
		        {k_FrameInnerLeft, k_FrameInnerBottom});
		DrawBox({.min = {max.x - inner, min.y + inner}, .max = {max.x, max.y - inner}}, {k_FrameInnerRight, k_FrameInnerTop},
		        {k_FrameRight, k_FrameInnerBottom});
		DrawBox({.min = {min.x, min.y}, .max = {min.x + inner, min.y + inner}}, {k_FrameLeft, k_FrameTop},
		        {k_FrameInnerLeft, k_FrameInnerTop});
		DrawBox({.min = {max.x - inner, min.y}, .max = {max.x, min.y + inner}}, {k_FrameInnerRight, k_FrameTop},
		        {k_FrameRight, k_FrameInnerTop});
		DrawBox({.min = {min.x, max.y - inner}, .max = {min.x + inner, max.y}}, {0.9375f, joinTop}, {0.9609375f, joinBottom});
		DrawBox({.min = {max.x - inner, max.y - inner}, .max = {max.x, max.y}}, {0.9765625f, joinTop}, {1.0f, joinBottom});
		DrawBevelBox({.min = {min.x + inner, min.y + inner}, .max = {max.x - inner, max.y}}, 0, Top, white);
		DrawLine({max.x - 10, lineY}, {max.x - 10, max.y - 4}, white);
		DrawLine({min.x + 10, lineY}, {min.x + 10, max.y - 4}, white);

		// SetupTabButton::Draw: the label at the mid text size, smaller if it doesn't fit, in the middle of the tab. The
		// size and the centring are as the original draws them.
		const DialogRect labelRect {.min = {min.x + 8, min.y + 7}, .max = {max.x - 8, max.y + 1}};
		auto size = k_MidTextSize;
		while (size > k_SmallTextSize / 2 &&
		       GetTextHeight(labelRect.Width(), label, size) > static_cast<float>(labelRect.Height() - 6))
		{
			--size;
		}
		const auto offset = static_cast<int>(
		    std::floor((static_cast<float>(labelRect.Height()) - GetTextHeight(labelRect.Width(), label, size)) * 0.5f));
		const DialogRect text {.min = {labelRect.min.x, labelRect.min.y + offset}, .max = labelRect.max};
		const DialogRect shadow {.min = text.min + 1, .max = text.max + 1};
		DrawTextWrapped(shadow, true, label, size, k_ShadowColour);
		DrawTextWrapped(shadow, true, label, size, k_ShadowColour);
		DrawTextWrapped(text, true, label, size, hovered ? k_HoverColour : k_FocusColour);
	}
	_alpha = alpha;
}

void DialogPainter::DrawArrow(glm::ivec2 position, int size, Arrow arrow, bool hovered, bool pressed) const
{
	const auto extent = k_ArrowSize - k_Texel;
	auto uvMin = glm::vec2(hovered ? k_ArrowHoveredU : k_ArrowU, 0.0f) + k_HalfTexel;
	auto uvMax = uvMin + extent;
	auto shadowMin = glm::vec2(k_ArrowShadowU, 0.0f);
	auto shadowMax = shadowMin + k_ArrowSize;
	if (arrow == Arrow::Right)
	{
		// The same arrow turned round
		std::swap(uvMin, uvMax);
		std::swap(shadowMin, shadowMax);
	}
	const auto drop = pressed ? 2 : 4;
	const auto lift = pressed ? 0 : -2;
	DrawBox({.min = position + drop, .max = position + size + drop}, shadowMin, shadowMax);
	DrawBox({.min = position + lift, .max = position + size + lift}, uvMin, uvMax);
}

void DialogPainter::DrawSquare(glm::ivec2 position, int size, bool checked, bool hovered, bool pressed) const
{
	// SetupThing::DrawBigButton's first two styles: the black squares of the atlas, or below them those ticked
	const auto style = (checked ? 0x20 : 0) + (hovered ? 2 : 0) + 0xB;
	const auto push = pressed ? 2 : 0;
	DrawBevelBox({.min = position + push - 1, .max = position + size + push - 3}, style, All, glm::vec4(1.0f), 8);
}

void DialogPainter::DrawShape(const std::array<glm::vec2, 4>& corners, const std::array<glm::vec4, 4>& colours) const
{
	std::array<glm::vec2, 4> screen {};
	std::array<glm::vec4, 4> faded {};
	for (size_t i = 0; i < 4; ++i)
	{
		screen.at(i) = ToScreen(corners.at(i));
		faded.at(i) = glm::vec4(glm::vec3(colours.at(i)), colours.at(i).a * _alpha);
	}
	_canvas.DrawShape(screen, faded);
}

void DialogPainter::DrawSymbol(const DialogRect& rect, int index, glm::vec4 tint) const
{
	if (_symbols == nullptr)
	{
		return;
	}
	constexpr float k_Grid = 0.25f;
	const auto column = index % 4;
	const auto row = index / 4;
	const auto cell = (glm::vec2(static_cast<float>(column), static_cast<float>(row)) * k_Grid) + k_HalfTexel;
	_canvas.DrawQuad(ToScreen(rect.min), ToScreen(rect.max), cell, cell + k_Grid - k_Texel,
	                 glm::vec4(glm::vec3(tint), tint.a * _alpha), _symbols);
}

void DialogPainter::DrawMouse(const DialogRect& rect, int cell, bool mirrored) const
{
	if (_mice == nullptr)
	{
		return;
	}
	constexpr float k_Grid = 0.25f;
	auto uvMin = glm::vec2(static_cast<float>(cell & 3), static_cast<float>(cell >> 2)) * k_Grid;
	auto uvMax = uvMin + k_Grid;
	if (mirrored)
	{
		std::swap(uvMin.x, uvMax.x);
	}
	_canvas.DrawQuad(ToScreen(rect.min), ToScreen(rect.max), uvMin, uvMax, glm::vec4(1.0f, 1.0f, 1.0f, _alpha), _mice);
}

void DialogPainter::DrawPointer(glm::ivec2 screen, uint32_t milliseconds) const
{
	const auto frame = k_PointerFrames.at((milliseconds >> 5) & 7);
	const auto cell = glm::vec2(static_cast<float>(frame & 7), static_cast<float>(frame >> 3)) * k_PointerCell;
	const auto min = glm::vec2(screen.x - 2, screen.y);
	_canvas.DrawQuad(min, min + 32.0f, cell + k_HalfTexel, cell + k_PointerExtent, glm::vec4(1.0f), &_atlas);
}

float DialogPainter::GetTextWidth(std::u16string_view text, int size) const
{
	return _font.GetWidth(text, static_cast<float>(size));
}

float DialogPainter::GetTextHeight(int width, std::u16string_view text, int size) const
{
	if (text.empty())
	{
		return 0.0f;
	}
	const auto lines = _font.Wrap(text, static_cast<float>(size), static_cast<float>(width));
	return static_cast<float>(lines.size() * size);
}

std::vector<std::u16string_view> DialogPainter::GetLines(int width, std::u16string_view text, int size) const
{
	return _font.Wrap(text, static_cast<float>(size), static_cast<float>(width));
}

float DialogPainter::DrawText(glm::ivec2 position, int width, Justify justify, std::u16string_view text, int size,
                              glm::vec4 colour) const
{
	// Cut short to fit
	auto shown = text;
	auto shownWidth = GetTextWidth(shown, size);
	while (!shown.empty() && shownWidth > static_cast<float>(width))
	{
		shown.remove_suffix(1);
		shownWidth = GetTextWidth(shown, size);
	}
	auto x = static_cast<float>(position.x);
	if (justify == Justify::Centre)
	{
		x -= std::floor(shownWidth * 0.5f);
	}
	else if (justify == Justify::Right)
	{
		x -= std::floor(shownWidth);
	}
	DrawString(ToScreen({x, static_cast<float>(position.y)}), shown, static_cast<float>(size) * _scale, colour);
	return shownWidth;
}

float DialogPainter::DrawTextWrapped(const DialogRect& rect, bool centred, std::u16string_view text, int size,
                                     glm::vec4 colour) const
{
	const auto lines = _font.Wrap(text, static_cast<float>(size), static_cast<float>(rect.Width()));
	auto y = static_cast<float>(rect.min.y);
	for (const auto& line : lines)
	{
		auto x = static_cast<float>(rect.min.x);
		if (centred)
		{
			x += std::floor((static_cast<float>(rect.Width()) - GetTextWidth(line, size)) * 0.5f);
		}
		DrawString(ToScreen({x, y}), line, static_cast<float>(size) * _scale, colour);
		y += static_cast<float>(size);
	}
	return y - static_cast<float>(rect.min.y);
}

void DialogPainter::DrawString(glm::vec2 screen, std::u16string_view text, float size, glm::vec4 colour) const
{
	const auto scale = size / static_cast<float>(_font.GetHeight());
	const auto atlasSize = glm::vec2(_font.GetAtlasSize());
	auto pen = glm::floor(screen);
	for (const auto c : text)
	{
		const auto* glyph = _font.Find(c);
		if (glyph == nullptr)
		{
			continue;
		}
		// The atlas holds the glyph at half size
		const auto cellSize = glm::vec2(glyph->atlasMax - glyph->atlasMin) * 2.0f * scale;
		const auto min = glm::vec2(pen.x + (glyph->left * scale), pen.y);
		_canvas.DrawQuad(min, min + cellSize, glm::vec2(glyph->atlasMin) / atlasSize, glm::vec2(glyph->atlasMax) / atlasSize,
		                 glm::vec4(glm::vec3(colour), colour.a * _alpha), &_fontTexture);
		pen.x += (glyph->left + glyph->ink + glyph->right) * scale;
	}
}
