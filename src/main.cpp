/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Game.h"

#include <cxxopts.hpp>
#include <iostream>
#include <map>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

bool parseOptions(int argc, char** argv, openblack::Arguments& args, int& return_code)
{
	cxxopts::Options options("openblack", "Open source reimplementation of the game Black & White (2001).");

	options.add_options()
		("h,help", "Display this help message.")
		("g,game-path", "Path to the Data/ and Scripts/ directories of the original Black & White game. (Required)", cxxopts::value<std::string>())
		("W,width", "Window resolution in the x axis.", cxxopts::value<uint16_t>()->default_value("1280"))
		("H,height", "Window resolution in the y axis.", cxxopts::value<uint16_t>()->default_value("1024"))
		("s,gui-scale", "Scaling of the GUI", cxxopts::value<float>()->default_value("1.0"))
		("V,vsync", "Enable Vertical Sync.")
		("m,window-mode", "Which mode to run window.", cxxopts::value<std::string>()->default_value("windowed"))
		("b,backend-type", "Which backend to use for rendering.", cxxopts::value<std::string>()->default_value("OpenGL"))
		("n,num-frames-to-simulate", "Number of frames to simulate before quitting.", cxxopts::value<uint32_t>()->default_value("0"))
	;

	try
	{
		auto result = options.parse(argc, argv);
		if (result["help"].as<bool>())
		{
			std::cout << options.help() << std::endl;
			return_code = EXIT_SUCCESS;
			return false;
		}
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
		auto rendererIter = rendererLookup.find(result["backend-type"].as<std::string>());
		if (rendererIter != rendererLookup.cend())
		{
			rendererType = rendererIter->second;
		}
		else
		{
			throw cxxopts::option_not_exists_exception(result["backend-type"].as<std::string>());
		}

		static const std::map<std::string_view, openblack::DisplayMode> displayModeLookup = {
		    std::pair {"windowed", openblack::DisplayMode::Windowed},
		    std::pair {"fullscreen", openblack::DisplayMode::Fullscreen},
		    std::pair {"borderless", openblack::DisplayMode::Borderless},
		};

		openblack::DisplayMode displayMode;
		auto displayModeIter = displayModeLookup.find(result["window-mode"].as<std::string>());
		if (displayModeIter != displayModeLookup.cend())
		{
			displayMode = displayModeIter->second;
		}
		else
		{
			throw cxxopts::option_not_exists_exception(result["window-mode"].as<std::string>());
		}

		args.executablePath = argv[0];
		if (result.count("game-path") == 0)
		{
#ifdef _WIN32
			// if we're on windows we can find the install path
			DWORD dataLen = 0;
			LSTATUS status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir",
			                             RRF_RT_REG_SZ, nullptr, nullptr, &dataLen);
			if (status == ERROR_SUCCESS)
			{
				char* path = new char[dataLen];
				status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir",
				                     RRF_RT_REG_SZ, nullptr, path, &dataLen);

				args.gamePath = std::string(path);
			}
			else
#endif
			{
				throw cxxopts::option_required_exception("game-path");
			}
		}
		else
		{
			args.gamePath = result["game-path"].as<std::string>();
		}
		args.windowWidth = result["width"].as<uint16_t>();
		args.windowHeight = result["height"].as<uint16_t>();
		args.scale = result["gui-scale"].as<float>();
		args.vsync = result["vsync"].as<bool>();
		args.displayMode = displayMode;
		args.rendererType = rendererType;
		args.numFramesToSimulate = result["num-frames-to-simulate"].as<uint32_t>();
	}
	catch (cxxopts::OptionParseException& err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << options.help() << std::endl;

		return_code = EXIT_FAILURE;
		return false;
	}

	return true;
}

int main(int argc, char** argv)
{
	std::cout << "============================================================="
	             "=================\n"
	             "   openblack - A modern reimplementation of Lionhead's Black "
	             "& White (2001)   \n"
	             "============================================================="
	             "=================\n"
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
