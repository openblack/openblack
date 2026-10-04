/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The game's interface against a made up script and font: its texts, its font and the Escape menu

#include <cstring>

#include <string>
#include <vector>

#include <Gui/GameFont.h>
#include <Gui/GameMenu.h>
#include <Gui/TextDatabase.h>
#include <SDL_keycode.h>
#include <gtest/gtest.h>

using namespace openblack::gui;

namespace
{
std::vector<uint8_t> Utf16Script(std::u16string_view text, bool byteOrderMark = true)
{
	std::vector<uint8_t> bytes;
	if (byteOrderMark)
	{
		bytes.insert(bytes.end(), {0xFF, 0xFE});
	}
	for (const auto c : text)
	{
		bytes.push_back(static_cast<uint8_t>(c & 0xFF));
		bytes.push_back(static_cast<uint8_t>(c >> 8));
	}
	return bytes;
}

struct FakeGlyph
{
	char16_t character;
	uint16_t width;
	float left;
	float ink;
	float right;
	/// Rows of '#' and '.', as tall as the font
	std::vector<std::string> rows;
};

/// A font in the format of data/j0.met and data/j0.fnt
struct FakeFont
{
	uint32_t height;
	std::vector<FakeGlyph> glyphs;
	std::vector<uint8_t> met;
	std::vector<uint8_t> fnt;

	template <typename T>
	static void Append(std::vector<uint8_t>& data, T value)
	{
		const auto offset = data.size();
		data.resize(offset + sizeof(T));
		std::memcpy(data.data() + offset, &value, sizeof(T));
	}

	void Build()
	{
		met.clear();
		fnt.clear();
		Append(met, height);
		std::vector<uint8_t> name(0x100, 0);
		const std::u16string_view fontName = u"Fake Sans";
		for (size_t i = 0; i < fontName.size(); ++i)
		{
			name[i * 2] = static_cast<uint8_t>(fontName[i]);
		}
		met.insert(met.end(), name.begin(), name.end());
		Append(met, static_cast<uint32_t>(glyphs.size()));
		for (const auto& glyph : glyphs)
		{
			// Runs of clear and set pixels in turn, starting with clear
			const auto offset = static_cast<uint32_t>(fnt.size());
			bool set = false;
			uint32_t run = 0;
			auto flush = [this, &run]() {
				if (run >= 0xFF)
				{
					fnt.push_back(0xFF);
					Append(fnt, static_cast<uint16_t>(run));
				}
				else
				{
					fnt.push_back(static_cast<uint8_t>(run));
				}
				run = 0;
			};
			for (const auto& row : glyph.rows)
			{
				for (const auto pixel : row)
				{
					if ((pixel == '#') != set)
					{
						flush();
						set = !set;
					}
					++run;
				}
			}
			flush();

			Append(met, static_cast<uint16_t>(glyph.character));
			Append(met, glyph.width);
			Append(met, static_cast<uint32_t>(0xCCCC0000));
			Append(met, glyph.left);
			Append(met, glyph.ink);
			Append(met, glyph.right);
			Append(met, offset);
			Append(met, static_cast<uint32_t>(fnt.size() - offset));
		}
	}
};

/// Four pixels high: a solid A, a checkered B, a space, a question mark, a hyphen and a wide W
FakeFont MakeFont()
{
	FakeFont font {.height = 4, .glyphs = {}, .met = {}, .fnt = {}};
	font.glyphs = {
	    {.character = u'A', .width = 2, .left = 1.0f, .ink = 2.0f, .right = 1.0f, .rows = {"##", "##", "##", "##"}},
	    {.character = u'B', .width = 2, .left = 0.0f, .ink = 2.0f, .right = 0.0f, .rows = {"#.", ".#", "#.", ".#"}},
	    {.character = u' ', .width = 0, .left = 1.0f, .ink = 0.0f, .right = 1.0f, .rows = {"", "", "", ""}},
	    {.character = u'?', .width = 2, .left = 0.0f, .ink = 2.0f, .right = 2.0f, .rows = {"##", "..", "#.", ".."}},
	    {.character = u'-', .width = 2, .left = 0.0f, .ink = 2.0f, .right = 0.0f, .rows = {"..", "##", "..", ".."}},
	    {.character = u'W', .width = 4, .left = 0.0f, .ink = 8.0f, .right = 0.0f, .rows = {"####", "####", "####", "####"}},
	};
	font.Build();
	return font;
}

GameFont LoadFont()
{
	auto fake = MakeFont();
	auto font = GameFont::Load(fake.met, fake.fnt);
	EXPECT_TRUE(font.has_value());
	return std::move(*font);
}

std::u16string ToString(const std::vector<std::u16string_view>& lines)
{
	std::u16string result;
	for (const auto& line : lines)
	{
		result += u"[";
		result += line;
		result += u"]";
	}
	return result;
}
} // namespace

