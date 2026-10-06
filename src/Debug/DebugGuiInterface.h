/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>
#include <string_view>

#include "Graphics/RenderPass.h"

struct SDL_Window;
struct SDL_Cursor;
union SDL_Event;

namespace openblack::graphics
{
class Renderer;
}

namespace openblack::debug::gui
{
/// The name of the window of the testbed's scenarios, which opens with the testbed
constexpr std::string_view k_TestbedScenariosWindow = "Testbed Scenarios";

class DebugGuiInterface
{
public:
	static std::unique_ptr<DebugGuiInterface> Create(graphics::RenderPass viewId) noexcept;

	virtual ~DebugGuiInterface() noexcept = default;
	[[nodiscard]] virtual bool StealsFocus() const noexcept = 0;
	/// The mouse is over a debug window, or held on one, as of the last frame of them
	[[nodiscard]] virtual bool IsMouseOverWindow() const noexcept = 0;
	virtual void SetScale(float scale) noexcept = 0;
	/// The main menu bar, which only shows while the game's own menu is open
	virtual void SetMenuBarVisible(bool visible) noexcept = 0;
	virtual bool ProcessEvents(const SDL_Event& event) noexcept = 0;
	/// Opens the debug window of the name, if there is one
	virtual void OpenWindow(std::string_view name) noexcept = 0;
	virtual bool Loop() noexcept = 0;
	virtual void Draw() noexcept = 0;
};
} // namespace openblack::debug::gui
