/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The temple's scrolls against a made up font: TempleRoom's text, as SeeIfAnyCuttingUpOfTheTextNeedsDoing breaks it
// into lines, and FormatTextureForScroll's texture of it

#include <cstring>

#include <string>
#include <vector>

#include <3D/OrientedText.h>
#include <3D/TempleScroll.h>
#include <3D/TempleScrolls.h>
#include <3D/TempleSigns.h>
#include <Gui/GameFont.h>
#include <Gui/TextDatabase.h>
#include <gtest/gtest.h>

using namespace openblack;

namespace
{
template <typename T>
void Append(std::vector<uint8_t>& data, T value)
{
	const auto offset = data.size();
	data.resize(offset + sizeof(T));
	std::memcpy(data.data() + offset, &value, sizeof(T));
}

/// A font in the format of data/j0.met and data/j0.fnt, 80 pixels high as j0 is: a solid A 16 pixels wide, its
/// metrics as wide, and a space as wide again
gui::GameFont LoadFont()
{
	constexpr uint32_t k_Height = 80;
	std::vector<uint8_t> met;
	std::vector<uint8_t> fnt;
	Append(met, k_Height);
	met.resize(met.size() + 0x100, 0);
	Append(met, uint32_t {2});
	struct Glyph
	{
		char16_t character;
		uint16_t width;
	};
	for (const auto& glyph : {Glyph {u'A', 16}, Glyph {u' ', 0}})
	{
		const auto offset = static_cast<uint32_t>(fnt.size());
		// Every pixel set: no clear pixels, then all of them
		const uint32_t pixels = glyph.width * k_Height;
		fnt.push_back(0);
		if (pixels > 0)
		{
			fnt.push_back(0xFF);
			Append(fnt, static_cast<uint16_t>(pixels));
		}
		Append(met, static_cast<uint16_t>(glyph.character));
		Append(met, glyph.width);
		Append(met, uint32_t {0});
		Append(met, 0.0f);
		Append(met, 16.0f);
		Append(met, 0.0f);
		Append(met, offset);
		Append(met, static_cast<uint32_t>(fnt.size() - offset));
	}
	auto font = gui::GameFont::Load(met, fnt);
	EXPECT_TRUE(font.has_value());
	return std::move(*font);
}

/// Each row of the parchment its own red
std::vector<uint8_t> Parchment()
{
	std::vector<uint8_t> parchment(256 * 256 * 3);
	for (size_t y = 0; y < 256; ++y)
	{
		for (size_t x = 0; x < 256; ++x)
		{
			parchment[((y * 256) + x) * 3] = static_cast<uint8_t>(y);
			parchment[(((y * 256) + x) * 3) + 1] = 0x80;
			parchment[(((y * 256) + x) * 3) + 2] = 0x40;
		}
	}
	return parchment;
}

uint16_t ParchmentAt(uint32_t row)
{
	return static_cast<uint16_t>(0xF000 | ((row & 0xF0) << 4) | 0x80 | 0x4);
}
} // namespace

TEST(TempleScrollText, EachTextIsALineThenABlankLine)
{
	const auto font = LoadFont();
	TempleScrollText text(font);
	text.Add(u"AA A");
	text.AddNewLine();
	text.Add(u"A");
	text.End();
	EXPECT_EQ(text.GetText(), u"AA A<N><N><N>A<N><N><E>");
}

TEST(TempleScrollText, LongTextBreaksAtTheLastSpaceThatFits)
{
	const auto font = LoadFont();
	// An A is 16 units of 80 across: 2 texels at a size of 10, 3.5 stretched. 230 texels take 65 and a bit.
	std::u16string words;
	for (int i = 0; i < 20; ++i)
	{
		words += u"AAAA ";
	}
	TempleScrollText text(font);
	text.Add(words);
	const auto& result = text.GetText();
	size_t start = 0;
	size_t lines = 0;
	while (true)
	{
		const auto end = result.find(u"<N>", start);
		if (end == std::u16string::npos)
		{
			break;
		}
		const auto line = std::u16string_view(result).substr(start, end - start);
		EXPECT_LT(font.GetWidth(line, 10.0f) * 1.75f, 230.0f) << lines;
		if (!line.empty())
		{
			EXPECT_NE(line.front(), u' ');
		}
		start = end + 3;
		++lines;
	}
	// Two lines of words and the blank line after them
	EXPECT_EQ(lines, 3);
	EXPECT_EQ(result.substr(0, result.find(u"<N>")).size() % 5, 4);
}