TEST(TextDatabase, ReadsTheAddTextLines)
{
	TextDatabase texts;
	const auto added =
	    texts.AddScript(Utf16Script(u"// A comment ADD_TEXT\r\n"
	                                u"ADD_TEXT( 1, HELP_TEXT_NARRATOR_NONE, \"FIRST\", \"Continue Game\")\r\n"
	                                u"ADD_TEXT(1,HELP_TEXT_NARRATOR_NONE,\"SECOND\",\"Line one\\nline \\\"two\\\"\")\r\n"
	                                u"ADD_TEXT( 1, HELP_TEXT_NARRATOR_NONE, \"BROKEN\")\r\n"
	                                u"MY_ADD_TEXT( 1, NONE, \"NOT\", \"read\")\r\n"));
	EXPECT_EQ(added, 2);
	EXPECT_EQ(texts.Get("FIRST"), u"Continue Game");
	EXPECT_EQ(texts.Get("SECOND"), u"Line one\nline \"two\"");
	EXPECT_TRUE(texts.Get("BROKEN").empty());
	EXPECT_TRUE(texts.Get("NOT").empty());
	EXPECT_TRUE(texts.Get("MISSING").empty());

	// A later script, with no byte order mark, replaces a text of the same name
	texts.AddScript(Utf16Script(u"ADD_TEXT(1, NONE, \"FIRST\", \"Patched\")", false));
	EXPECT_EQ(texts.Get("FIRST"), u"Patched");
	EXPECT_EQ(texts.GetCount(), 2);
}

TEST(TextDatabase, ConvertsBetweenUtf8AndUtf16)
{
	const std::u16string text = u"Bläck & Wh€te \U0001F600";
	EXPECT_EQ(ToUtf16(ToUtf8(text)), text);
	EXPECT_EQ(ToUtf8(u"Black & White"), "Black & White");
}

TEST(GameFont, DecodesTheGlyphsAtHalfSize)
{
	const auto font = LoadFont();
	EXPECT_EQ(font.GetName(), "Fake Sans");
	EXPECT_EQ(font.GetHeight(), 4);
	ASSERT_EQ(font.GetGlyphs().size(), 6);

	const auto coverage = [&font](char16_t c) {
		const auto* glyph = font.Find(c);
		std::vector<int> result;
		for (auto y = glyph->atlasMin.y; y < glyph->atlasMax.y; ++y)
		{
			for (auto x = glyph->atlasMin.x; x < glyph->atlasMax.x; ++x)
			{
				result.push_back(font.GetAtlas()[(static_cast<size_t>(y) * font.GetAtlasSize().x) + x]);
			}
		}
		return result;
	};
	// Each pixel of the atlas is the average of four of the glyph
	EXPECT_EQ(coverage(u'A'), (std::vector<int> {255, 255}));
	EXPECT_EQ(coverage(u'B'), (std::vector<int> {127, 127}));
	EXPECT_EQ(coverage(u'?'), (std::vector<int> {127, 63}));
	EXPECT_EQ(coverage(u'W'), (std::vector<int> {255, 255, 255, 255}));
	EXPECT_TRUE(coverage(u' ').empty());
}

