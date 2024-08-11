/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>

#include "Graphics/RenderPass.h"

struct SDL_Window;
struct SDL_Cursor;
union SDL_Event;

namespace openblack
{
class Game;
}

namespace openblack::graphics
{
class Renderer;
}

namespace openblack::debug::gui
{
class DebugGuiInterface
{
public:
	static std::unique_ptr<DebugGuiInterface> Create(graphics::RenderPass viewId) noexcept;

	virtual ~DebugGuiInterface() noexcept = default;
	[[nodiscard]] virtual bool StealsFocus() const noexcept = 0;
	virtual void SetScale(float scale) noexcept = 0;
	virtual bool ProcessEvents(const SDL_Event& event) noexcept = 0;
	virtual bool Loop(Game& game, const graphics::Renderer& renderer) noexcept = 0;
	virtual void Draw() noexcept = 0;
};
} // namespace openblack::debug::gui
