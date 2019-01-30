#pragma once
#ifndef OPENBLACK_GAME_H
#define OPENBLACK_GAME_H

#include <string>
#include <vector>

#include "GameWindow.h"

namespace OpenBlack
{
	class Camera;
	class MeshPack;
	class MeshViewer;
	class LandIsland;

	class Game {
	public:
		Game(int argc, char **argv);
		~Game();

		void Run();

		void LoadMap(std::string name);

		std::string GetGamePath();

		Camera& GetCamera() { return *_camera; }
		MeshPack& GetMeshPack() { return *_meshPack; }
		
		static Game* instance() {
			return sInstance;
		}
	private:
		static Game* sInstance;

		//std::unique_ptr<MeshViewer> _meshViewer;
		std::unique_ptr<LandIsland> _landIsland;
		std::unique_ptr<MeshPack> _meshPack;
		std::unique_ptr<Camera> _camera;
		std::unique_ptr<GameWindow> _window;

		bool _running;

		void guiLoop();
	};
}

#endif