TEST(GameFont, FallsBackToTheQuestionMark)
{
	const auto font = LoadFont();
	EXPECT_EQ(font.Find(u'Z'), font.Find(u'?'));
	EXPECT_EQ(font.Find(u'A')->character, u'A');
}

TEST(GameFont, MeasuresInLinesOfASize)
{
	const auto font = LoadFont();
	// The pen moves left + ink + right, scaled from the font's height to the size
	EXPECT_FLOAT_EQ(font.GetWidth(u"A", 4.0f), 4.0f);
	EXPECT_FLOAT_EQ(font.GetWidth(u"AB", 8.0f), 12.0f);
	EXPECT_FLOAT_EQ(font.GetWidth(u"A A", 4.0f), 10.0f);
	EXPECT_FLOAT_EQ(font.GetWidth(u"A\nA", 4.0f), 8.0f);
}

TEST(GameFont, WrapsAtSpacesHyphensAndLineBreaks)
{
	const auto font = LoadFont();
	// At size 4: A and B 4 and 2 wide, a space 2, a hyphen 2, W 8
	EXPECT_EQ(ToString(font.Wrap(u"AA AA", 4.0f, 10.0f)), u"[AA][AA]");
	EXPECT_EQ(ToString(font.Wrap(u"AA AA", 4.0f, 18.0f)), u"[AA AA]");
	EXPECT_EQ(ToString(font.Wrap(u"AA-BB", 4.0f, 11.0f)), u"[AA-][BB]");
	EXPECT_EQ(ToString(font.Wrap(u"A\nB", 4.0f, 100.0f)), u"[A][B]");
	EXPECT_EQ(ToString(font.Wrap(u"A\r\nB", 4.0f, 100.0f)), u"[A][B]");
	// A word too long for a line breaks where it reaches the end
	EXPECT_EQ(ToString(font.Wrap(u"WWW", 4.0f, 17.0f)), u"[WW][W]");
}

TEST(GameFont, RejectsFilesThatDontFit)
{
	auto fake = MakeFont();
	EXPECT_FALSE(GameFont::Load(std::span(fake.met).first(100), fake.fnt).has_value());
	EXPECT_FALSE(GameFont::Load(fake.met, std::span(fake.fnt).first(3)).has_value());
}

namespace
{
TextDatabase MenuTexts()
{
	std::u16string script;
	const std::vector<std::pair<std::u16string, std::u16string>> texts = {
	    {u"HELP_TEXT_DIALOG_WELCOME", u"Welcome %s"},
	    {u"HELP_TEXT_DIALOG_CONTINUEGAME", u"Continue"},
	    {u"HELP_TEXT_DIALOG_ADDITION_169", u"Skirmish"},
	    {u"HELP_TEXT_DIALOG_JOINGONLINE", u"Online"},
	    {u"HELP_TEXT_DIALOG_OPTIONS", u"Options"},
	    {u"HELP_TEXT_DIALOG_QUIT", u"Quit"},
	    {u"HELP_TEXT_DIALOG_ADDITION_88", u"Main"},
	    {u"HELP_TEXT_PAUSE_STATS_151", u"Stats"},
	    {u"HELP_TEXT_DIALOG_PLAYERLIST", u"Players"},
	    {u"HELP_TEXT_FRONT_END_06", u"Advanced"},
	    {u"HELP_TEXT_DIALOG_CONTROLOPTIONS", u"Controls"},
	    {u"HELP_TEXT_DIALOG_QUIT_QUESTION", u"Sure?"},
	    {u"HELP_TEXT_DIALOG_AREYOUSUREQUIT", u"Quit game?"},
	    {u"HELP_TEXT_REQUESTER_BOXES_01", u"Yes."},
	    {u"HELP_TEXT_REQUESTER_BOXES_02", u"No."},
	    {u"HELP_TEXT_DIALOG_BACK", u"Back"},
	    {u"HELP_TEXT_DIALOG_DETAIL_00", u"Minimum"},
	    {u"HELP_TEXT_DIALOG_DETAIL_04", u"Maximum"},
	    {u"HELP_TEXT_ACTIONS_HELP", u"Help"},
	    {u"HELP_TEXT_ACTIONS_SELECT", u"Move"},
	    {u"HELP_TEXT_ACTIONS_LEFTMOUSEBUTTON", u"LMB"},
	    {u"HELP_TEXT_ACTIONS_ZOOMON", u"Zoom On"},
	};
	for (const auto& [name, text] : texts)
	{
		script.append(u"ADD_TEXT(1, N, \"").append(name).append(u"\", \"").append(text).append(u"\")\n");
	}
	TextDatabase database;
	database.AddScript(Utf16Script(script));
	return database;
}

using Action = GameMenu::Action;
using Page = GameMenu::Page;

/// Presses and lets go of the mouse button at a point
Action Click(GameMenu& menu, glm::ivec2 point)
{
	menu.MouseMove(point);
	menu.MouseDown(point);
	return menu.MouseUp(point);
}

glm::ivec2 ButtonCentre(size_t index)
{
	return GameMenu::GetButtonRect(index).Centre();
}

glm::ivec2 TabCentre(size_t index)
{
	return Dialog::GetTabRect(index).Centre();
}

struct MenuFixture
{
	TextDatabase texts = MenuTexts();
	GameFont font = LoadFont();
	GameMenu menu {texts, font, u"aaa", MenuSettings {}};

