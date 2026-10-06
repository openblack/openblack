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

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <glm/vec2.hpp>

#include "Canvas.h"
#include "Creature/CreatureFightHud.h"
#include "Creature/CreatureStatusPanel.h"
#include "DialogPainter.h"
#include "GameFont.h"
#include "GameMenu.h"
#include "ScreenFade.h"
#include "TextDatabase.h"
#include "ToolTips.h"

union SDL_Event;

namespace openblack::graphics
{
class Texture2D;
}

namespace openblack::gui
{

/// The game's own interface, drawn over the scene: for now the menu that Escape brings up.
///
/// It loads what Black & White's dialogs are made of: the texts of the info scripts, the font j0 and the front end
/// atlas, and takes the mouse and keyboard from the game while a dialog is open.
class GameInterface
{
public:
	/// Null when the game's files for it are missing
	static std::unique_ptr<GameInterface> Create(std::u16string_view playerName, MenuSettings settings);
	~GameInterface();

	/// Takes Escape, and every key press and mouse event while the menu is open. True when the event was taken.
	bool ProcessEvent(const SDL_Event& event, glm::u16vec2 resolution);
	/// What the player last chose in the menu, once
	GameMenu::Action TakeAction();
	/// Whether the menu's settings changed since last asked
	bool TakeSettingsChanged() { return _menu->TakeSettingsChanged(); }

	void Update(float deltaSeconds);
	/// The open dialogs and, over them, the pointer at the mouse. The pointer also shows over the debug windows, which
	/// hide the hand, and is drawn over them.
	void Draw(glm::u16vec2 resolution, glm::ivec2 mouse, uint32_t milliseconds, bool overDebugWindow);

	[[nodiscard]] GameMenu& GetMenu() noexcept { return *_menu; }
	[[nodiscard]] const TextDatabase& GetTexts() const noexcept { return _texts; }
	/// The dialogs' font, which the temple's scrolls are written in too
	[[nodiscard]] const GameFont& GetFont() const noexcept { return _font; }
	/// The font's glyphs, white with their coverage in alpha
	[[nodiscard]] const graphics::Texture2D& GetFontTexture() const noexcept { return *_fontTexture; }

	/// Words shown over the screen, wrapped across the dialogs' 800 by 600 and 60 high from its top, as the temple's
	/// future room shows them
	struct Message
	{
		std::u16string text;
		float alpha;
	};
	void SetMessage(std::optional<Message> message) { _message = std::move(message); }

	/// What the hand shows for what it is over
	[[nodiscard]] ToolTips& GetToolTips() noexcept { return _toolTips; }
	/// The colour over the whole screen, over the interface too, which the temple fades in and out by
	[[nodiscard]] ScreenFade& GetScreenFade() noexcept { return _screenFade; }
	/// Where on the screen the hand is, in pixels, which the tooltip is drawn by, or nowhere to show none
	void SetHandOnScreen(std::optional<glm::vec2> position) { _handOnScreen = position; }
	/// The creature's status panel shown this frame, or none. It shows its reward row when the values have a reward,
	/// and near the top of the screen without one, as while the camera follows a creature.
	void SetCreaturePanel(std::optional<creature_panel::Values> values) { _creaturePanel = values; }
	/// The fight's panel shown this frame, or none: each fighter's name over its health and stamina
	void SetFightPanel(std::optional<creature_fight_hud::Values> values) { _fightPanel = std::move(values); }

private:
	GameInterface(TextDatabase texts, GameFont font, std::unique_ptr<graphics::Texture2D> atlas,
	              std::unique_ptr<graphics::Texture2D> fontTexture, std::unique_ptr<graphics::Texture2D> symbols,
	              std::unique_ptr<graphics::Texture2D> mice, std::unique_ptr<graphics::Texture2D> atmos,
	              std::u16string_view playerName, MenuSettings settings);

	/// The tooltip by the hand, its words and then its mouse
	void DrawToolTip(glm::u16vec2 resolution);
	/// The creature's status panel, at the left of the screen
	void DrawCreaturePanel(glm::u16vec2 resolution);
	/// The fight's panel, at the top left of the screen
	void DrawFightPanel(glm::u16vec2 resolution);
	/// The tooltip's glow: a soft box of atmos.raw added round a rectangle
	void DrawGlow(glm::vec2 min, glm::vec2 max, glm::vec4 colour);

	TextDatabase _texts;
	GameFont _font;
	std::unique_ptr<graphics::Texture2D> _atlas;
	std::unique_ptr<graphics::Texture2D> _fontTexture;
	std::unique_ptr<graphics::Texture2D> _symbols;
	std::unique_ptr<graphics::Texture2D> _mice;
	/// The atmosphere texture of glows and arrows
	std::unique_ptr<graphics::Texture2D> _atmos;
	Canvas _canvas;
	Canvas _pointerCanvas {graphics::RenderPass::Cursor};
	DialogPainter _painter;
	std::unique_ptr<GameMenu> _menu;
	std::optional<Message> _message;
	ToolTips _toolTips;
	ScreenFade _screenFade;
	std::optional<glm::vec2> _handOnScreen;
	std::optional<creature_panel::Values> _creaturePanel;
	std::optional<creature_fight_hud::Values> _fightPanel;
	/// Whether the tooltip is left of the hand, which it moves to in the right third of the screen and from in the left
	bool _toolTipOnLeft {false};
	GameMenu::Action _action {GameMenu::Action::None};
};

} // namespace openblack::gui
