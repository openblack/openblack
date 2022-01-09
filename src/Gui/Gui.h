/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <bgfx/bgfx.h>
#include <glm/fwd.hpp>
#include <imgui.h>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <Graphics/RenderPass.h>

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Cursor SDL_Cursor;
union SDL_Event;

namespace ImGui
{
#define IMGUI_FLAGS_NONE UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle _handle, uint8_t _flags, uint8_t _mip, const ImVec2& _size,
                  const ImVec2& _uv0 = ImVec2(0.0f, 0.0f), const ImVec2& _uv1 = ImVec2(1.0f, 1.0f),
                  const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                  const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f))
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
	texture.s.handle = _handle;
	texture.s.flags = _flags;
	texture.s.mip = _mip;
	Image(texture.ptr, _size, _uv0, _uv1, _tintCol, _borderCol);
}

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle _handle, const ImVec2& _size, const ImVec2& _uv0 = ImVec2(0.0f, 0.0f),
                  const ImVec2& _uv1 = ImVec2(1.0f, 1.0f), const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                  const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f))
{
	Image(_handle, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _tintCol, _borderCol);
}

} // namespace ImGui

namespace openblack
{
class Game;
class GameWindow;
class Renderer;
} // namespace openblack

namespace openblack::gui
{
class DebugWindow;

class Gui
{
public:
	static std::unique_ptr<Gui> create(const GameWindow* window, graphics::RenderPass viewId, float scale);

	virtual ~Gui();

	bool ProcessEventSdl2(const SDL_Event& event);
	void NewFrame(GameWindow* window);
	bool Loop(Game& game, const Renderer& renderer);
	void Draw();

private:
	Gui(ImGuiContext* imgui, bgfx::ViewId viewId, std::vector<std::unique_ptr<DebugWindow>>&& debugWindows);
	bool InitSdl2(SDL_Window* window);
	void NewFrameSdl2(SDL_Window* window);
	bool CreateFontsTextureBgfx();
	bool CreateDeviceObjectsBgfx();
	void RenderDrawDataBgfx(ImDrawData* drawData);

	void RenderArrow(const std::string& name, const ImVec2& pos, const ImVec2& size) const;
	std::optional<glm::uvec4> RenderVillagerName(const std::vector<glm::vec4>& coveredAreas, const std::string& name,
	                                             const std::string& text, const glm::vec4& color, const ImVec2& pos,
	                                             float arrowLength, std::function<void(void)> debugCallback) const;
	bool ShowMenu(Game& game);
	void ShowVillagerNames(const Game& game);
	void ShowCameraPositionOverlay(const Game& game);

	static const char* StaticGetClipboardText(void* ud) { return reinterpret_cast<Gui*>(ud)->GetClipboardText(); }
	static void StaticSetClipboardText(void* ud, const char* text) { reinterpret_cast<Gui*>(ud)->SetClipboardText(text); }
	const char* GetClipboardText();
	void SetClipboardText(const char* text);

	void UpdateMousePosAndButtons();
	void UpdateMouseCursor();
	void UpdateGamepads();

	ImGuiContext* _imgui;
	ImVec2 _menuBarSize;
	uint64_t _time;
	bgfx::DynamicVertexBufferHandle _vertexBuffer;
	bgfx::DynamicIndexBufferHandle _indexBuffer;
	uint32_t _vertexCount;
	uint32_t _indexCount;
	bgfx::VertexLayout _layout;
	bgfx::ProgramHandle _program;
	bgfx::ProgramHandle _imageProgram;
	bgfx::TextureHandle _texture;
	bgfx::UniformHandle _s_tex;
	bgfx::UniformHandle _u_imageLodEnabled;
	SDL_Window* _window;
	std::array<bool, 3> _mousePressed;
	std::array<SDL_Cursor*, ImGuiMouseCursor_COUNT> _mouseCursors;
	char* _clipboardTextData;
	int64_t _last;
	int32_t _lastScroll;
	const bgfx::ViewId _viewId;
	std::vector<std::unique_ptr<DebugWindow>> _debugWindows;
};
} // namespace openblack::gui