	MenuFixture() { menu.Open(); }
};

// The arrows and squares are hit in their middle
constexpr glm::ivec2 k_Back {50, 550};
constexpr glm::ivec2 k_OptionsQuit {750, 550};
} // namespace

TEST(GameMenu, IsLaidOutAsTheOriginal)
{
	// MainMenu::InitControls: 440 by 70 buttons 80 apart, from 145 down
	EXPECT_EQ(GameMenu::GetButtonRect(0).min, glm::ivec2(180, 145));
	EXPECT_EQ(GameMenu::GetButtonRect(0).max, glm::ivec2(620, 215));
	EXPECT_EQ(GameMenu::GetButtonRect(4).min, glm::ivec2(180, 465));
	// AddMainMenuTabs: a fifth of the box's width each, along its top
	EXPECT_EQ(Dialog::GetTabRect(0).min, glm::ivec2(10, 10));
	EXPECT_EQ(Dialog::GetTabRect(0).max, glm::ivec2(166, 50));
	EXPECT_EQ(Dialog::GetTabRect(4).max, glm::ivec2(790, 50));
}

TEST(GameMenu, GreetsThePlayer)
{
	const MenuFixture f;
	EXPECT_EQ(f.menu.GetWelcome(), u"Welcome aaa");
	EXPECT_EQ(f.menu.GetButtonLabel(0), u"Continue");
	EXPECT_EQ(f.menu.GetButtonLabel(4), u"Quit");
	// The player's creature takes their name until they give it another
	EXPECT_EQ(f.menu.GetSettings().creatureName, u"aaa");
}

TEST(GameMenu, FadesInAndOut)
{
	const auto texts = MenuTexts();
	const auto font = LoadFont();
	GameMenu menu(texts, font, u"aaa", MenuSettings {});
	EXPECT_FALSE(menu.IsVisible());
	menu.Open();
	EXPECT_TRUE(menu.IsOpen());
	menu.Update(0.6f);
	menu.Close();
	EXPECT_FALSE(menu.IsOpen());
	EXPECT_TRUE(menu.IsVisible());
	menu.Update(0.1f);
	EXPECT_TRUE(menu.IsVisible());
	menu.Update(0.11f);
	EXPECT_FALSE(menu.IsVisible());
}

