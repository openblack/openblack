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
#include <string_view>

#include <glm/vec2.hpp>

#include "Canvas.h"
#include "DialogPainter.h"
#include "GameFont.h"
#include "GameMenu.h"
#include "TextDatabase.h"

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
	/// The open dialogs and, over them, the pointer at the mouse
	void Draw(glm::u16vec2 resolution, glm::ivec2 mouse, uint32_t milliseconds);

	[[nodiscard]] GameMenu& GetMenu() noexcept { return *_menu; }
	[[nodiscard]] const TextDatabase& GetTexts() const noexcept { return _texts; }

private:
	GameInterface(TextDatabase texts, GameFont font, std::unique_ptr<graphics::Texture2D> atlas,
	              std::unique_ptr<graphics::Texture2D> fontTexture, std::unique_ptr<graphics::Texture2D> symbols,
	              std::unique_ptr<graphics::Texture2D> mice, std::u16string_view playerName, MenuSettings settings);

	TextDatabase _texts;
	GameFont _font;
	std::unique_ptr<graphics::Texture2D> _atlas;
	std::unique_ptr<graphics::Texture2D> _fontTexture;
	std::unique_ptr<graphics::Texture2D> _symbols;
	std::unique_ptr<graphics::Texture2D> _mice;
	Canvas _canvas;
	DialogPainter _painter;
	std::unique_ptr<GameMenu> _menu;
	GameMenu::Action _action {GameMenu::Action::None};
};

} // namespace openblack::gui
