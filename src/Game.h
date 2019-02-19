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
