/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <iostream>
#include <string>

#include <ANMFile.h>
#include <cxxopts.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

int PrintHeader(openblack::anm::ANMFile& anm)
{
	auto& header = anm.GetHeader();

	std::printf("name: %s\n", header.name.data());
	std::printf("unknown0x20: 0x%X\n", header.unknown0x20);
	std::printf("unknown0x24: %f\n", header.unknown0x24);
	std::printf("unknown0x28: %f\n", header.unknown0x28);
	std::printf("unknown0x2C: %f\n", header.unknown0x2C);
	std::printf("unknown0x30: %f\n", header.unknown0x30);
	std::printf("unknown0x34: %f\n", header.unknown0x34);
	std::printf("frame count: %u\n", header.frameCount);
	std::printf("unknown0x3C: 0x%X\n", header.unknown0x3C);
	std::printf("animation_duration: %u\n", header.animationDuration);
	std::printf("unknown0x44: 0x%X\n", header.unknown0x44);
	std::printf("unknown0x48: 0x%X\n", header.unknown0x48);
	std::printf("frames base: 0x%X\n", header.framesBase);
	std::printf("unknown0x50: 0x%X\n", header.unknown0x50);

	return EXIT_SUCCESS;
}

int ListKeyframes(openblack::anm::ANMFile& anm)
{
	const auto& keyframes = anm.GetKeyframes();

	uint32_t lastTime = 0;
	for (uint32_t i = 0; i < keyframes.size(); ++i)
	{
		std::printf("%u: time %u (+%u)\n", i, keyframes[i].time, keyframes[i].time - lastTime);
		lastTime = keyframes[i].time;
	}

	return EXIT_SUCCESS;
}

int ViewKeyframe(const std::filesystem::path& filename, openblack::anm::ANMFile& anm)
{
	for (uint32_t index = 0; index < anm.GetKeyframes().size(); ++index)
	{
		const auto& keyframe = anm.GetKeyframe(index);

		std::printf("file: %s, frame: %u, time: %u\n", filename.generic_string().c_str(), index, keyframe.time);
		std::printf("bones:\n");

		for (uint32_t i = 0; i < keyframe.bones.size(); ++i)
		{
			const auto& matrix = keyframe.bones[i].matrix;
			std::printf("%3u:\n", i);
			std::printf("\t[%8.5f %8.5f %8.5f %8.5f]\n", matrix[0], matrix[3], matrix[6], matrix[9]);
			std::printf("\t[%8.5f %8.5f %8.5f %8.5f]\n", matrix[1], matrix[4], matrix[7], matrix[10]);
			std::printf("\t[%8.5f %8.5f %8.5f %8.5f]\n", matrix[2], matrix[5], matrix[8], matrix[11]);
		}
	}

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode
	{
		Header,
		ListKeyframes,
		Keyframe,
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
	openblack::anm::ANMFile anm {};

	if (args.gltfFile.empty())
	{
		return EXIT_FAILURE;
	}

	tinygltf::TinyGLTF loader;
	tinygltf::Model gltf;

	std::string err;
	std::string warn;

	// TODO (bwrsandman): Extract animation from gltf
	const std::string name = "TODO";
	memcpy(anm.GetHeader().name.data(), name.c_str(),
	       std::min(sizeof(anm.GetHeader().name[0]) * anm.GetHeader().name.size(), name.length()));
	anm.GetHeader().frameCount = 0;
	anm.GetHeader().animationDuration = 0;
	anm.Write(args.outFilename);

	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("anmtool", "Inspect and extract files from LionHead ANM files.");

	options.add_options()                                            //
	    ("h,help", "Display this help message.")                     //
	    ("subcommand", "Subcommand.", cxxopts::value<std::string>()) //
	    ;
	options.positional_help("[read|write] [OPTION...]");
	options.add_options()                                                                                      //
	    ("H,header", "Print Header Contents.", cxxopts::value<std::vector<std::filesystem::path>>())           //
	    ("l,list-keyframes", "List Keyframes.", cxxopts::value<std::vector<std::filesystem::path>>())          //
	    ("k,keyframe-content", "View Keyframe Contents", cxxopts::value<std::vector<std::filesystem::path>>()) //
	    ;
	options.add_options("write from and to glTF format")                                    //
	    ("o,output", "Output file (required).", cxxopts::value<std::filesystem::path>())    //
	    ("i,input-mesh", "Input file (required).", cxxopts::value<std::filesystem::path>()) //
	    ;

	options.parse_positional({"subcommand"});

	auto result = options.parse(argc, argv);
	if (result["help"].as<bool>())
	{
		std::cout << options.help() << '\n';
		returnCode = EXIT_SUCCESS;
		return false;
	}
	if (result["subcommand"].count() == 0)
	{
		std::cerr << options.help() << '\n';
		returnCode = EXIT_FAILURE;
		return false;
	}
	if (result["subcommand"].as<std::string>() == "read")
	{
		if (result["header"].count() > 0)
		{
			args.mode = Arguments::Mode::Header;
			args.read.filenames = result["header"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["list-keyframes"].count() > 0)
		{
			args.mode = Arguments::Mode::ListKeyframes;
			args.read.filenames = result["list-keyframes"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["keyframe-content"].count() > 0)
		{
			args.mode = Arguments::Mode::Keyframe;
			args.read.filenames = result["keyframe-content"].as<std::vector<std::filesystem::path>>();
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
			if (result["input-mesh"].count() > 0)
			{
				args.write.gltfFile = result["input-mesh"].as<std::filesystem::path>();
			}
			return true;
		}
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
		openblack::anm::ANMFile anm;

		// Open file
		const auto result = anm.Open(filename);
		if (result != openblack::anm::ANMResult::Success)
		{
			std::cerr << openblack::anm::ResultToStr(result) << "\n";
			returnCode |= EXIT_FAILURE;
			continue;
		}

		switch (args.mode)
		{
		case Arguments::Mode::Header:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= PrintHeader(anm);
			break;
		case Arguments::Mode::ListKeyframes:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= ListKeyframes(anm);
			break;
		case Arguments::Mode::Keyframe:
			returnCode |= ViewKeyframe(filename, anm);
			break;
		default:
			returnCode = EXIT_FAILURE;
			break;
		}
	}

	return returnCode;
}
