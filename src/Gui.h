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

#include <array>
#include <memory>

#include <bgfx/bgfx.h>
#include <imgui.h>

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Cursor SDL_Cursor;
union SDL_Event;

namespace openblack
{
class Game;
class GameWindow;
class MeshViewer;

class Gui
{
  public:
	static std::unique_ptr<Gui> create(const GameWindow &window, bgfx::ViewId viewId, float scale);

	virtual ~Gui();

	bool ProcessEventSdl2(const SDL_Event& event);
	void NewFrame(GameWindow& window);
	void Loop(Game& game);
	void Draw();
 private:
	Gui(ImGuiContext *imgui, bgfx::ViewId viewId, std::unique_ptr<MeshViewer> &&meshViewer);
	bool InitSdl2(SDL_Window* window);
	void NewFrameSdl2(SDL_Window* window);
	void NewFrameBgfx();
	bool CreateFontsTextureBgfx();
	bool CreateDeviceObjectsBgfx();
	void RenderDrawDataBgfx(ImDrawData* drawData);

	static const char* StaticGetClipboardText(void* ud) { return reinterpret_cast<Gui*>(ud)->GetClipboardText(); }
	static void StaticSetClipboardText(void* ud, const char* text)  { reinterpret_cast<Gui*>(ud)->SetClipboardText(text); }
	const char* GetClipboardText();
	void SetClipboardText(const char* text);

	void UpdateMousePosAndButtons();
	void UpdateMouseCursor();
	void UpdateGamepads();

	ImGuiContext* _imgui;
	uint64_t _time;
	bgfx::VertexLayout  _layout;
	bgfx::ProgramHandle _program;
	bgfx::ProgramHandle _imageProgram;
	bgfx::TextureHandle _texture;
	bgfx::TextureHandle _fontTexture;
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
