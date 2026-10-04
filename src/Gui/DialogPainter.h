/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace openblack::graphics
{
class Texture2D;
}

namespace openblack::gui
{

class Canvas;
class GameFont;

/// A rectangle in the dialogs' 800 by 600 space, from min up to but not including max
struct DialogRect
{
	glm::ivec2 min;
	glm::ivec2 max;

	[[nodiscard]] bool Contains(glm::ivec2 point) const noexcept
	{
		return point.x >= min.x && point.y >= min.y && point.x < max.x && point.y < max.y;
	}
	[[nodiscard]] glm::ivec2 Centre() const noexcept { return (min + max) / 2; }
	[[nodiscard]] int Width() const noexcept { return max.x - min.x; }
	[[nodiscard]] int Height() const noexcept { return max.y - min.y; }
};

/// Draws dialogs the way SetupThing does.
///
/// Dialogs are laid out in an 800 by 600 space. On a screen at least that big it is drawn a pixel a unit in the middle
/// of the screen (SetupThing::adjust), on a smaller screen it is scaled down to fit. They are drawn with the front end
/// atlas (data/textures/Front_end_buttons.raw): boxes are filled with the colour at the centre of one of its 16 by 16
/// cells and edged with lines, and soft shadows come from its frame. Text is in the font j0.
class DialogPainter
{
public:
	enum class Justify
	{
		Left,
		Centre,
		Right,
	};

	/// Which lines a bevelled box draws on its top and bottom: without Top the top line is only a stub at each corner
	enum Edges : uint8_t
	{
		Top = 1 << 0,
		Bottom = 1 << 1,
		All = Top | Bottom,
	};

	/// What SetupThing::DrawBigButton draws: a triangle pointing left, or right
	enum class Arrow
	{
		Left,
		Right,
	};

	static constexpr glm::ivec2 k_Size {800, 600};

	/// The text colours of SetupThing's controls
	static constexpr glm::vec4 k_TextColour {208.0f / 255.0f, 208.0f / 255.0f, 208.0f / 255.0f, 1.0f};
	static constexpr glm::vec4 k_FocusColour {1.0f, 1.0f, 1.0f, 1.0f};
	static constexpr glm::vec4 k_HoverColour {192.0f / 255.0f, 128.0f / 255.0f, 32.0f / 255.0f, 1.0f};
	static constexpr glm::vec4 k_ShadowColour {0.0f, 0.0f, 0.0f, 1.0f};
	/// GetBigTextSize, GetMidTextSize and GetSmallTextSize for screens that don't need bigger text
	static constexpr int k_BigTextSize = 35;
	static constexpr int k_MidTextSize = 22;
	static constexpr int k_SmallTextSize = 20;

	DialogPainter(Canvas& canvas, const GameFont& font, const graphics::Texture2D& atlas,
	              const graphics::Texture2D& fontTexture);

	/// Places the dialog space on a screen of a size
	void Begin(glm::u16vec2 resolution);
	/// SetupThing::unadjust: where a point of the screen is in the dialog space
	[[nodiscard]] glm::ivec2 ToDialog(glm::ivec2 screen) const;

	/// SetupThing::DrawAlpha, from 0 to 1: how opaque everything drawn is
	void SetAlpha(float alpha) const noexcept { _alpha = alpha; }
	[[nodiscard]] float GetAlpha() const noexcept { return _alpha; }

