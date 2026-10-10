/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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
	Gui(ImGuiContext* imgui, std::vector<std::unique_ptr<Window>>&& debugWindows, bool headless) noexcept;
	~Gui() noexcept override;

	[[nodiscard]] bool StealsFocus() const noexcept override;
	void SetScale(float scale) noexcept override;
	bool ProcessEvents(const SDL_Event& event) noexcept override;
	bool Loop() noexcept override;
	void Draw(SDL_GPUCommandBuffer* cmdBuf, SDL_GPUTexture* targetTexture) noexcept override;

private:
	void NewFrame() noexcept;

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
	std::vector<std::unique_ptr<Window>> _debugWindows;
	std::string _screenshotFilename = "screenshot.png";
	bool _stealsFocus = false;
};
} // namespace openblack::debug::gui