TEST(GameMenu, ButtonsActWhenLetGoOverTheOneTheyWentDownOn)
{
	MenuFixture f;
	EXPECT_EQ(Click(f.menu, ButtonCentre(0)), Action::Continue);
	EXPECT_EQ(Click(f.menu, ButtonCentre(1)), Action::StartSkirmish);
	EXPECT_EQ(Click(f.menu, ButtonCentre(2)), Action::JoinOnline);
	EXPECT_EQ(Click(f.menu, TabCentre(1)), Action::Statistics);
	// Tabs without a label take no clicks
	EXPECT_EQ(Click(f.menu, TabCentre(3)), Action::None);
	// Between the buttons
	EXPECT_EQ(Click(f.menu, {400, 220}), Action::None);

	// Let go over another button
	f.menu.MouseDown(ButtonCentre(0));
	f.menu.MouseMove(ButtonCentre(2));
	EXPECT_EQ(f.menu.MouseUp(ButtonCentre(2)), Action::None);

	// Closed, it takes no clicks
	f.menu.Close();
	EXPECT_EQ(Click(f.menu, ButtonCentre(0)), Action::None);
}

TEST(GameMenu, OptionsOpenTheOptionsTabs)
{
	MenuFixture f;
	EXPECT_EQ(Click(f.menu, ButtonCentre(3)), Action::None);
	EXPECT_EQ(f.menu.GetPage(), Page::Options);
	Click(f.menu, TabCentre(2));
	EXPECT_EQ(f.menu.GetPage(), Page::Players);
	Click(f.menu, TabCentre(3));
	EXPECT_EQ(f.menu.GetPage(), Page::Advanced);
	Click(f.menu, TabCentre(4));
	EXPECT_EQ(f.menu.GetPage(), Page::Controls);
	Click(f.menu, TabCentre(1));
	EXPECT_EQ(f.menu.GetPage(), Page::Options);
	// Back, or the Main Menu tab, goes back to the first page
	Click(f.menu, k_Back);
	EXPECT_EQ(f.menu.GetPage(), Page::Main);
	Click(f.menu, ButtonCentre(3));
	Click(f.menu, TabCentre(0));
	EXPECT_EQ(f.menu.GetPage(), Page::Main);
	// Opening the menu again starts on the first page
	Click(f.menu, ButtonCentre(3));
	f.menu.Close();
	f.menu.Open();
	EXPECT_EQ(f.menu.GetPage(), Page::Main);
}

TEST(GameMenu, QuittingAsksFirst)
{
	MenuFixture f;
	EXPECT_EQ(Click(f.menu, ButtonCentre(4)), Action::None);
	EXPECT_TRUE(f.menu.IsAskingToQuit());
	// The menu's buttons are behind the question
	EXPECT_EQ(Click(f.menu, ButtonCentre(0)), Action::None);

	// Escape backs out of the question
	EXPECT_EQ(f.menu.Escape(), Action::None);
	EXPECT_FALSE(f.menu.IsAskingToQuit());
	EXPECT_TRUE(f.menu.IsOpen());

	// No backs out too, on the arrow or its label to its left
	Click(f.menu, ButtonCentre(4));
	EXPECT_EQ(Click(f.menu, {584, 354}), Action::None);
	EXPECT_FALSE(f.menu.IsAskingToQuit());
	Click(f.menu, ButtonCentre(4));
	EXPECT_EQ(Click(f.menu, {560, 354}), Action::None);
	EXPECT_FALSE(f.menu.IsAskingToQuit());

	// Yes quits, on its label to its right too
	Click(f.menu, ButtonCentre(4));
	EXPECT_EQ(Click(f.menu, {240, 354}), Action::Quit);
	EXPECT_FALSE(f.menu.IsAskingToQuit());
	Click(f.menu, ButtonCentre(4));
	EXPECT_EQ(Click(f.menu, {216, 354}), Action::Quit);

	// The options' Quit asks too
	Click(f.menu, ButtonCentre(3));
	EXPECT_EQ(Click(f.menu, k_OptionsQuit), Action::None);
	EXPECT_TRUE(f.menu.IsAskingToQuit());
	EXPECT_EQ(Click(f.menu, {216, 354}), Action::Quit);
}

