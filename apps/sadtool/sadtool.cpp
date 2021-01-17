/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <SadFile.h>

#include <cstdlib>
#include <cxxopts.hpp>
#include <fstream>
#include <string>

int PrintRawBytes(const void* data, std::size_t size)
{
	const uint32_t bytesPerLine = 0x10;
	std::printf("       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	std::printf("     ,------------------------------------------------\n");
	for (std::size_t y = 0; y < size / bytesPerLine; ++y)
	{
		auto base = static_cast<uint32_t>(y * bytesPerLine);
		printf("%04X |", base);
		for (uint32_t x = 0; x < bytesPerLine; ++x)
		{
			auto byte = reinterpret_cast<const uint8_t*>(data)[base + x];
			printf(" \033[%dm%02X\033[0m", byte == 0 ? 2 : 1, byte);
		}
		printf("\n");
	}
	auto remaining = size & (bytesPerLine - 1);
	if (remaining != 0)
	{
		auto base = static_cast<uint32_t>(size & ~(bytesPerLine - 1));
		printf("%04X |", base);
		for (std::size_t x = 0; x < remaining; ++x)
		{
			auto byte = reinterpret_cast<const uint8_t*>(data)[base + x];
			printf(" \033[%dm%02X\033[0m", byte == 0 ? 2 : 1, byte);
		}
		printf("\n");
	}

	return EXIT_SUCCESS;
}

int ListBlocks(openblack::sad::SadFile& sad)
{
	auto& blocks = sad.GetBlocks();

	std::printf("file: %s\n", sad.GetFilename().c_str());
	std::printf("%u blocks\n", static_cast<uint32_t>(blocks.size()));
	std::printf("%u sounds\n", static_cast<uint32_t>(sad.GetSounds().size()));
	uint32_t i = 0;
	for (auto& [name, data] : blocks)
	{
		std::printf("%u: name \"%s\", size %u\n", ++i, name.c_str(), static_cast<uint32_t>(data.size()));
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int ViewBytes(openblack::sad::SadFile& pack, const std::string& name)
{
	auto& block = pack.GetBlock(name);

	std::printf("file: %s, block %s\n", pack.GetFilename().c_str(), name.c_str());

	PrintRawBytes(block.data(), block.size() * sizeof(block[0]));

	return EXIT_SUCCESS;
}

int ViewInfo(openblack::sad::SadFile& sad)
{
	using Lookup = openblack::sad::SadBlockHeader;
//	auto lookup = sad.GetInfoBlockLookup();
//	std::sort(lookup.begin(), lookup.end(), [](const Lookup& a, Lookup& b) { return a.blockId < b.blockId; });

	std::printf("file: %s\n", sad.GetFilename().c_str());

//	for (auto& item : lookup)
//	{
//		std::printf("block: %4x, unknown: %u\n", item.blockId, item.unknown);
//	}

	return EXIT_SUCCESS;
}

int ViewSounds(openblack::sad::SadFile& sad)
{
	using Lookup = openblack::sad::SadBlockHeader;
	auto lookup = sad.GetSounds();
//	std::sort(lookup.begin(), lookup.end(), [](const Lookup& a, Lookup& b) { return a.blockId < b.blockId; });

	std::printf("file: %s\n", sad.GetFilename().c_str());

//	for (auto& item : lookup)
//	{
//		std::printf("block: %4x, unknown: %u\n", item.blockId, item.unknown);
//	}

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode
	{
		List,
		Bytes,
		Info,
		Sounds,
		WriteSounds,
		WriteAnimationPack,
	};
	std::vector<std::string> filenames;
	Mode mode;
	std::string block;
	uint32_t blockId;
	std::string outFilename;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("sadtool", "Inspect and extract files from LionHead sad files.");

	// clang-format off
	options.add_options()
		("h,help", "Display this help message.")
		("l,list-blocks", "List block statistics.")
		("b,view-bytes", "View raw byte content of block.", cxxopts::value<std::string>())
		("i,info-block", "List INFO block statistics.")
		("B,body-block", "List Body block statistics.")
		("M,meshes-block", "List MESHES block statistics.")
		("m,mesh", "List mesh statistics.", cxxopts::value<uint32_t>())
		("T,texture-block", "View texture block statistics.")
		("t,texture", "View texture statistics.", cxxopts::value<std::string>())
		("A,animation-block", "List animation block statistics.")
		("a,animation", "List animation statistics.", cxxopts::value<uint32_t>())
		("e,extract", "Extract contents of a block to filename.", cxxopts::value<std::string>())
		("write-mesh", "Create Mesh Pack.", cxxopts::value<std::string>())
		("write-animation", "Create Mesh Pack.", cxxopts::value<std::string>())
		("sad-files", "Sad Files.", cxxopts::value<std::vector<std::string>>())
	;
	// clang-format on
	options.parse_positional({"sad-files"});
	options.positional_help("sad-files...");

	try
	{
		auto result = options.parse(argc, argv);
		args.outFilename = "";
		if (result["help"].as<bool>())
		{
			std::cout << options.help() << std::endl;
			return_code = EXIT_SUCCESS;
			return false;
		}
		if (result["write-mesh"].count() > 0)
		{
			args.mode = Arguments::Mode::WriteSounds;
			args.outFilename = result["write-mesh"].as<std::string>();
			return true;
		}
		if (result["write-animation"].count() > 0)
		{
			args.mode = Arguments::Mode::WriteAnimationPack;
			args.outFilename = result["write-animation"].as<std::string>();
			return true;
		}
		// Following this, all args require positional arguments
		if (result["sad-files"].count() == 0)
		{
			throw cxxopts::missing_argument_exception("sad-files");
		}
		if (result["list-blocks"].count() > 0)
		{
			args.mode = Arguments::Mode::List;
			args.filenames = result["sad-files"].as<std::vector<std::string>>();
			return true;
		}
		if (result["view-bytes"].count() > 0)
		{
			args.mode = Arguments::Mode::Bytes;
			args.filenames = result["sad-files"].as<std::vector<std::string>>();
			args.block = result["view-bytes"].as<std::string>();
			return true;
		}
		if (result["info-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Info;
			args.filenames = result["sad-files"].as<std::vector<std::string>>();
			return true;
		}
		if (result["body-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Sounds;
			args.filenames = result["sad-files"].as<std::vector<std::string>>();
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

	for (auto& filename : args.filenames)
	{
		openblack::sad::SadFile sad;
		try
		{
			// Open file
			sad.Open(filename);

			switch (args.mode)
			{
			case Arguments::Mode::List:
				return_code |= ListBlocks(sad);
				break;
			case Arguments::Mode::Bytes:
				return_code |= ViewBytes(sad, args.block);
				break;
			case Arguments::Mode::Info:
				return_code |= ViewInfo(sad);
				break;
			case Arguments::Mode::Sounds:
				return_code |= ViewSounds(sad);
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
