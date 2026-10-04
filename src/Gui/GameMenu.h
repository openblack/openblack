/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "Common/Zoomer.h"
#include "Dialog.h"

namespace openblack::gui
{

class GameFont;
class TextDatabase;

/// What the player sets in the game's menu
struct MenuSettings
{
	/// 0 to 1
	float sfxVolume {1.0f};
	float musicVolume {1.0f};
	/// HELP_TEXT_DIALOG_DETAIL_00 to _04, Minimum to Maximum Detail
	int detail {4};
	bool autoSave {true};
	bool pushScrolling {false};
	/// HELP_TEXT_DIALOG_NOHELP to ALLHELP
	int helpLevel {3};
	/// HELP_TEXT_DIALOG_NOTEXT to ALLTEXT
	int storyText {1};
	/// HELP_TEXT_DIALOG_NOTOOLTIPS to ALLTOOLTIPS
	int toolTips {2};
	bool leftHandedHand {false};
	bool creatureHelp {true};
	bool textFromBottom {false};
	std::u16string creatureName;
	/// One of the 16 player symbols
	int symbol {15};
};

/// The menu Escape brings up during a game, and the options behind it.
///
/// The first page, MainMenu, greets the player above five buttons: Continue Game, Start Skirmish Game, Join Online Game,
/// Options and Quit Black & White, with a Main Menu and a Statistics tab. Options opens the options' pages, each a tab
/// of the same box (AddOptionsTabs): Options for sound, detail, autosave and push scrolling (MiniDialogBoxOptions),
/// Players for the player's profile (ProfileEditor), Advanced for help, the hand and text (DialogBoxOptions) and
/// Controls for the keys and buttons of the game's actions (DialogBoxKeyBinding). Their Main Menu tab, or Back, goes
/// back to the first page.
///
/// The menu fades in over half a second and out over a fifth. Quitting first asks whether the player is sure, in a
/// smaller opaque box over the menu with a Yes and a No arrow (SetupBox::MessageBoxA).
class GameMenu
{
public:
	enum class Action
	{
		None,
		Continue,
		StartSkirmish,
		JoinOnline,
		Statistics,
		CreatePlayer,
		DeletePlayer,
		EditTattoo,
		StartNewGame,
		RedefineControl,
		Quit,
	};

	enum class Page
	{
		Main,
		Options,
		Players,
		Advanced,
		Controls,
		_Count
	};

	/// The texts are read as the menu is made, the font measures them. playerName fills in the welcome and is the
	/// player's profile.
	GameMenu(const TextDatabase& texts, const GameFont& font, std::u16string_view playerName, MenuSettings settings);
	~GameMenu();

	/// Starts fading in on the first page, if it isn't open already
	void Open();
	/// Starts fading out, closing the question about quitting too
	void Close();
	/// Open and taking the player's input, not fading out
	[[nodiscard]] bool IsOpen() const noexcept { return _open; }
	/// On the screen, fading out included
	[[nodiscard]] bool IsVisible() const noexcept { return _open || _fade.GetValue() > 0.0f; }
	[[nodiscard]] bool IsAskingToQuit() const noexcept { return _question.has_value(); }
	[[nodiscard]] Page GetPage() const noexcept { return _page; }
	void ShowPage(Page page);

	void Update(float deltaSeconds);

	// Input at points of the dialog space
	void MouseMove(glm::ivec2 point);
	void MouseDown(glm::ivec2 point);
	/// What the player chose by letting go of the button, Quit only once they have said yes
	Action MouseUp(glm::ivec2 point);
	void Wheel(glm::ivec2 point, int steps);
	void TextInput(std::u16string_view text);
	void KeyDown(int key);
	/// Escape backs out of the question, then out of the options, then continues the game
	Action Escape();

	/// Whether a control was clicked since last asked, which SetupBox answers with the menu button sound
	bool TakeClicked() noexcept { return std::exchange(_clicked, false); }
	/// Whether the settings changed since last asked
	bool TakeSettingsChanged() noexcept { return std::exchange(_settingsChanged, false); }
	[[nodiscard]] const MenuSettings& GetSettings() const noexcept { return _settings; }

	void Draw(const DialogPainter& painter) const;

	// The layout of the first page, for hit tests
	static constexpr size_t k_ButtonCount = 5;
	[[nodiscard]] static DialogRect GetButtonRect(size_t index);
	[[nodiscard]] static constexpr DialogRect GetQuestionRect() { return {.min = {150, 200}, .max = {650, 400}}; }
	[[nodiscard]] const std::u16string& GetWelcome() const noexcept { return _welcome; }
	[[nodiscard]] const std::u16string& GetButtonLabel(size_t index) const { return _buttonLabels.at(index); }
	/// The lines of the Controls page
	[[nodiscard]] const List& GetControls() const noexcept { return *_controls; }

private:
	struct Question
	{
		std::u16string text;
		std::unique_ptr<Dialog> answers;
		Zoomer fade;
	};

	void BuildMain(const TextDatabase& texts);
	void BuildOptions(const TextDatabase& texts);
	void BuildPlayers(const TextDatabase& texts);
	void BuildAdvanced(const TextDatabase& texts);
	void BuildControls(const TextDatabase& texts);
	/// The tabs of the options' pages, with selected open
	std::vector<Dialog::Tab> OptionsTabs(const TextDatabase& texts, Page selected);
	void Ask(std::u16string text);
	void Settle(Action action);
	void SettingsChanged() { _settingsChanged = true; }
	[[nodiscard]] Dialog& CurrentDialog() const { return *_pages.at(static_cast<size_t>(_page)); }

	const GameFont& _font;
	std::u16string _playerName;
	std::u16string _welcome;
	std::array<std::u16string, k_ButtonCount> _buttonLabels;
	std::u16string _yes;
	std::u16string _no;
	/// HELP_TEXT_DIALOG_QUIT_QUESTION, asked by the first page, and HELP_TEXT_DIALOG_AREYOUSUREQUIT by the options
	std::u16string _quitQuestion;
	std::u16string _quitGameQuestion;
	std::array<std::u16string, 5> _detailLabels;
	std::array<std::u16string, 5> _helpLabels;
	std::array<std::u16string, 3> _storyLabels;
	std::array<std::u16string, 4> _toolTipLabels;
	MenuSettings _settings;
	std::array<std::unique_ptr<Dialog>, static_cast<size_t>(Page::_Count)> _pages;
	List* _controls {nullptr};
	/// The wheel's mice on the Controls page turn over
	std::vector<std::pair<size_t, bool>> _wheelMice;
	float _wheelTime {0.0f};

	Page _page {Page::Main};
	bool _open {false};
	Zoomer _fade;
	std::optional<Question> _question;
	Action _action {Action::None};
	bool _clicked {false};
	bool _answered {false};
	bool _settingsChanged {false};
};

} // namespace openblack::gui
