/*****************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <cstdlib>

#include <filesystem>
#include <fstream>
#include <iostream>

#include <InfoConstants.h>
#include <cxxopts.hpp>

struct Arguments
{
	std::filesystem::path outFilename;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode)
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
			returnCode = EXIT_SUCCESS;
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
			returnCode = EXIT_FAILURE;
			return false;
		}
	}
	catch (cxxopts::OptionParseException& err)
	{
		std::cerr << err.what() << std::endl;
		returnCode = EXIT_FAILURE;
		return false;
	}
}

int main(int argc, char* argv[]) noexcept
{
	Arguments args;
	int returnCode = EXIT_SUCCESS;
	if (!parseOptions(argc, argv, args, returnCode))
	{
		return returnCode;
	}

	openblack::InfoConstants constants;
	std::memset(&constants, 0, sizeof(constants));

	// Add celtic abode name and mesh ids used in scene test
	for (uint8_t i = 0; i < 6; ++i)
	{
		auto& abode = constants.abode[i];
		auto abodeDebugName = std::string("ABODE_") + openblack::k_AbodeNumberStrs[i].data();
		abode.abodeNumber = static_cast<openblack::AbodeNumber>(i);
		std::memcpy(abode.debugString.data(), abodeDebugName.c_str(), abodeDebugName.length());
		abode.meshId = static_cast<openblack::MeshId>(static_cast<uint32_t>(openblack::MeshId::BuildingCeltic1) + i);
	}
	std::string townCentreDebugName = "ABODE_TOWN_CENTRE";
	constants.abode[12].abodeNumber = openblack::AbodeNumber::TownCentre;
	std::memcpy(constants.abode[12].debugString.data(), townCentreDebugName.c_str(), townCentreDebugName.length());
	constants.abode[12].meshId = openblack::MeshId::BuildingCelticVillageCentre;

	std::ofstream output(args.outFilename, std::ios::binary);
	output.write(reinterpret_cast<const char*>(&constants), sizeof(constants));

	return EXIT_SUCCESS;
}