	/// SetupThing::DrawBox: a rectangle of the atlas, tinted
	void DrawBox(const DialogRect& rect, glm::vec2 uvMin, glm::vec2 uvMax, glm::vec3 tint = glm::vec3(1.0f)) const;
	/// SetupThing::DrawLine: a one pixel line between two points of the dialog space
	void DrawLine(glm::ivec2 from, glm::ivec2 to, glm::vec4 colour) const;
	/// SetupThing::DrawBevBox: a box filled with the colour of the atlas cell numbered style (16 to a row), edged with
	/// lines two pixels in, in the edge colour of the style (white, white, orange or black for style % 4), all tinted.
	/// With 8 cells to a row instead the box shows the whole 32 pixel cell of the atlas that starts at style's cell,
	/// edged only for the squares SetupThing::DrawBigButton draws (white, orange when hovered).
	void DrawBevelBox(const DialogRect& rect, int style, uint8_t edges, glm::vec4 tint, int cellsToARow = 16) const;
	/// SetupThing::DrawBigButton's square buttons: a black square, ticked when checked, edged orange when hovered and
	/// pushed in when pressed
	void DrawSquare(glm::ivec2 position, int size, bool checked, bool hovered, bool pressed) const;
	/// SetupThing::DrawQuad: a four sided shape in the dialog space, its corners in order round it, each with its own
	/// colour
	void DrawShape(const std::array<glm::vec2, 4>& corners, const std::array<glm::vec4, 4>& colours) const;
	/// One of the 16 player symbols of data/textures/ChooseSymbol.raw, 4 to a row, tinted
	void DrawSymbol(const DialogRect& rect, int index, glm::vec4 tint) const;
	/// One of the 16 mice of data/textures/mousehelp.raw, 4 to a row, mirrored to show the left button
	void DrawMouse(const DialogRect& rect, int cell, bool mirrored) const;
	/// The textures of the symbols and the mice, which DrawSymbol and DrawMouse leave out without
	void SetPictures(const graphics::Texture2D* symbols, const graphics::Texture2D* mice) noexcept
	{
		_symbols = symbols;
		_mice = mice;
	}
	/// SetupThing::DrawBg: the background of a dialog. A see-through grey box, or an opaque one, edged with white lines
	/// and casting a shadow around it, along its top only when edges has Top.
	void DrawBackground(const DialogRect& rect, glm::vec3 tint, bool opaque, uint8_t edges) const;
	/// SetupThing::DrawTab with SetupTabButton's label: a tab on top of a dialog, open onto it when selected, or
	/// just the line along the bottom of a tab without a label
	void DrawTab(const DialogRect& rect, std::u16string_view label, bool selected, bool first, bool last, bool hovered) const;
	/// SetupThing::DrawBigButton: an arrow size pixels square with its shadow, orange when hovered and nearer its shadow
	/// when pressed
	void DrawArrow(glm::ivec2 position, int size, Arrow arrow, bool hovered, bool pressed) const;
	/// FrontEnd's pointer: an animated arrow at a point of the screen, in pixels, on a canvas of its own so that it can
	/// be drawn over everything else
	void DrawPointer(Canvas& canvas, glm::ivec2 screen, uint32_t milliseconds) const;

	/// SetupThing::DrawText: a line of text whose top left, top centre or top right is at position, cut short to fit
	/// width. Returns the width drawn.
	float DrawText(glm::ivec2 position, int width, Justify justify, std::u16string_view text, int size, glm::vec4 colour) const;
	/// SetupThing::DrawTextWrap: text broken into lines that fit the rectangle, each a size high, from its top.
	/// Returns the height drawn.
	float DrawTextWrapped(const DialogRect& rect, bool centred, std::u16string_view text, int size, glm::vec4 colour) const;
	/// SetupThing::GetTextWidth
	[[nodiscard]] float GetTextWidth(std::u16string_view text, int size) const;
	/// SetupThing::GetTextHeight: the height text takes broken into lines width wide
	[[nodiscard]] float GetTextHeight(int width, std::u16string_view text, int size) const;
	/// The lines text breaks into at a width
	[[nodiscard]] std::vector<std::u16string_view> GetLines(int width, std::u16string_view text, int size) const;

private:
	[[nodiscard]] glm::vec2 ToScreen(glm::vec2 dialog) const;
	void DrawString(glm::vec2 screen, std::u16string_view text, float size, glm::vec4 colour) const;

	Canvas& _canvas;
	const GameFont& _font;
	const graphics::Texture2D& _atlas;
	const graphics::Texture2D& _fontTexture;
	const graphics::Texture2D* _symbols {nullptr};
	const graphics::Texture2D* _mice {nullptr};
	glm::vec2 _offset {0.0f, 0.0f};
	float _scale {1.0f};
	mutable float _alpha {1.0f};
};

} // namespace openblack::gui
