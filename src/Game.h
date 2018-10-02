#pragma once
#ifndef OPENBLACK_GAME_H
#define OPENBLACK_GAME_H

#include <3D/Camera.h>
#include <string>
#include <vector>

#include <3D/MeshPack.h>

#include <MeshViewer.h>

struct SDL_Window;

namespace OpenBlack
{
	class Game {
	public:
		Game(int argc, char **argv);
		~Game();

		void Run();

		void LoadMap(std::string name);

		std::string GetGamePath();

		Camera& GetCamera() { return *m_Camera; }
		MeshPack& GetMeshPack() { return *m_MeshPack; }
		
		static Game* instance() {
			return sInstance;
		}
	private:
		static Game* sInstance;

		MeshViewer *m_MeshViewer;

		Camera *m_Camera;
		MeshPack *m_MeshPack;

		SDL_Window * m_Window;
		void * m_GLContext;

		bool m_Running;

		void createWindow(int width, int height);

		void guiLoop();
		void loop();
	};
}

#endif