TEST(GameMenu, EscapeBacksOut)
{
	MenuFixture f;
	Click(f.menu, ButtonCentre(3));
	EXPECT_EQ(f.menu.Escape(), Action::None);
	EXPECT_EQ(f.menu.GetPage(), Page::Main);
	EXPECT_EQ(f.menu.Escape(), Action::Continue);
	f.menu.Close();
	EXPECT_EQ(f.menu.Escape(), Action::None);
}

TEST(GameMenu, ClicksOnEveryControlThatActs)
{
	MenuFixture f;
	EXPECT_FALSE(f.menu.TakeClicked());
	Click(f.menu, ButtonCentre(2));
	EXPECT_TRUE(f.menu.TakeClicked());
	EXPECT_FALSE(f.menu.TakeClicked());
	// Opening the question and answering it click too
	Click(f.menu, ButtonCentre(4));
	EXPECT_TRUE(f.menu.TakeClicked());
	Click(f.menu, {584, 354});
	EXPECT_TRUE(f.menu.TakeClicked());
	Click(f.menu, TabCentre(1));
	EXPECT_TRUE(f.menu.TakeClicked());
	// Missing, or letting go elsewhere, doesn't
	Click(f.menu, {400, 220});
	EXPECT_FALSE(f.menu.TakeClicked());
	f.menu.MouseDown(ButtonCentre(0));
	f.menu.MouseUp(ButtonCentre(2));
	EXPECT_FALSE(f.menu.TakeClicked());
	// Nor does Escape
	f.menu.Escape();
	EXPECT_FALSE(f.menu.TakeClicked());
}

TEST(GameMenu, SlidersStepOrFollowTheKnob)
{
	MenuFixture f;
	Click(f.menu, ButtonCentre(3));
	f.menu.TakeSettingsChanged();
	// The SFX slider is 300 by 30 at 250, 140: at full volume its knob is at its right end
	EXPECT_FLOAT_EQ(f.menu.GetSettings().sfxVolume, 1.0f);
	Click(f.menu, {300, 155});
	EXPECT_NEAR(f.menu.GetSettings().sfxVolume, 0.9f, 1e-5f);
	EXPECT_TRUE(f.menu.TakeSettingsChanged());

	// Dragging the knob, at 250 + 270 * 0.9 = 493, 27 pixels left takes a tenth off
	f.menu.MouseDown({500, 155});
	f.menu.MouseMove({473, 155});
	f.menu.Update(0.0f);
	f.menu.MouseUp({473, 155});
	EXPECT_NEAR(f.menu.GetSettings().sfxVolume, 0.8f, 1e-5f);

	// The music slider below
	Click(f.menu, {300, 215});
	EXPECT_NEAR(f.menu.GetSettings().musicVolume, 0.9f, 1e-5f);
}

TEST(GameMenu, SelectorsAndCheckBoxesChangeTheSettings)
{
	MenuFixture f;
	Click(f.menu, ButtonCentre(3));
	// The detail: the right arrow steps on and wraps round, the left steps back, the button steps on
	EXPECT_EQ(f.menu.GetSettings().detail, 4);
	Click(f.menu, {524, 365});
	EXPECT_EQ(f.menu.GetSettings().detail, 0);
	Click(f.menu, {276, 365});
	EXPECT_EQ(f.menu.GetSettings().detail, 4);
	Click(f.menu, {400, 365});
	EXPECT_EQ(f.menu.GetSettings().detail, 0);

	// AutoSave's square at 285, 450
	EXPECT_TRUE(f.menu.GetSettings().autoSave);
	Click(f.menu, {297, 462});
	EXPECT_FALSE(f.menu.GetSettings().autoSave);

	// Advanced: the left handed hand at 185, 450 and the help level
	Click(f.menu, TabCentre(3));
	Click(f.menu, {197, 462});
	EXPECT_TRUE(f.menu.GetSettings().leftHandedHand);
	EXPECT_EQ(f.menu.GetSettings().helpLevel, 3);
	Click(f.menu, {524, 165});
	EXPECT_EQ(f.menu.GetSettings().helpLevel, 4);
	EXPECT_TRUE(f.menu.TakeSettingsChanged());
}

