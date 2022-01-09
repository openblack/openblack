/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <ANMFile.h>

#include <cstdlib>
#include <string>

#include <cxxopts.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

int PrintHeader(openblack::anm::ANMFile& anm)
{
	auto& header = anm.GetHeader();

	std::printf("file: %s\n", anm.GetFilename().c_str());

	std::printf("name: %s\n", header.name);
	std::printf("unknown_0x20: 0x%X\n", header.unknown_0x20);
	std::printf("unknown_0x24: %f\n", header.unknown_0x24);
	std::printf("unknown_0x28: %f\n", header.unknown_0x28);
	std::printf("unknown_0x2C: %f\n", header.unknown_0x2C);
	std::printf("unknown_0x30: %f\n", header.unknown_0x30);
	std::printf("unknown_0x34: %f\n", header.unknown_0x34);
	std::printf("frame count: %u\n", header.frame_count);
	std::printf("unknown_0x3C: 0x%X\n", header.unknown_0x3C);
	std::printf("animation_duration: %u\n", header.animation_duration);
	std::printf("unknown_0x44: 0x%X\n", header.unknown_0x44);
	std::printf("unknown_0x48: 0x%X\n", header.unknown_0x48);
	std::printf("frames base: 0x%X\n", header.frames_base);
	std::printf("unknown_0x50: 0x%X\n", header.unknown_0x50);

	return EXIT_SUCCESS;
}

int ListKeyframes(openblack::anm::ANMFile& anm)
{
	auto& keyframes = anm.GetKeyframes();

	std::printf("file: %s\n", anm.GetFilename().c_str());

	uint32_t last_time = 0;
	for (uint32_t i = 0; i < keyframes.size(); ++i)
	{
		std::printf("%u: time %u (+%u)\n", i, keyframes[i].time, keyframes[i].time - last_time);
		last_time = keyframes[i].time;
	}

	return EXIT_SUCCESS;
}

int ViewKeyframe(openblack::anm::ANMFile& anm)
{
	for (uint32_t index = 0; index < anm.GetKeyframes().size(); ++index)
	{
		const auto& keyframe = anm.GetKeyframe(index);

		std::printf("file: %s, frame: %u, time: %u\n", anm.GetFilename().c_str(), index, keyframe.time);
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
		std::vector<std::string> filenames;
	} read;
	struct Write
	{
		std::string outFilename;
		std::string gltfFile;
	} write;
};

int WriteFile(const Arguments::Write& args)
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
	char name[] = "TODO";
	memcpy(anm.GetHeader().name, name, sizeof(name));
	anm.GetHeader().frame_count = 0;
	anm.GetHeader().animation_duration = 0;
	anm.Write(args.outFilename);

	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("anmtool", "Inspect and extract files from LionHead ANM files.");

	// clang-format off
	options.add_options()
		("h,help", "Display this help message.")
		("subcommand", "Subcommand.", cxxopts::value<std::string>())
	;
	options.positional_help("[read|write] [OPTION...]");
	options.add_options()
		("H,header", "Print Header Contents.", cxxopts::value<std::vector<std::string>>())
		("l,list-keyframes", "List Keyframes.", cxxopts::value<std::vector<std::string>>())
		("k,keyframe-content", "View Keyframe Contents", cxxopts::value<std::vector<std::string>>())
	;
	options.add_options("write from and to glTF format")
		("o,output", "Output file (required).", cxxopts::value<std::string>())
		("i,input-mesh", "Input file (required).", cxxopts::value<std::string>())
	;
	// clang-format on

	options.parse_positional({"subcommand"});

	try
	{
		auto result = options.parse(argc, argv);
		if (result["help"].as<bool>())
		{
			std::cout << options.help() << std::endl;
			return_code = EXIT_SUCCESS;
			return false;
		}
		if (result["subcommand"].count() == 0)
		{
			std::cerr << options.help() << std::endl;
			return_code = EXIT_FAILURE;
			return false;
		}
		if (result["subcommand"].as<std::string>() == "read")
		{
			if (result["header"].count() > 0)
			{
				args.mode = Arguments::Mode::Header;
				args.read.filenames = result["header"].as<std::vector<std::string>>();
				return true;
			}
			if (result["list-keyframes"].count() > 0)
			{
				args.mode = Arguments::Mode::ListKeyframes;
				args.read.filenames = result["list-keyframes"].as<std::vector<std::string>>();
				return true;
			}
			if (result["keyframe-content"].count() > 0)
			{
				args.mode = Arguments::Mode::Keyframe;
				args.read.filenames = result["keyframe-content"].as<std::vector<std::string>>();
				return true;
			}
		}
		else if (result["subcommand"].as<std::string>() == "write")
		{
			args.write.outFilename = "";
			if (result["output"].count() > 0)
			{
				args.mode = Arguments::Mode::Write;
				args.write.outFilename = result["output"].as<std::string>();
				if (result["input-mesh"].count() > 0)
				{
					args.write.gltfFile = result["input-mesh"].as<std::string>();
				}
				return true;
			}
		}
	}
	catch (cxxopts::OptionParseException& err)
	{
		std::cerr << err.what() << std::endl;
	}

	std::cerr << options.help() << std::endl;
	return_code = EXIT_FAILURE;
	return false;
}

int main(int argc, char* argv[])
{
	Arguments args;
	int return_code = EXIT_SUCCESS;
	if (!parseOptions(argc, argv, args, return_code))
	{
		return return_code;
	}

	if (args.mode == Arguments::Mode::Write)
	{
		return WriteFile(args.write);
	}

	for (auto& filename : args.read.filenames)
	{
		openblack::anm::ANMFile anm;
		try
		{
			// Open file
			anm.Open(filename);

			switch (args.mode)
			{
			case Arguments::Mode::Header:
				return_code |= PrintHeader(anm);
				break;
			case Arguments::Mode::ListKeyframes:
				return_code |= ListKeyframes(anm);
				break;
			case Arguments::Mode::Keyframe:
				return_code |= ViewKeyframe(anm);
				break;
			default:
				return_code = EXIT_FAILURE;
				break;
			}
		}
		catch (std::runtime_error& err)
		{
			std::cerr << err.what() << std::endl;
			return_code |= EXIT_FAILURE;
		}
	}

	return return_code;
}
