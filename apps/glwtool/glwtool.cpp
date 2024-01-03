/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <iostream>
#include <string>

#include <GLWFile.h>
#include <cxxopts.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

using namespace openblack::glw;

int ListGlows(GLWFile& glw)
{
	const auto& glows = glw.GetGlows();

	std::printf("file: %s\n", glw.GetFilename().c_str());

	for (uint32_t i = 0; i < glows.size(); ++i)
	{
		std::printf("\t%u: name \"%s\"\n", i, glows[i].name);
	}

	return EXIT_SUCCESS;
}

int ViewGlow(GLWFile& glw)
{
	for (uint32_t index = 0; index < glw.GetGlows().size(); ++index)
	{
		const auto& glow = glw.GetGlow(index);

		std::printf("file: %s, glow: %u\n", glw.GetFilename().c_str(), index);
		std::printf("information:\n");
		std::printf("\tname: %s\n", glow.name);
		std::printf("\tsize: %u\n", glow.size);
		std::printf("\tunk1: %u\n", glow.unk1);
		std::printf("\tred: %f\n", glow.red);
		std::printf("\tgreen: %f\n", glow.green);
		std::printf("\tblue: %f\n", glow.blue);
		std::printf("\tglowX: %f\n", glow.glowX);
		std::printf("\tglowY: %f\n", glow.glowY);
		std::printf("\tglowZ: %f\n", glow.glowZ);
		std::printf("\tunkX: %f\n", glow.unkX);
		std::printf("\tunkY: %f\n", glow.unkY);
		std::printf("\tunkZ: %f\n", glow.unkZ);
		std::printf("\tunkX2: %f\n", glow.unkX2);
		std::printf("\tunkY2: %f\n", glow.unkY2);
		std::printf("\tunkZ2: %f\n", glow.unkZ2);
		std::printf("\tunk14: %f\n", glow.unk14);
		std::printf("\tdirX: %f\n", glow.dirX);
		std::printf("\tdirY: %f\n", glow.dirY);
		std::printf("\tdirZ: %f\n", glow.dirZ);
	}

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode
	{
		Header,
		ListGlows,
		ViewGlow,
		Write,
	};
	Mode mode;
	struct Read
	{
		std::vector<std::filesystem::path> filenames;
	} read;
	struct Write
	{
		std::filesystem::path outFilename;
		std::filesystem::path gltfFile;
	} write;
};

int WriteFile(const Arguments::Write& args) noexcept
{
	GLWFile glw {};

	if (args.gltfFile.empty())
	{
		return EXIT_FAILURE;
	}

	tinygltf::TinyGLTF loader;
	tinygltf::Model gltf;

	std::string err;
	std::string warn;

	//	const std::string name = "TODO";
	//	memcpy(anm.GetHeader().name.data(), name.c_str(),
	//	       std::min(sizeof(anm.GetHeader().name[0]) * anm.GetHeader().name.size(), name.length()));
	//	anm.GetHeader().frameCount = 0;
	//	anm.GetHeader().animationDuration = 0;
	//	anm.Write(args.outFilename);

	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("glwtool", "Inspect and extract files from LionHead GLW files.");

	try
	{
		options.add_options()                                            //
		    ("h,help", "Display this help message.")                     //
		    ("subcommand", "Subcommand.", cxxopts::value<std::string>()) //
		    ;
		options.positional_help("[read|write] [OPTION...]");
		options.add_options()                                                                              //
		    ("l,list-glows", "List glows.", cxxopts::value<std::vector<std::filesystem::path>>())          //
		    ("g,glow-content", "View Glow Contents", cxxopts::value<std::vector<std::filesystem::path>>()) //
		    ;
		options.add_options("write from and to glTF format")                                    //
		    ("o,output", "Output file (required).", cxxopts::value<std::filesystem::path>())    //
		    ("i,input-glow", "Input file (required).", cxxopts::value<std::filesystem::path>()) //
		    ;

		options.parse_positional({"subcommand"});
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		returnCode = EXIT_FAILURE;
		return false;
	}

	try
	{
		auto result = options.parse(argc, argv);
		if (result["help"].as<bool>())
		{
			std::cout << options.help() << std::endl;
			returnCode = EXIT_SUCCESS;
			return false;
		}
		if (result["subcommand"].count() == 0)
		{
			std::cerr << options.help() << std::endl;
			returnCode = EXIT_FAILURE;
			return false;
		}
		if (result["subcommand"].as<std::string>() == "read")
		{
			if (result["list-glows"].count() > 0)
			{
				args.mode = Arguments::Mode::ListGlows;
				args.read.filenames = result["list-glows"].as<std::vector<std::filesystem::path>>();
				return true;
			}
			if (result["glow-content"].count() > 0)
			{
				args.mode = Arguments::Mode::ViewGlow;
				args.read.filenames = result["glow-content"].as<std::vector<std::filesystem::path>>();
				return true;
			}
		}
		else if (result["subcommand"].as<std::string>() == "write")
		{
			args.write.outFilename = "";
			if (result["output"].count() > 0)
			{
				args.mode = Arguments::Mode::Write;
				args.write.outFilename = result["output"].as<std::filesystem::path>();
				if (result["input-glow"].count() > 0)
				{
					args.write.gltfFile = result["input-mesh"].as<std::filesystem::path>();
				}
				return true;
			}
		}
	}
	catch (const std::exception& err)
	{
		std::cerr << err.what() << std::endl;
	}

	std::cerr << options.help() << std::endl;
	returnCode = EXIT_FAILURE;
	return false;
}

int main(int argc, char* argv[]) noexcept
{
	Arguments args;
	int returnCode = EXIT_SUCCESS;
	if (!parseOptions(argc, argv, args, returnCode))
	{
		return returnCode;
	}

	if (args.mode == Arguments::Mode::Write)
	{
		return WriteFile(args.write);
	}

	for (auto& filename : args.read.filenames)
	{
		GLWFile glw;
		try
		{
			// Open file
			glw.Open(filename);

			switch (args.mode)
			{
			case Arguments::Mode::Header:
				break;
			case Arguments::Mode::ListGlows:
				returnCode |= ListGlows(glw);
				break;
			case Arguments::Mode::ViewGlow:
				returnCode |= ViewGlow(glw);
				break;
			default:
				returnCode = EXIT_FAILURE;
				break;
			}
		}
		catch (std::exception& err)
		{
			std::cerr << err.what() << std::endl;
			returnCode |= EXIT_FAILURE;
		}
	}

	return returnCode;
}