TEST(TempleScrollText, WordsItCantShowAreHidden)
{
	const auto font = LoadFont();
	TempleScrollText text(font);
	text.Add(u"A $x A");
	EXPECT_EQ(text.GetText(), u"A \xF8FE\xF8FE A<N><N>");
}

TEST(TempleScrollText, AGestureEndsTheTextAndGoesOnALineOfItsOwn)
{
	const auto font = LoadFont();
	TempleScrollText text(font);
	text.Add(u"A $g3 A");
	const std::u16string gesture {TempleScrollText::k_Gesture, static_cast<char16_t>(TempleScrollText::k_Hidden + 3)};
	EXPECT_EQ(text.GetText(), u"A <N>" + gesture + u"<N><N><N>");
}

TEST(TempleScrollText, ValuesTakeThePlaceOfTheirMarks)
{
	EXPECT_EQ(TempleScrollText::WithNumber(u"Deaths: $I", 179), u"Deaths: 179");
	EXPECT_EQ(TempleScrollText::WithNumber(u"Health: $I%", -5), u"Health: -5%");
	EXPECT_EQ(TempleScrollText::WithString(u"I'm $s greedy.", u"slightly"), u"I'm slightly greedy.");
}

TEST(TempleScrollTexture, ClampsToTheText)
{
	EXPECT_EQ(TempleScrollTexture::ClampPosition(-4, 500), 0);
	EXPECT_EQ(TempleScrollTexture::ClampPosition(100, 500), 100);
	// 24 lines of 10 texels show at once
	EXPECT_EQ(TempleScrollTexture::ClampPosition(400, 500), 260);
	EXPECT_EQ(TempleScrollTexture::ClampPosition(50, 100), 0);
}

TEST(TempleScrollTexture, TheParchmentRollsWithThePosition)
{
	const auto font = LoadFont();
	std::vector<uint16_t> texels(256 * 256);
	EXPECT_EQ(TempleScrollTexture::Draw(texels, Parchment(), u"<E>", 0, font), 0);
	EXPECT_EQ(texels[0], ParchmentAt(0));
	EXPECT_EQ(texels[255 * 256], ParchmentAt(255));

	TempleScrollTexture::Draw(texels, Parchment(), u"<E>", 0x123, font);
	EXPECT_EQ(texels[0], ParchmentAt(0x23));
	EXPECT_EQ(texels[(0xFF - 0x23 + 1) * 256], ParchmentAt(0));
}

TEST(TempleScrollTexture, LinesAreCentredInYellowOverTheirShadows)
{
	const auto font = LoadFont();
	std::vector<uint16_t> texels(256 * 256);
	// The text is its lines' height: two lines here
	EXPECT_EQ(TempleScrollTexture::Draw(texels, Parchment(), u"A<N><N><E>", 0, font), 20);
	// The A is 3.5 texels across from 126.25: texels 126 to 128, 10 high. Where it covers the texel wholly, the
	// texel is the yellow's ramp at full coverage.
	EXPECT_EQ(texels[(5 * 256) + 127], 0xFEE0);
	// Its shadow shows below it
	EXPECT_EQ(texels[(10 * 256) + 128] & 0x0FFF, 0x0000);
	// Away from it the parchment is untouched
	EXPECT_EQ(texels[(5 * 256) + 100], ParchmentAt(5));
}

TEST(TempleScrollTexture, LinesBeforeThePositionArePassedOver)
{
	const auto font = LoadFont();
	std::vector<uint16_t> texels(256 * 256);
	// Turned on by a line and 5 texels, the second line is drawn 5 texels above the top
	EXPECT_EQ(TempleScrollTexture::Draw(texels, Parchment(), u"<N>A<N><E>", 15, font), 20);
	EXPECT_EQ(texels[(2 * 256) + 127], 0xFEE0);
	EXPECT_EQ(texels[(6 * 256) + 127] & 0x0FFF, ParchmentAt(6 + 15) & 0x0FFF);
}

