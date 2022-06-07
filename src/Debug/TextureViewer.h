/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#pragma once

#include <entt/fwd.hpp>

#include "Window.h"

namespace openblack::debug::gui
{
class TextureViewer: public Window
{
public:
	TextureViewer();

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	entt::id_type _selectedTexture;
	ImGuiTextFilter _filter;
};
} // namespace openblack::debug::gui
