/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>

typedef void *SDL_GLContext;
struct ImGuiContext;
union SDL_Event;

namespace OpenBlack
{
class Game;
class GameWindow;
class MeshViewer;

class Gui
{
  public:
	static std::unique_ptr<Gui> create(const GameWindow &window, const SDL_GLContext &context, float scale);

	virtual ~Gui();

	void ProcessSDLEvent(const SDL_Event&);
	void Loop(Game& game);
	void Draw();
 private:
	Gui(ImGuiContext* imgui, std::unique_ptr<MeshViewer>&& meshViewer);
	ImGuiContext* _imgui;
	std::unique_ptr<MeshViewer> _meshViewer;
};
} // namespace OpenBlack
