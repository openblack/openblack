/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Graphics/RenderPass.h"

#include <bgfx/bgfx.h>
#include <glm/fwd.hpp>
#include <imgui.h>

#include <array>
#include <functional>
#include <memory>
#include <optional>

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
class Console;
class Game;
class GameWindow;
class MeshViewer;
class Renderer;

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
	Gui(ImGuiContext* imgui, bgfx::ViewId viewId, std::unique_ptr<MeshViewer>&& meshViewer,
	    std::unique_ptr<Console>&& terminal);
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
	void ShowProfilerWindow(Game& game);
	void ShowWaterFramebuffer(const Game& game);
	void ShowLandIslandWindow(Game& game);
	void ShowCameraPositionOverlay(const Game& game);
	void ShowTempleInteriorWindow(Game& game);

	static const char* StaticGetClipboardText(void* ud) { return reinterpret_cast<Gui*>(ud)->GetClipboardText(); }
	static void StaticSetClipboardText(void* ud, const char* text) { reinterpret_cast<Gui*>(ud)->SetClipboardText(text); }
	const char* GetClipboardText();
	void SetClipboardText(const char* text);

	void UpdateMousePosAndButtons();
	void UpdateMouseCursor();
	void UpdateGamepads();

	template <typename T, uint8_t N>
	struct CircularBuffer
	{
		static constexpr uint8_t _bufferSize = N;
		T _values[_bufferSize] = {};
		uint8_t _offset = 0;

		[[nodiscard]] T back() const { return _values[_offset]; }
		void pushBack(T value)
		{
			_values[_offset] = value;
			_offset = (_offset + 1u) % _bufferSize;
		}
	};

	ImGuiContext* _imgui;
	ImVec2 _menuBarSize;
	uint64_t _time;
	CircularBuffer<float, 100> _times;
	CircularBuffer<float, 100> _fps;
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
	std::unique_ptr<MeshViewer> _meshViewer;
	std::unique_ptr<Console> _console;
};
} // namespace openblack