TEST(GameMenu, PlayersNameTheirCreatureAndPickASymbol)
{
	MenuFixture f;
	Click(f.menu, ButtonCentre(3));
	Click(f.menu, TabCentre(2));

	// The creature's name box takes the focus and the keyboard
	Click(f.menu, {400, 390});
	f.menu.TextInput(u"AB");
	EXPECT_EQ(f.menu.GetSettings().creatureName, u"aaaAB");
	f.menu.KeyDown(SDLK_BACKSPACE);
	EXPECT_EQ(f.menu.GetSettings().creatureName, u"aaaA");

	// Holding the button down on the symbol brings up the others to pick from, once they are all the way up
	const SymbolPicture picture({.min = {368, 295}, .max = {432, 359}}, 15);
	const auto three = picture.GetRingRect(3).Centre();
	f.menu.MouseDown({400, 327});
	f.menu.MouseMove(three);
	f.menu.Update(0.1f);
	f.menu.MouseUp(three);
	EXPECT_EQ(f.menu.GetSettings().symbol, 15);
	f.menu.MouseDown({400, 327});
	f.menu.MouseMove(three);
	f.menu.Update(0.6f);
	f.menu.MouseUp(three);
	EXPECT_EQ(f.menu.GetSettings().symbol, 3);
}

TEST(SymbolPicture, RingComesUpAndGoes)
{
	SymbolPicture picture({.min = {368, 295}, .max = {432, 359}}, 15);
	EXPECT_FLOAT_EQ(picture.GetRingOpening(), 0.0f);
	// SetupPicture: up over half a second, down over a second
	picture.MouseDown({400, 327});
	picture.Update(0.25f);
	EXPECT_GT(picture.GetRingOpening(), 0.0f);
	EXPECT_LT(picture.GetRingOpening(), 1.0f);
	picture.Update(0.3f);
	EXPECT_FLOAT_EQ(picture.GetRingOpening(), 1.0f);
	picture.Release();
	EXPECT_FALSE(picture.IsRingOpen());
	picture.Update(0.5f);
	EXPECT_GT(picture.GetRingOpening(), 0.0f);
	picture.Update(0.6f);
	EXPECT_FLOAT_EQ(picture.GetRingOpening(), 0.0f);

	// Six symbols on the inner ring 56 out from the centre, the first at the top, ten on the outer 104 out, each 23 either way
	// of its centre (24 at an opacity of 255 over 256)
	EXPECT_EQ(picture.GetRingRect(0).Centre(), glm::ivec2(400, 327 - 56));
	EXPECT_EQ(picture.GetRingRect(0).Width(), 46);
	EXPECT_EQ(picture.GetRingRect(3).Centre(), glm::ivec2(400, 327 + 56));
	EXPECT_EQ(picture.GetRingRect(6).Centre(), glm::ivec2(400, 327 - 104));
}

TEST(GameMenu, ListsTheControls)
{
	const MenuFixture f;
	const auto& items = f.menu.GetControls().GetItems();
	ASSERT_EQ(items.size(), 33);
	// ControlMap::GetText: the action, its key and its mouse button after a gap for the mouse's picture
	EXPECT_EQ(items[0].text, u"Help : F1 ");
	EXPECT_FALSE(items[0].mouseCell.has_value());
	EXPECT_EQ(items[1].text, u"Move :       (LMB)");
	EXPECT_EQ(items[1].mouseCell, 9);
	EXPECT_TRUE(items[1].mouseMirrored);
	EXPECT_EQ(items[6].text, u"Zoom On : CTRL ");
	EXPECT_EQ(items[6].mouseCell, 10);
	EXPECT_EQ(items[29].text, u" : CTRL + S ");
}
