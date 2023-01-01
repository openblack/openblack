/*****************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

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

#include "Graphics/RenderPass.h"

struct SDL_Window;
struct SDL_Cursor;
union SDL_Event;

namespace ImGui
{
#define IMGUI_FLAGS_NONE UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle handle, uint8_t flags, uint8_t mip, const ImVec2& size,
                  const ImVec2& cuv0 = ImVec2(0.0f, 0.0f), const ImVec2& cuv1 = ImVec2(1.0f, 1.0f),
                  const ImVec4& tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                  const ImVec4& borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f))
{
	union
	{
		struct
		{
			bgfx::TextureHandle handle;
			uint8_t flags;
			uint8_t mip;
		} s;
		ImTextureID ptr;
	} texture;
	texture.s.handle = handle;
	texture.s.flags = flags;
	texture.s.mip = mip;

	// Do y-flip
	auto uv0 = cuv0;
	auto uv1 = cuv1;
	if (bgfx::getRendererType() == bgfx::RendererType::OpenGL || bgfx::getRendererType() == bgfx::RendererType::OpenGLES)
	{
		uv0.y = 1.0f - uv0.y;
		uv1.y = 1.0f - uv1.y;
	}

	Image(texture.ptr, size, uv0, uv1, tintCol, borderCol);
}

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle handle, const ImVec2& size, const ImVec2& uv0 = ImVec2(0.0f, 0.0f),
                  const ImVec2& uv1 = ImVec2(1.0f, 1.0f), const ImVec4& tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                  const ImVec4& borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f))
{
	Image(handle, IMGUI_FLAGS_ALPHA_BLEND, 0, size, uv0, uv1, tintCol, borderCol);
}

} // namespace ImGui

namespace openblack
{
class Game;
class GameWindow;
class Renderer;
} // namespace openblack

namespace openblack::debug::gui
{
class Window;

class Gui
{
public:
	static std::unique_ptr<Gui> Create(const GameWindow* window, graphics::RenderPass viewId, float scale);

	virtual ~Gui();

	bool ProcessEvents(const SDL_Event& event);
	bool Loop(Game& game, const Renderer& renderer);
	void Draw();

private:
	Gui(ImGuiContext* imgui, bgfx::ViewId viewId, std::vector<std::unique_ptr<Window>>&& debugWindows, bool headless);
	bool CreateFontsTextureBgfx();
	bool CreateDeviceObjectsBgfx();
	void NewFrame();
	void RenderDrawDataBgfx(ImDrawData* drawData);

	void RenderArrow(const std::string& name, const ImVec2& pos, const ImVec2& size) const;
	std::optional<glm::uvec4> RenderVillagerName(const std::vector<glm::vec4>& coveredAreas, const std::string& name,
	                                             const std::string& text, const glm::vec4& color, const ImVec2& pos,
	                                             float arrowLength, std::function<void(void)> debugCallback) const;
	bool ShowMenu(Game& game);
	void ShowVillagerNames(const Game& game);
	void ShowCameraPositionOverlay(const Game& game);

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
};
} // namespace openblack::debug::gui
