/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
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

int ViewKeyframe(openblack::anm::ANMFile& anm, uint32_t index)
{
	if (index > anm.GetKeyframes().size())
	{
		return EXIT_FAILURE;
	}

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

	return EXIT_SUCCESS;
}

int ViewBone(openblack::anm::ANMFile& anm, uint32_t index)
{
	for (const auto& keyframe : anm.GetKeyframes())
	{
		if (index > keyframe.bones.size())
		{
			return EXIT_FAILURE;
		}
	}

	std::printf("file: %s, bone: %u\n", anm.GetFilename().c_str(), index);
	std::printf("bones:\n");

	for (uint32_t i = 0; i < anm.GetKeyframes().size(); ++i)
	{
		std::printf("%3u:\n", i);
		const auto& matrix = anm.GetKeyframes()[i].bones[index].matrix;

		std::printf("\t[%8.5f %8.5f %8.5f %8.5f]\n", matrix[0], matrix[3], matrix[6], matrix[9]);
		std::printf("\t[%8.5f %8.5f %8.5f %8.5f]\n", matrix[1], matrix[4], matrix[7], matrix[10]);
		std::printf("\t[%8.5f %8.5f %8.5f %8.5f]\n", matrix[2], matrix[5], matrix[8], matrix[11]);
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
		Bone,
	};
	Mode mode;
	struct Read
	{
		uint32_t id;
		std::vector<std::string> filenames;
	} read;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("anmtool", "Inspect and extract files from LionHead ANM files.");

	// clang-format off
	options.add_options()
		("h,help", "Display this help message.")
		("H,header", "Print Header Contents.")
		("l,list-keyframes", "List Keyframes.")
		("k,keyframe", "View Keyframe Contents", cxxopts::value<uint32_t>())
		("b,bone", "View Bone Animation", cxxopts::value<uint32_t>())
		("anm-files", "ANM Files.", cxxopts::value<std::vector<std::string>>())
	;
	// clang-format on
	options.parse_positional({"anm-files"});
	options.positional_help("anm-files...");

	try
	{
		auto result = options.parse(argc, argv);
		if (result["help"].as<bool>())
		{
			std::cout << options.help() << std::endl;
			return_code = EXIT_SUCCESS;
			return false;
		}
		// Following this, all args require positional arguments
		if (result["anm-files"].count() == 0)
		{
			throw cxxopts::missing_argument_exception("anm-files");
		}
		if (result["header"].count() > 0)
		{
			args.mode = Arguments::Mode::Header;
			args.read.filenames = result["anm-files"].as<std::vector<std::string>>();
			return true;
		}
		if (result["list-keyframes"].count() > 0)
		{
			args.mode = Arguments::Mode::ListKeyframes;
			args.read.filenames = result["anm-files"].as<std::vector<std::string>>();
			return true;
		}
		if (result["keyframe"].count() > 0)
		{
			args.mode = Arguments::Mode::Keyframe;
			args.read.id = result["keyframe"].as<uint32_t>();
			args.read.filenames = result["anm-files"].as<std::vector<std::string>>();
			return true;
		}
		if (result["bone"].count() > 0)
		{
			args.mode = Arguments::Mode::Bone;
			args.read.id = result["bone"].as<uint32_t>();
			args.read.filenames = result["anm-files"].as<std::vector<std::string>>();
			return true;
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
				return_code |= ViewKeyframe(anm, args.read.id);
				break;
			case Arguments::Mode::Bone:
				return_code |= ViewBone(anm, args.read.id);
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
