/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <InfoConstants.h>

#include <cstdlib>
#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>

struct Arguments
{
	std::filesystem::path outFilename;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("gen_info", "Generate Mock info.dat file.");

	// clang-format off
	options.add_options()
		("h,help", "Display this help message.")
		("o,output-file", "Where to generate file.", cxxopts::value<std::filesystem::path>())
	;
	// clang-format on

	std::filesystem::path outFilename;
	try
	{
		auto result = options.parse(argc, argv);
		if (result["help"].as<bool>())
		{
			std::cout << options.help() << std::endl;
			return_code = EXIT_SUCCESS;
			return false;
		}
		if (result["output-file"].count() > 0)
		{
			args.outFilename = result["output-file"].as<std::filesystem::path>();
			return true;
		}
		else
		{
			std::cerr << options.help() << std::endl;
			return_code = EXIT_FAILURE;
			return false;
		}
	}
	catch (cxxopts::OptionParseException& err)
	{
		std::cerr << err.what() << std::endl;
		return_code = EXIT_FAILURE;
		return false;
	}
}

int main(int argc, char* argv[])
{
	Arguments args;
	int return_code = EXIT_SUCCESS;
	if (!parseOptions(argc, argv, args, return_code))
	{
		return return_code;
	}

	openblack::InfoConstants constants;
	std::memset(&constants, 0, sizeof(constants));

	// Add abode name used in scene test
	const std::string abodeDebugName = "ABODE_F";
	std::memcpy(constants.abode[0].debugString.data(), abodeDebugName.c_str(), abodeDebugName.length());

	std::ofstream output(args.outFilename, std::ios::binary);
	output.write(reinterpret_cast<const char*>(&constants), sizeof(constants));

	return EXIT_SUCCESS;
}
