/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GameMenu.h"

#include <algorithm>
#include <string_view>

#include <SDL_keycode.h>

#include "GameFont.h"
#include "TextDatabase.h"

using namespace openblack::gui;

namespace
{
using Look = BigButton::Look;
using LabelSide = BigButton::LabelSide;
using Layout = StaticText::Layout;
constexpr auto k_Big = DialogPainter::k_BigTextSize;
constexpr auto k_Mid = DialogPainter::k_MidTextSize;

// DialogBoxBase and SetupBox: fading in when shown and out when hidden
constexpr float k_FadeInSeconds = 0.5f;
constexpr float k_FadeOutSeconds = 0.2f;

// MainMenu::Init and InitControls
constexpr int k_ButtonLeft = 180;
constexpr int k_ButtonWidth = 440;
constexpr int k_ButtonHeight = 70;
constexpr int k_FirstButtonTop = 145;
constexpr int k_ButtonSpacing = 80;

// The options' pages: Back at the bottom left, 40 pixel arrows
constexpr glm::ivec2 k_BackPosition {30, 530};
constexpr int k_BackSize = 40;
// The selectors: a 200 by 30 button between two 32 pixel arrows
constexpr int k_SelectorLeft = 300;
constexpr int k_SelectorWidth = 200;
constexpr int k_SelectorHeight = 30;
constexpr int k_SelectorLeftArrow = 260;
constexpr int k_SelectorRightArrow = 508;
constexpr int k_ArrowSize = 32;
constexpr int k_CheckBoxTop = 450;

constexpr std::array k_DetailTexts = {"HELP_TEXT_DIALOG_DETAIL_00", "HELP_TEXT_DIALOG_DETAIL_01", "HELP_TEXT_DIALOG_DETAIL_02",
                                      "HELP_TEXT_DIALOG_DETAIL_03", "HELP_TEXT_DIALOG_DETAIL_04"};
constexpr std::array k_HelpTexts = {"HELP_TEXT_DIALOG_NOHELP", "HELP_TEXT_DIALOG_LOWHELP", "HELP_TEXT_DIALOG_MIDDLEHELP",
                                    "HELP_TEXT_DIALOG_HIGHHELP", "HELP_TEXT_DIALOG_ALLHELP"};
constexpr std::array k_StoryTexts = {"HELP_TEXT_DIALOG_NOTEXT", "HELP_TEXT_DIALOG_STORYTEXT", "HELP_TEXT_DIALOG_ALLTEXT"};
constexpr std::array k_ToolTipTexts = {"HELP_TEXT_DIALOG_NOTOOLTIPS", "HELP_TEXT_DIALOG_MINIMUMTOOLTIPS",
                                       "HELP_TEXT_DIALOG_INTELLIGENTNOTOOLTIPS", "HELP_TEXT_DIALOG_ALLTOOLTIPS"};

/// ControlMap's mouse buttons
enum class Mouse
{
	None,
	Left,
	Middle,
	WheelUp,
	WheelDown,
	Right,
	/// Both buttons, for the actions that zoom while a key is held
	Both,
};

/// ControlMap::LoadDefaults for a mouse with a wheel: the actions in the order the Controls page lists them, their
/// names, and the DirectInput key and mouse button bound to them
struct Binding
{
	std::string_view name;
	uint8_t key;
	uint8_t modifier;
	Mouse mouse;
};
constexpr std::array k_Bindings = {
    Binding {.name = "HELP_TEXT_ACTIONS_HELP", .key = 0x3B, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_SELECT", .key = 0, .modifier = 0, .mouse = Mouse::Left},
    Binding {.name = "HELP_TEXT_ACTIONS_APPLY", .key = 0, .modifier = 0, .mouse = Mouse::Right},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMOUT", .key = 0, .modifier = 0, .mouse = Mouse::WheelDown},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMIN", .key = 0, .modifier = 0, .mouse = Mouse::WheelUp},
    Binding {.name = "HELP_TEXT_ACTIONS_TALK", .key = 0x14, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMON", .key = 0x9D, .modifier = 0, .mouse = Mouse::Both},
    Binding {.name = "HELP_TEXT_ACTIONS_MOVELEFT", .key = 0xCB, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_MOVERIGHT", .key = 0xCD, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_MOVEFORWARD", .key = 0xC8, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_MOVEBACKWARD", .key = 0xD0, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_TILTUP", .key = 0x1E, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_TILTDOWN", .key = 0x10, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ROTATELEFT", .key = 0x2C, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ROTATERIGHT", .key = 0x16, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_TILTROTATEON", .key = 0x36, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_TILTROTATEAROUNDMOUSE", .key = 0, .modifier = 0, .mouse = Mouse::Middle},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMTO_CITADEL", .key = 0x39, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMTO_CREATURE", .key = 0x2E, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMTO_REALM", .key = 0x3D, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMTO_INSIDECITADEL", .key = 0x3E, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMTO_CREATUREROOM", .key = 0x3F, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMTO_CHALLENGEROOM", .key = 0x40, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMTO_SAVEGAMEROOM", .key = 0x41, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMTO_OPTIONSROOM", .key = 0x42, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_ZOOMTO_MULTIPLAYERROOM", .key = 0x43, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_LEASH", .key = 0x26, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_SHOWVILLAGERNAME", .key = 0x31, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_SHOWVILLAGERDETAILS", .key = 0x1F, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_TOOLTIP_155", .key = 0x1F, .modifier = 0x1D, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_TOOLTIP_154", .key = 0x26, .modifier = 0x1D, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_PREVIOUS_LEASH", .key = 0x2F, .modifier = 0, .mouse = Mouse::None},
    Binding {.name = "HELP_TEXT_ACTIONS_NEXT_LEASH", .key = 0x30, .modifier = 0, .mouse = Mouse::None},
};

/// GetKeyNameText's names, which the game shows, of the DirectInput keys bound by default
std::u16string KeyName(uint8_t key)
{
	switch (key)
	{
	case 0x0B:
		return u"0";
	case 0x10:
		return u"Q";
	case 0x14:
		return u"T";
	case 0x16:
		return u"U";
	case 0x1D:
	case 0x9D:
		return u"CTRL";
	case 0x1E:
		return u"A";
	case 0x1F:
		return u"S";
	case 0x26:
		return u"L";
	case 0x2C:
		return u"Z";
	case 0x2E:
		return u"C";
	case 0x2F:
		return u"V";
	case 0x30:
		return u"B";
	case 0x31:
		return u"N";
	case 0x36:
		return u"RIGHT SHIFT";
	case 0x39:
		return u"SPACE";
	case 0xC8:
		return u"UP";
	case 0xCB:
		return u"LEFT";
	case 0xCD:
		return u"RIGHT";
	case 0xD0:
		return u"DOWN";
	default:
		if (key >= 0x3B && key <= 0x44)
		{
			return u"F" + ToUtf16(std::to_string(key - 0x3B + 1));
		}
		return u"?";
	}
}

/// fn_00447450's mice for a wheel mouse: the third row of data/textures/mousehelp.raw, the left button the mirrored
/// right one. The wheel turns over the fourth row.
constexpr int k_MouseRow = 2;
std::optional<std::pair<int, bool>> MouseCell(Mouse mouse)
{
	switch (mouse)
	{
	case Mouse::Left:
		return std::pair((k_MouseRow * 4) + 1, true);
	case Mouse::Right:
		return std::pair((k_MouseRow * 4) + 1, false);
	case Mouse::Both:
		return std::pair((k_MouseRow * 4) + 2, false);
	case Mouse::Middle:
		return std::pair((k_MouseRow * 4) + 3, false);
	case Mouse::WheelUp:
	case Mouse::WheelDown:
		return std::pair(11, false);
	default:
		return std::nullopt;
	}
}
} // namespace

GameMenu::GameMenu(const TextDatabase& texts, const GameFont& font, std::u16string_view playerName, MenuSettings settings)
    : _font(font)
    , _playerName(playerName)
    , _yes(texts.Get("HELP_TEXT_REQUESTER_BOXES_01"))
    , _no(texts.Get("HELP_TEXT_REQUESTER_BOXES_02"))
    , _quitQuestion(texts.Get("HELP_TEXT_DIALOG_QUIT_QUESTION"))
    , _quitGameQuestion(texts.Get("HELP_TEXT_DIALOG_AREYOUSUREQUIT"))
    , _settings(std::move(settings))
{
	const auto read = [&texts](auto& labels, const auto& names) {
		for (size_t i = 0; i < labels.size(); ++i)
		{
			labels.at(i) = texts.Get(names.at(i));
		}
	};
	read(_detailLabels, k_DetailTexts);
	read(_helpLabels, k_HelpTexts);
	read(_storyLabels, k_StoryTexts);
	read(_toolTipLabels, k_ToolTipTexts);
	_settings.detail = std::clamp(_settings.detail, 0, static_cast<int>(_detailLabels.size()) - 1);
	_settings.helpLevel = std::clamp(_settings.helpLevel, 0, static_cast<int>(_helpLabels.size()) - 1);
	_settings.storyText = std::clamp(_settings.storyText, 0, static_cast<int>(_storyLabels.size()) - 1);
	_settings.toolTips = std::clamp(_settings.toolTips, 0, static_cast<int>(_toolTipLabels.size()) - 1);
	if (_settings.creatureName.empty())
	{
		_settings.creatureName = _playerName;
	}

	BuildMain(texts);
	BuildOptions(texts);
	BuildPlayers(texts);
	BuildAdvanced(texts);
	BuildControls(texts);
}

GameMenu::~GameMenu() = default;

DialogRect GameMenu::GetButtonRect(size_t index)
{
	const auto top = k_FirstButtonTop + (k_ButtonSpacing * static_cast<int>(index));
	return {.min = {k_ButtonLeft, top}, .max = {k_ButtonLeft + k_ButtonWidth, top + k_ButtonHeight}};
}

std::vector<Dialog::Tab> GameMenu::OptionsTabs(const TextDatabase& texts, Page selected)
{
	// AddOptionsTabs: Main Menu, Options, Players, Advanced and Controls
	(void)selected;
	return {
	    {.label = std::u16string(texts.Get("HELP_TEXT_DIALOG_ADDITION_88")), .onSelect = [this] { ShowPage(Page::Main); }},
	    {.label = std::u16string(texts.Get("HELP_TEXT_DIALOG_OPTIONS")), .onSelect = [this] { ShowPage(Page::Options); }},
	    {.label = std::u16string(texts.Get("HELP_TEXT_DIALOG_PLAYERLIST")), .onSelect = [this] { ShowPage(Page::Players); }},
	    {.label = std::u16string(texts.Get("HELP_TEXT_FRONT_END_06")), .onSelect = [this] { ShowPage(Page::Advanced); }},
	    {.label = std::u16string(texts.Get("HELP_TEXT_DIALOG_CONTROLOPTIONS")),
	     .onSelect = [this] { ShowPage(Page::Controls); }},
	};
}

void GameMenu::BuildMain(const TextDatabase& texts)
{
	// AddMainMenuTabs: the third tab, Multiplayer, only has a label in a multiplayer game
	std::vector<Dialog::Tab> tabs {
	    {.label = std::u16string(texts.Get("HELP_TEXT_DIALOG_ADDITION_88")), .onSelect = {}},
	    // TODO(raffclar): the statistics page
	    {.label = std::u16string(texts.Get("HELP_TEXT_PAUSE_STATS_151")), .onSelect = [this] { Settle(Action::Statistics); }},
	};
	auto& page = *(_pages[static_cast<size_t>(Page::Main)] = std::make_unique<Dialog>(std::move(tabs), 0));

	_welcome = texts.Get("HELP_TEXT_DIALOG_WELCOME");
	if (const auto found = _welcome.find(u"%s"); found != std::u16string::npos)
	{
		_welcome.replace(found, 2, _playerName);
	}
	page.Add<StaticText>(DialogRect {.min = {50, 65}, .max = {750, 155}}, _welcome, Layout::Wrapped);

	// "Leave Skirmish Game" (HELP_TEXT_DIALOG_ADDITION_170) during a skirmish and "Exit Online Game."
	// (HELP_TEXT_FRONT_END_03) during a multiplayer game
	const std::array<std::pair<const char*, std::function<void()>>, k_ButtonCount> buttons = {{
	    {"HELP_TEXT_DIALOG_CONTINUEGAME", [this] { Settle(Action::Continue); }},
	    {"HELP_TEXT_DIALOG_ADDITION_169", [this] { Settle(Action::StartSkirmish); }},
	    {"HELP_TEXT_DIALOG_JOINGONLINE", [this] { Settle(Action::JoinOnline); }},
	    {"HELP_TEXT_DIALOG_OPTIONS", [this] { ShowPage(Page::Options); }},
	    {"HELP_TEXT_DIALOG_QUIT", [this] { Ask(_quitQuestion); }},
	}};
	for (size_t i = 0; i < k_ButtonCount; ++i)
	{
		_buttonLabels.at(i) = texts.Get(buttons.at(i).first);
		page.Add<Button>(GetButtonRect(i), _buttonLabels.at(i)).onClick = buttons.at(i).second;
	}
}

void GameMenu::BuildOptions(const TextDatabase& texts)
{
	// MiniDialogBoxOptions::Init
	auto& page = *(_pages[static_cast<size_t>(Page::Options)] =
	                   std::make_unique<Dialog>(OptionsTabs(texts, Page::Options), static_cast<size_t>(Page::Options)));
	page.Add<StaticText>(DialogRect {.min = {50, 60}, .max = {750, 100}},
	                     std::u16string(texts.Get("HELP_TEXT_DIALOG_SOUNDANDVIDEOCONTROL")), Layout::Wrapped);

	auto& sfx = page.Add<Slider>(DialogRect {.min = {250, 140}, .max = {550, 170}},
	                             std::u16string(texts.Get("HELP_TEXT_DIALOG_MASTERVOLUME")), _settings.sfxVolume);
	sfx.onChange = [this](float value) {
		_settings.sfxVolume = value;
		SettingsChanged();
	};
	auto& music = page.Add<Slider>(DialogRect {.min = {250, 200}, .max = {550, 230}},
	                               std::u16string(texts.Get("HELP_TEXT_DIALOG_MUSICVOLUME")), _settings.musicVolume);
	music.onChange = [this](float value) {
		_settings.musicVolume = value;
		SettingsChanged();
	};

	page.Add<StaticText>(DialogRect {.min = {50, 280}, .max = {750, 320}},
	                     std::u16string(texts.Get("HELP_TEXT_DIALOG_VIDEOCONTROL")), Layout::Centre);
	// The detail: the button and the right arrow step it on, the left arrow back
	// TODO(raffclar): the detail doesn't change anything yet. The original says a changed detail only takes effect the next
	// time the game starts (HELP_TEXT_DIALOG_VIDEOCHANGE).
	auto& detail = page.Add<Button>(
	    DialogRect {.min = {k_SelectorLeft, 350}, .max = {k_SelectorLeft + k_SelectorWidth, 350 + k_SelectorHeight}},
	    _detailLabels.at(static_cast<size_t>(_settings.detail)), k_Mid);
	const auto stepDetail = [this, &detail](int step) {
		const auto count = static_cast<int>(_detailLabels.size());
		_settings.detail = (_settings.detail + step + count) % count;
		detail.label = _detailLabels.at(static_cast<size_t>(_settings.detail));
		SettingsChanged();
	};
	detail.onClick = [stepDetail] { stepDetail(1); };
	page.Add<BigButton>(_font, glm::ivec2(k_SelectorLeftArrow, 349), k_ArrowSize, u"", LabelSide::Right, Look::LeftArrow)
	    .onClick = [stepDetail] { stepDetail(-1); };
	page.Add<BigButton>(_font, glm::ivec2(k_SelectorRightArrow, 349), k_ArrowSize, u"", LabelSide::Right, Look::RightArrow)
	    .onClick = [stepDetail] { stepDetail(1); };

	auto& autoSave = page.Add<CheckBox>(_font, glm::ivec2(285, k_CheckBoxTop),
	                                    std::u16string(texts.Get("HELP_TEXT_DIALOG_AUTOSAVE")), _settings.autoSave);
	autoSave.onChange = [this](bool checked) {
		_settings.autoSave = checked;
		SettingsChanged();
	};
	auto& pushScrolling =
	    page.Add<CheckBox>(_font, glm::ivec2(485, k_CheckBoxTop), std::u16string(texts.Get("HELP_TEXT_DIALOG_ADDITION_116")),
	                       _settings.pushScrolling);
	pushScrolling.onChange = [this](bool checked) {
		_settings.pushScrolling = checked;
		SettingsChanged();
	};

	page.Add<BigButton>(_font, k_BackPosition, k_BackSize, std::u16string(texts.Get("HELP_TEXT_DIALOG_BACK")), LabelSide::Right,
	                    Look::LeftArrow)
	    .onClick = [this] { ShowPage(Page::Main); };
	page.Add<BigButton>(_font, glm::ivec2(730, 530), k_BackSize, std::u16string(texts.Get("HELP_TEXT_DIALOG_QUIT")),
	                    LabelSide::Left, Look::RightArrow)
	    .onClick = [this] { Ask(_quitGameQuestion); };
}

void GameMenu::BuildPlayers(const TextDatabase& texts)
{
	// ProfileEditor::Init
	auto& page = *(_pages[static_cast<size_t>(Page::Players)] =
	                   std::make_unique<Dialog>(OptionsTabs(texts, Page::Players), static_cast<size_t>(Page::Players)));
	page.Add<StaticText>(DialogRect {.min = {100, 60}, .max = {700, 120}},
	                     std::u16string(texts.Get("HELP_TEXT_DIALOG_SELECT_A_PLAYER")), Layout::Wrapped);

	// openblack has no profiles of its own: the player is the only one
	auto& players = page.Add<List>(_font, DialogRect {.min = {200, 120}, .max = {600, 280}}, k_Mid, true, false, true);
	players.SetItems({{.text = _playerName, .mouseCell = std::nullopt, .mouseMirrored = false}});
	players.SetSelection(0);

	page.Add<StaticText>(DialogRect {.min = {170, 295}, .max = {360, 359}},
	                     std::u16string(texts.Get("HELP_TEXT_DIALOG_PLAYER")), Layout::Right);
	page.Add<StaticText>(DialogRect {.min = {440, 295}, .max = {630, 359}},
	                     std::u16string(texts.Get("HELP_TEXT_DIALOG_SYMBOL")), Layout::Left);
	auto& symbol = page.Add<SymbolPicture>(DialogRect {.min = {368, 295}, .max = {432, 359}}, _settings.symbol);
	symbol.onChange = [this](int value) {
		_settings.symbol = value;
		SettingsChanged();
	};

	page.Add<StaticText>(DialogRect {.min = {0, 370}, .max = {190, 410}},
	                     std::u16string(texts.Get("HELP_TEXT_DIALOG_CREATURENAME")), Layout::Right, k_Mid);
	// SetupEdit takes 29 characters
	auto& name = page.Add<EditBox>(DialogRect {.min = {200, 370}, .max = {600, 410}}, _settings.creatureName, 29);
	name.onChange = [this](const std::u16string& text) {
		_settings.creatureName = text;
		SettingsChanged();
	};

	// TODO(raffclar): profiles, tattoos and new games
	const std::array<std::tuple<int, const char*, Action>, 4> buttons = {{
	    {209, "HELP_TEXT_DIALOG_CREATENEWPLAYER", Action::CreatePlayer},
	    {324, "HELP_TEXT_DIALOG_DELETEPLAYER", Action::DeletePlayer},
	    {444, "HELP_TEXT_DIALOG_EDITTATOO", Action::EditTattoo},
	    {559, "HELP_TEXT_DIALOG_STARTGAME", Action::StartNewGame},
	}};
	for (const auto& [x, label, action] : buttons)
	{
		page.Add<BigButton>(_font, glm::ivec2(x, k_CheckBoxTop), 25, std::u16string(texts.Get(label)), LabelSide::Below,
		                    Look::Square)
		    .onClick = [this, action] { Settle(action); };
	}

	page.Add<BigButton>(_font, k_BackPosition, k_BackSize, std::u16string(texts.Get("HELP_TEXT_DIALOG_BACK")), LabelSide::Right,
	                    Look::LeftArrow)
	    .onClick = [this] { ShowPage(Page::Main); };
}

void GameMenu::BuildAdvanced(const TextDatabase& texts)
{
	// DialogBoxOptions::Init
	auto& page = *(_pages[static_cast<size_t>(Page::Advanced)] =
	                   std::make_unique<Dialog>(OptionsTabs(texts, Page::Advanced), static_cast<size_t>(Page::Advanced)));
	page.Add<StaticText>(DialogRect {.min = {50, 60}, .max = {750, 100}},
	                     std::u16string(texts.Get("HELP_TEXT_DIALOG_HELPCONTROLS")), Layout::Wrapped);

	// Three selectors, the button and the right arrow stepping them on and the left arrow back
	// TODO(raffclar): help, story text and tooltips don't change anything yet
	const auto addSelector = [this, &page](int top, int& value, auto& labels) {
		auto& button = page.Add<Button>(
		    DialogRect {.min = {k_SelectorLeft, top}, .max = {k_SelectorLeft + k_SelectorWidth, top + k_SelectorHeight}},
		    labels.at(static_cast<size_t>(value)), k_Mid);
		const auto step = [this, &button, &value, &labels](int by) {
			const auto count = static_cast<int>(labels.size());
			value = (value + by + count) % count;
			button.label = labels.at(static_cast<size_t>(value));
			SettingsChanged();
		};
		button.onClick = [step] { step(1); };
		page.Add<BigButton>(_font, glm::ivec2(k_SelectorLeftArrow, top - 1), k_ArrowSize, u"", LabelSide::Right,
		                    Look::LeftArrow)
		    .onClick = [step] { step(-1); };
		page.Add<BigButton>(_font, glm::ivec2(k_SelectorRightArrow, top - 1), k_ArrowSize, u"", LabelSide::Right,
		                    Look::RightArrow)
		    .onClick = [step] { step(1); };
	};
	addSelector(150, _settings.helpLevel, _helpLabels);
	addSelector(215, _settings.storyText, _storyLabels);
	addSelector(280, _settings.toolTips, _toolTipLabels);

	const std::array<std::tuple<int, const char*, bool*>, 3> boxes = {{
	    {385, "HELP_TEXT_DIALOG_CREATUREHELP", &_settings.creatureHelp},
	    {185, "HELP_TEXT_DIALOG_LEFTHANDED", &_settings.leftHandedHand},
	    {585, "HELP_TEXT_DIALOG_ADDITION_126", &_settings.textFromBottom},
	}};
	for (const auto& [x, label, setting] : boxes)
	{
		auto& box = page.Add<CheckBox>(_font, glm::ivec2(x, k_CheckBoxTop), std::u16string(texts.Get(label)), *setting);
		box.onChange = [this, setting](bool checked) {
			*setting = checked;
			SettingsChanged();
		};
	}

	page.Add<BigButton>(_font, k_BackPosition, k_BackSize, std::u16string(texts.Get("HELP_TEXT_DIALOG_BACK")), LabelSide::Right,
	                    Look::LeftArrow)
	    .onClick = [this] { ShowPage(Page::Main); };
}

void GameMenu::BuildControls(const TextDatabase& texts)
{
	// DialogBoxKeyBinding::Init
	auto& page = *(_pages[static_cast<size_t>(Page::Controls)] =
	                   std::make_unique<Dialog>(OptionsTabs(texts, Page::Controls), static_cast<size_t>(Page::Controls)));
	page.Add<StaticText>(DialogRect {.min = {100, 60}, .max = {700, 160}},
	                     std::u16string(texts.Get("HELP_TEXT_DIALOG_CONTROLTITLE")), Layout::Wrapped);

	// ControlMap::GetText: "<action> : <key> <mouse>", the mouse's name after a gap of five spaces for its picture
	std::vector<List::Item> items;
	for (size_t i = 0; i < k_Bindings.size(); ++i)
	{
		const auto& binding = k_Bindings.at(i);
		std::u16string text(texts.Get(binding.name));
		text += u" : ";
		if (binding.key != 0)
		{
			text += binding.modifier != 0 ? KeyName(binding.modifier) + u" + " + KeyName(binding.key) : KeyName(binding.key);
		}
		text += u" ";
		std::string_view mouseName;
		switch (binding.mouse)
		{
		case Mouse::Left:
			mouseName = "HELP_TEXT_ACTIONS_LEFTMOUSEBUTTON";
			break;
		case Mouse::Right:
			mouseName = "HELP_TEXT_ACTIONS_RIGHTMOUSEBUTTON";
			break;
		case Mouse::Middle:
			mouseName = "HELP_TEXT_ACTIONS_MIDDLEMOUSEBUTTON";
			break;
		case Mouse::WheelUp:
			mouseName = "HELP_TEXT_ACTIONS_WHEEL_UP";
			break;
		case Mouse::WheelDown:
			mouseName = "HELP_TEXT_ACTIONS_WHEEL_DOWN";
			break;
		default:
			break;
		}
		if (!mouseName.empty())
		{
			text += u"     (" + std::u16string(texts.Get(mouseName)) + u")";
		}
		const auto cell = MouseCell(binding.mouse);
		items.push_back({.text = std::move(text),
		                 .mouseCell = cell ? std::optional(cell->first) : std::nullopt,
		                 .mouseMirrored = cell && cell->second});
		if (binding.mouse == Mouse::WheelUp || binding.mouse == Mouse::WheelDown)
		{
			_wheelMice.emplace_back(i, binding.mouse == Mouse::WheelUp);
		}
	}
	// Halfway between the big and the mid text size
	auto& list =
	    page.Add<List>(_font, DialogRect {.min = {100, 120}, .max = {700, 440}}, (k_Big + k_Mid) / 2, true, true, false);
	list.SetItems(std::move(items));
	// TODO(raffclar): redefining an action, by double clicking it
	_controls = &list;

	// TODO(raffclar): the bindings can't change yet, so they are the defaults already
	page.Add<Button>(DialogRect {.min = {260, 530}, .max = {540, 570}},
	                 std::u16string(texts.Get("HELP_TEXT_DIALOG_LOADDEFAULTS")), k_Mid);

	page.Add<BigButton>(_font, k_BackPosition, k_BackSize, std::u16string(texts.Get("HELP_TEXT_DIALOG_BACK")), LabelSide::Right,
	                    Look::LeftArrow)
	    .onClick = [this] { ShowPage(Page::Main); };
}

void GameMenu::Ask(std::u16string text)
{
	// SetupBox::MessageBoxA with a Yes and a No: the question in the middle of a 500 by 200 box
	auto answers = std::make_unique<Dialog>();
	answers->Add<StaticText>(DialogRect {.min = {200, 220}, .max = {600, 350}}, text, Layout::Wrapped);
	answers->Add<BigButton>(_font, glm::ivec2(200, 338), k_ArrowSize, _yes, LabelSide::Right, Look::LeftArrow).onClick =
	    [this] { Settle(Action::Quit); };
	answers->Add<BigButton>(_font, glm::ivec2(568, 338), k_ArrowSize, _no, LabelSide::Left, Look::RightArrow).onClick = [this] {
		Settle(Action::None);
	};
	_question = Question {.text = std::move(text), .answers = std::move(answers), .fade = Zoomer()};
	_question->fade.SetDestination(1.0f, k_FadeInSeconds);
	CurrentDialog().Reset();
}

void GameMenu::Settle(Action action)
{
	_action = action;
	// The question goes once the click on its answer is over
	_answered = _question.has_value();
}

void GameMenu::ShowPage(Page page)
{
	CurrentDialog().Reset();
	_page = page;
	CurrentDialog().Reset();
}

void GameMenu::Open()
{
	if (_open)
	{
		return;
	}
	_open = true;
	_question.reset();
	_page = Page::Main;
	CurrentDialog().Reset();
	_fade.SetDestination(1.0f, k_FadeInSeconds);
}

void GameMenu::Close()
{
	_open = false;
	_question.reset();
	CurrentDialog().Reset();
	_fade.SetDestination(0.0f, k_FadeOutSeconds);
}

void GameMenu::Update(float deltaSeconds)
{
	_fade.Update(deltaSeconds);
	if (_question)
	{
		_question->fade.Update(deltaSeconds);
	}
	if (_open)
	{
		CurrentDialog().Update(_question ? 0.0f : deltaSeconds);
		if (_question)
		{
			_question->answers->Update(deltaSeconds);
		}
	}

	// fn_00447450: the wheel turns over a picture every tenth of a second, resting one in five
	_wheelTime += deltaSeconds;
	const auto frame = static_cast<int>(_wheelTime * 10.0f) % 5;
	for (const auto& [index, up] : _wheelMice)
	{
		// The wheel turns the four frames one way scrolling up and the other scrolling down, then rests
		auto cell = 11;
		if (frame < 4)
		{
			cell = up ? 12 + frame : 15 - frame;
		}
		_controls->SetMouseCell(index, cell);
	}
}

void GameMenu::MouseMove(glm::ivec2 point)
{
	if (_open)
	{
		(_question ? *_question->answers : CurrentDialog()).MouseMove(point);
	}
}

void GameMenu::MouseDown(glm::ivec2 point)
{
	if (_open)
	{
		(_question ? *_question->answers : CurrentDialog()).MouseDown(point);
	}
}

GameMenu::Action GameMenu::MouseUp(glm::ivec2 point)
{
	if (!_open)
	{
		return Action::None;
	}
	if ((_question ? *_question->answers : CurrentDialog()).MouseUp(point))
	{
		_clicked = true;
	}
	if (std::exchange(_answered, false))
	{
		_question.reset();
	}
	return std::exchange(_action, Action::None);
}

void GameMenu::Wheel(glm::ivec2 point, int steps)
{
	if (_open && !_question)
	{
		CurrentDialog().Wheel(point, steps);
	}
}

void GameMenu::TextInput(std::u16string_view text)
{
	if (_open && !_question)
	{
		CurrentDialog().TextInput(text);
	}
}

void GameMenu::KeyDown(int key)
{
	if (_open && !_question)
	{
		CurrentDialog().KeyDown(key);
	}
}

GameMenu::Action GameMenu::Escape()
{
	if (!_open)
	{
		return Action::None;
	}
	if (_question)
	{
		_question.reset();
		return Action::None;
	}
	if (_page != Page::Main)
	{
		ShowPage(Page::Main);
		return Action::None;
	}
	return Action::Continue;
}

void GameMenu::Draw(const DialogPainter& painter) const
{
	const auto fade = std::clamp(_fade.GetValue(), 0.0f, 1.0f);
	if (fade <= 0.0f)
	{
		return;
	}
	painter.SetAlpha(fade);
	CurrentDialog().Draw(painter, _open && !_question);
	if (_question)
	{
		// SetupBox::MessageBoxA: an opaque box over the menu, fading in
		painter.SetAlpha(fade * std::clamp(_question->fade.GetValue(), 0.0f, 1.0f));
		painter.DrawBackground(GetQuestionRect(), glm::vec3(1.0f), true, DialogPainter::All);
		_question->answers->Draw(painter, _open);
	}
	painter.SetAlpha(1.0f);
}
