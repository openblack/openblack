/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace openblack
{
namespace gui
{
class GameFont;
}

/// The text a temple room writes on one of its scrolls, as the game builds it: lines that fit across the scroll, each
/// ended by "<N>", and "<E>" after the last.
class TempleScrollText
{
public:
	/// The rooms' buffers hold 7000 characters
	static constexpr size_t k_MaxLength = 7000;
	/// The two characters the game puts on a line of their own for a gesture's picture ("$g" and its number) and a
	/// control's ("$m"), which are left blank on the texture
	static constexpr char16_t k_Gesture = u'\xF8FF';
	static constexpr char16_t k_Control = u'\xF900';
	/// The character a word that can't be shown is turned into, which takes no room
	static constexpr char16_t k_Hidden = u'\xF8FE';

	/// The lines are measured in the scrolls' font
	explicit TempleScrollText(const gui::GameFont& font);

	/// Breaks text into lines that fit across the scroll and ends each of them, then a blank line follows. Text that
	/// would take the buffer past its end is left out.
	void Add(std::u16string_view text);
	/// Ends a line. The texts that need the player's creature are just this without one.
	void AddNewLine();
	/// Marks the end of the text
	void End();

	[[nodiscard]] const std::u16string& GetText() const noexcept { return _text; }

	/// Text with its first "$I" the number
	[[nodiscard]] static std::u16string WithNumber(std::u16string_view text, int32_t number);
	/// Text with its first "$s" the string
	[[nodiscard]] static std::u16string WithString(std::u16string_view text, std::u16string_view value);

private:
	const gui::GameFont& _font;
	std::u16string _text;
};

/// A scroll's texture: the parchment of ChallengeScroll.raw with the scroll's text written over it, as the game draws
/// it. Its texels are 16 bits, four each of alpha, red, green and blue from the top.
class TempleScrollTexture
{
public:
	static constexpr uint16_t k_Size = 256;
	/// The scroll shows 24 lines of text, each 10 texels high
	static constexpr uint32_t k_LineHeight = 10;
	static constexpr uint32_t k_VisibleLines = 24;
	/// How much wider than they are high the letters are drawn
	static constexpr float k_Stretch = 1.75f;
	/// Yellow letters over a black shadow a texel down and right
	static constexpr uint16_t k_TextColour = 0xFFF0;
	static constexpr uint16_t k_ShadowColour = 0xF000;

	/// Draws the parchment turned up by position texels, the lines of the text from the line
	/// at position, then their shadows and letters. Gives the height of the text, the lines read times their height.
	/// While the camera is close to the scroll its text is drawn in front of it instead, and the texture is only
	/// the parchment.
	static uint32_t Draw(std::span<uint16_t> texels, std::span<const uint8_t> parchment, std::u16string_view text,
	                     uint32_t position, const gui::GameFont& font, bool writeText = true);
	/// The texture's lines: each line of the text from the line at position, where it goes on the texture,
	/// centred across it. Gives the height of the text.
	static uint32_t LayOut(std::u16string_view text, uint32_t position, const gui::GameFont& font,
	                       const std::function<void(std::u16string_view line, float x, float y)>& line);
	/// A position the scroll can be turned to, from its top to where its last line is at the bottom
	[[nodiscard]] static int32_t ClampPosition(int32_t position, uint32_t textHeight);
	/// Writes text into the texels from x and y, its letters size texels high and
	/// stretched across, blending colour over the texels by each letter's coverage
	static void DrawText(std::span<uint16_t> texels, std::u16string_view text, float x, float y, float size, float stretch,
	                     uint16_t colour, const gui::GameFont& font);
};

} // namespace openblack
