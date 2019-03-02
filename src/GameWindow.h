#pragma once
#ifndef OPENBLACK_GAMEWINDOW_H
#define OPENBLACK_GAMEWINDOW_H

#include <memory>
#include <string>
#include <cstdint>
#include <SDL.h>

struct SDLDestroyer {
	void operator()(SDL_Window* window) const { SDL_DestroyWindow(window); }
	void operator()(SDL_GLContext* glcontext) const { SDL_GL_DeleteContext(glcontext); }
};

namespace OpenBlack
{
	enum DisplayMode { Windowed, Fullscreen, Borderless };

	class GameWindow {
	public:
		GameWindow(const std::string& title, const SDL_DisplayMode& display, DisplayMode displaymode);
		GameWindow(const std::string& title, int width, int height, DisplayMode displaymode);

		SDL_Window* GetHandle() const;
		SDL_GLContext* GetGLContext() const;

		bool IsOpen() const;
		float GetBrightness() const;
		void SetBrightness(float bright);
		uint32_t GetID() const;
		uint32_t GetFlags() const;
		void GrabInput(bool b = true);
		void SetMousePosition(int x, int y);
		bool IsInputGrabbed();
		float GetAspectRatio() const;

		void SetTitle(const std::string& str);
		std::string GetTitle() const;

		int GetSwapInterval();
		void SetSwapInterval(int interval);

		void Show();
		void Hide();

		void SetPosition(int x, int y);
		void GetPosition(int& x, int& y);
		void SetSize(int width, int height);
		void GetSize(int& width, int& height);
		void SetMinimumSize(int width, int height);
		void GetMinimumSize(int& width, int& height);
		void SetMaximumSize(int width, int height);
		void GetMaximumSize(int& width, int& height);

		void Minimise();
		void Maximise();
		void Restore();
		void Raise();
		void SetBordered(bool b = true);
		void SetFullscreen(bool f = true);

		void Close();
		void SwapWindow();
	private:
		std::unique_ptr<SDL_Window, SDLDestroyer> _window;
		std::unique_ptr<SDL_GLContext, SDLDestroyer> _glcontext;
	};
}

#endif
