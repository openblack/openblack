/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/fwd.hpp>
#include <imgui.h>

#include "DebugGuiInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::debug::gui
{
class Window;

class Gui final: public DebugGuiInterface
{
public:
	Gui(ImGuiContext* imgui, bgfx::ViewId viewId, std::vector<std::unique_ptr<Window>>&& debugWindows, bool headless) noexcept;
	~Gui() noexcept override;

	[[nodiscard]] bool StealsFocus() const noexcept override;
	void SetScale(float scale) noexcept override;
	bool ProcessEvents(const SDL_Event& event) noexcept override;
	bool Loop() noexcept override;
	void Draw() noexcept override;
	void EnableMain() noexcept override;
	void DisableMain() noexcept override;

private:
	bool CreateFontsTextureBgfx() noexcept;
	bool CreateDeviceObjectsBgfx() noexcept;
	void NewFrame() noexcept;
	void RenderDrawDataBgfx(ImDrawData* drawData) noexcept;

	void RenderArrow(const std::string& name, const ImVec2& pos, const ImVec2& size) const noexcept;
	std::optional<glm::uvec4> RenderVillagerName(const std::vector<glm::vec4>& coveredAreas, const std::string& name,
	                                             const std::string& text, const glm::vec4& color, const ImVec2& pos,
	                                             float arrowLength, std::function<void(void)> debugCallback) const noexcept;
	bool ShowMenu() noexcept;
	void ShowVillagerNames() noexcept;
	void ShowCameraPositionOverlay() noexcept;

	ImGuiContext* _imgui;
	const bool _headless;
	ImVec2 _menuBarSize;
	uint64_t _time {0};
	bgfx::VertexLayout _layout;
	bgfx::ProgramHandle _program;
	bgfx::ProgramHandle _imageProgram;
	bgfx::TextureHandle _texture;
	bgfx::UniformHandle _s_tex;
	bgfx::UniformHandle _u_imageLodEnabled;
	const bgfx::ViewId _viewId;
	std::vector<std::unique_ptr<Window>> _debugWindows;
	std::string _screenshotFilename = "screenshot.png";
	bool _stealsFocus = false;
	bool _enableMain = true;
};
} // namespace openblack::debug::gui