TEST(TempleScrollTexture, ALineTooLongForItsBufferEndsTheText)
{
	const auto font = LoadFont();
	std::vector<uint16_t> texels(256 * 256);
	const std::u16string longLine(64, u' ');
	EXPECT_EQ(TempleScrollTexture::Draw(texels, Parchment(), u"A<N>" + longLine + u"<N>A<N><E>", 0, font), 10);
}

TEST(OrientedText, StandsOffThePageAlongTheAxisLeft)
{
	// The scrolls run along z and down y, off x; the main room's signs along y and down z, off x; the creature room's
	// along x and down z, off y
	EXPECT_EQ(OrientedTextDepthAxis(2, 1), 0);
	EXPECT_EQ(OrientedTextDepthAxis(1, 2), 0);
	EXPECT_EQ(OrientedTextDepthAxis(0, 2), 1);
}

TEST(OrientedText, EachGlyphIsAQuadDownItsLetters)
{
	const auto font = LoadFont();
	std::vector<OrientedTextVertex> vertices;
	// Two As and a space, 10 high: an A starts where the pen is, 16 units and one of 80 across, its height down
	AppendOrientedText(vertices, font, TextFrame {}, 0, 1, u"A A", glm::vec3(1.0f, 2.0f, 3.0f), 10.0f, 1.0f,
	                   {255, 255, 0, 255});
	ASSERT_EQ(vertices.size(), 12);
	EXPECT_FLOAT_EQ(vertices[0].position.x, 1.0f);
	EXPECT_FLOAT_EQ(vertices[0].position.y, 2.0f);
	EXPECT_FLOAT_EQ(vertices[0].position.z, 3.0f);
	EXPECT_FLOAT_EQ(vertices[1].position.y, 12.0f);
	EXPECT_FLOAT_EQ(vertices[2].position.x, 1.0f + (17.0f * 10.0f / 80.0f));
	// The second A comes after the first's 16 units and the space's 16
	EXPECT_FLOAT_EQ(vertices[6].position.x, 1.0f + (32.0f * 10.0f / 80.0f));
	EXPECT_EQ(vertices[0].colour, 0xFF00FFFFu);
}

TEST(TempleSigns, TheMainRoomLightsTheSignOfTheDoorUnderTheCursor)
{
	EXPECT_EQ(TempleSigns::MainRoomSignOfDoor(std::nullopt), std::nullopt);
	EXPECT_EQ(TempleSigns::MainRoomSignOfDoor(0), 0);
	EXPECT_EQ(TempleSigns::MainRoomSignOfDoor(5), 5);
	// The challenge room's door, past the wall of scrolls
	EXPECT_EQ(TempleSigns::MainRoomSignOfDoor(7), 6);
}

TEST(TempleScrolls, TheCreaturesMindListsItsDesires)
{
	const auto font = LoadFont();
	gui::TextDatabase texts;
	const std::u16string script = u"ADD_TEXT(1, NONE, \"HELP_TEXT_ROOM_LIKES_SCROLL_TITLE\", \"Mind\")\n"
	                              u"ADD_TEXT(1, NONE, \"HELP_TEXT_ROOM_PERSONALITY_GREEDY\", \"I'm $s greedy.\")\n"
	                              u"ADD_TEXT(1, NONE, \"HELP_TEXT_CREATURE_ATTITUDE_07\", \"slightly\")\n";
	std::vector<uint8_t> bytes;
	for (const auto c : script)
	{
		bytes.push_back(static_cast<uint8_t>(c & 0xFF));
		bytes.push_back(static_cast<uint8_t>(c >> 8));
	}
	texts.AddScript(bytes);
	auto facts = TempleScrolls::Facts::Mock();
	const auto mind = TempleScrolls::Write(TempleScrolls::Content::CreatureMind, texts, font, facts);
	ASSERT_TRUE(mind.has_value());
	const std::u16string_view expected = u"Mind<N><N><N>I'm slightly greedy.<N><N>";
	EXPECT_EQ(mind->substr(0, expected.size()), expected);

	// Without the player's creature the room leaves the scroll unwritten
	facts.creature.reset();
	EXPECT_FALSE(TempleScrolls::Write(TempleScrolls::Content::CreatureMind, texts, font, facts).has_value());
}
