#pragma once
#ifndef OPENBLACK_GAME_H
#define OPENBLACK_GAME_H

#include <3D/Camera.h>
#include <string>
#include <vector>

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

		Camera& GetCamera() { return *mCamera; }
		

	private:
		Camera *mCamera;

		SDL_Window * mWindow;
		void * mGLContext;
		std::vector<uint32_t> mIndicesArray;

		void createWindow(int width, int height);
		void setWindowIcon();
		void createContext();

		void loop();
	};
}

#endif
