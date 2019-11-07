/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <Graphics/RenderPass.h>
#include <array>
#include <bgfx/bgfx.h>
#include <imgui.h>
#include <memory>

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Cursor SDL_Cursor;
union SDL_Event;

namespace ImGui
{
#define IMGUI_FLAGS_NONE UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle _handle, uint8_t _flags, uint8_t _mip, const ImVec2& _size, const ImVec2& _uv0 = ImVec2(0.0f, 0.0f), const ImVec2& _uv1 = ImVec2(1.0f, 1.0f), const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f))
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
	texture.s.flags  = _flags;
	texture.s.mip    = _mip;
	Image(texture.ptr, _size, _uv0, _uv1, _tintCol, _borderCol);
}

// Helper function for passing bgfx::TextureHandle to ImGui::Image.
inline void Image(bgfx::TextureHandle _handle, const ImVec2& _size, const ImVec2& _uv0 = ImVec2(0.0f, 0.0f), const ImVec2& _uv1 = ImVec2(1.0f, 1.0f), const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f))
{
	Image(_handle, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _tintCol, _borderCol);
}

} // namespace ImGui

namespace openblack
{
class Game;
class GameWindow;
class MeshViewer;

class Gui
{
  public:
	static std::unique_ptr<Gui> create(const GameWindow& window, graphics::RenderPass viewId, float scale);

	virtual ~Gui();

	bool ProcessEventSdl2(const SDL_Event& event);
	void NewFrame(GameWindow& window);
	bool Loop(Game& game);
	void Draw();

  private:
	Gui(ImGuiContext* imgui, bgfx::ViewId viewId, std::unique_ptr<MeshViewer>&& meshViewer);
	bool InitSdl2(SDL_Window* window);
	void NewFrameSdl2(SDL_Window* window);
	bool CreateFontsTextureBgfx();
	bool CreateDeviceObjectsBgfx();
	void RenderDrawDataBgfx(ImDrawData* drawData);

	void ShowProfilerWindow(Game& game);

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
		T _values[_bufferSize]               = {};
		uint8_t _offset                      = 0;

		[[nodiscard]] T back() const { return _values[_offset]; }
		void pushBack(T value)
		{
			_values[_offset] = value;
			_offset          = (_offset + 1u) % _bufferSize;
		}
	};

	ImGuiContext* _imgui;
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
};
} // namespace openblack
