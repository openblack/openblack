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

#include <memory>
#include <iostream>
#include <map>

#include <Game.h>
#include <Common/CmdLineArgs.h>

#ifdef _WIN32
#include <windows.h>
#endif

bool parseOptions(int argc, char** argv, openblack::Arguments& args, int& return_code)
{
	int windowWidth = 1280, windowHeight = 1024;
	bool vsync = false;
	std::string displayModeStr;
	std::string rendererTypeStr;
	float scale = 1.0f;
	std::string gamePath;

	auto cmdArgs = openblack::CmdLineArgs(argc, argv);
	cmdArgs.Get("w", windowWidth);
	cmdArgs.Get("h", windowHeight);
	cmdArgs.Get("v", vsync);
	cmdArgs.Get("m", displayModeStr);
	cmdArgs.Get("r", rendererTypeStr);
	cmdArgs.Get<float>("scale", scale);
	cmdArgs.Get("g", gamePath);

	static const std::map<std::string_view, bgfx::RendererType::Enum> rendererLookup = {
		std::pair {"OpenGL", bgfx::RendererType::OpenGL},
		std::pair {"OpenGLES", bgfx::RendererType::OpenGLES},
		std::pair {"Vulkan", bgfx::RendererType::Vulkan},
		std::pair {"Direct3D9", bgfx::RendererType::Direct3D9},
		std::pair {"Direct3D11", bgfx::RendererType::Direct3D11},
		std::pair {"Direct3D12", bgfx::RendererType::Direct3D12},
		std::pair {"Metal", bgfx::RendererType::Metal},
		std::pair {"Gnm", bgfx::RendererType::Gnm},
		std::pair {"Nvn", bgfx::RendererType::Nvn},
		std::pair {"Noop", bgfx::RendererType::Noop},
	};
	bgfx::RendererType::Enum rendererType;
	auto rendererIter = rendererLookup.find(rendererTypeStr);
	if (rendererIter != rendererLookup.cend())
	{
		rendererType = rendererIter->second;
	}
	else
	{
		rendererType = bgfx::RendererType::OpenGL;
	}

	static const std::map<std::string_view, openblack::DisplayMode> displayModeLookup = {
		std::pair{"windowed", openblack::DisplayMode::Windowed},
		std::pair{"fullscreen", openblack::DisplayMode::Fullscreen},
		std::pair{"borderless", openblack::DisplayMode::Borderless},
	};

	openblack::DisplayMode displayMode;
	auto displayModeIter = displayModeLookup.find(displayModeStr);
	if (displayModeIter != displayModeLookup.cend())
	{
		displayMode = displayModeIter->second;
	}
	else
	{
		displayMode = openblack::DisplayMode::Windowed;
	}

	args.executablePath = argv[0];
	args.gamePath = gamePath;
	args.windowWidth = windowWidth;
	args.windowHeight = windowHeight;
	args.scale = scale;
	args.vsync = vsync;
	args.displayMode = displayMode;
	args.rendererType = rendererType;

	return true;
}

int main(int argc, char** argv)
{
	std::cout << "==============================================================================\n"
	             "   openblack - A modern reimplementation of Lionhead's Black & White (2001)   \n"
	             "==============================================================================\n"
	             "\n";

#ifdef NDEBUG
	try
	{
#endif
		openblack::Arguments args;
		int return_code = EXIT_FAILURE;
		if (!parseOptions(argc, argv, args, return_code))
		{
			return return_code;
		}
		auto game = std::make_unique<openblack::Game>(std::move(args));
		game->Run();
#ifdef NDEBUG
	}
	catch (std::runtime_error& e)
	{
		// Only catch runtime_error as these should be user issues.
		// Catching other types would just make debugging them more difficult.

		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal error", e.what(), nullptr);
		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}

#if defined(_WIN32) && !defined(_CONSOLE)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	return main(__argc, __argv);
}
#endif
