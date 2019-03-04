/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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
#ifndef OPENBLACK_GAME_H
#define OPENBLACK_GAME_H

#include <string>
#include <vector>

#include "GameWindow.h"
#include <LHScriptX/Command.h>

namespace OpenBlack
{
	class Camera;
	class MeshPack;
	class MeshViewer;
	class LandIsland;

	namespace LHScriptX {
		class Script;
	}

	class Game {
	public:
		Game(int argc, char **argv);
		~Game();

		void Run();

		void LoadMap(std::string name);

		std::string GetGamePath();

		GameWindow& GetWindow() { return *_window;  }
		Camera& GetCamera() { return *_camera; }
		MeshPack& GetMeshPack() { return *_meshPack; }
		
		static Game* instance() {
			return sInstance;
		}
	private:
		static Game* sInstance;

		std::unique_ptr<GameWindow> _window;
		std::unique_ptr<Camera> _camera;

		std::unique_ptr<LandIsland> _landIsland;
		std::unique_ptr<MeshPack> _meshPack;

		std::unique_ptr<LHScriptX::Script> _scriptx;

		bool _wireframe;

		bool _running;

		void guiLoop();
	};
}

#endif
