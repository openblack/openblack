/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

/* Example commands with packfile:
 * Listing the Bank Info for all sounds in Black & White Audio directories:
 * find "Black and White/Audio" -name "*.sad" \
 *     -printf "packtool -s LHFileSegmentBankInfo -e stdout %p | ./cmake-build-debug-gcc/bin/sadtool read -i stdin\n" | sh
 */

#include <SADFile.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <string>

#include <cxxopts.hpp>

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

int PrintInfo(const openblack::sad::SADFile& sad)
{
	const auto& info = sad.GetInfo();

	std::printf("file: %s\n", std::get<0>(sad.GetFilenames()).c_str());

	std::printf("unknown_0x0: 0x%08X\n", info.unknown_0x0);
	std::printf("unknown_0x4: 0x%08X\n", info.unknown_0x4);
	std::printf("music_bank: %s (0x%08X)\n", info.musicBank ? "true" : "false", info.musicBank);
	std::printf("description: \"%s\"\n", info.description.data());
	std::string backwards_comment(info.backwardsComment.data(), 0, info.backwardsComment.size());
	std::printf("backwards_comment: \"%s\"\n", info.backwardsComment.data());
	std::reverse(backwards_comment.begin(), backwards_comment.end());
	std::printf("backwards_comment (reversed): \"%s\"\n", backwards_comment.c_str());

	return EXIT_SUCCESS;
}

int PrintTable(const openblack::sad::SADFile& sad)
{
	const auto& table = sad.GetTable();

	std::printf("file: %s\n", std::get<1>(sad.GetFilenames()).c_str());

	std::printf("sample count: %zu\n", table.size());
	uint32_t i = 0;
	for (const auto& sample : table)
	{
		std::printf("sample %u:\n", i);
		std::printf("\taudioSampleName: %s\n", sample.name.data());
		std::printf("\tunknown0: 0x%08X\n", sample.unknown0);
		std::printf("\tid: %0x08X\n", sample.id);
		std::printf("\tisBank: %s (0x%08X)\n", sample.isBank ? "true" : "false", sample.isBank);
		std::printf("\tsize: 0x%08X\n", sample.size);
		std::printf("\trelativeOffset: 0x%08X\n", sample.offset);
		std::printf("\tclone: 0x%08X\n", sample.isClone);
		std::printf("\tgroup: 0x%04X\n", sample.group);
		std::printf("\tatmosGroup: 0x%04X\n", sample.atmosGroup);
		std::printf("\tunknown4: 0x%08X\n", sample.unknown4);
		std::printf("\tunknown5: 0x%08X\n", sample.unknown5);
		std::printf("\tunknown6a: 0x%04X\n", sample.unknown6a);
		std::printf("\tunknown6b: 0x%04X\n", sample.unknown6b);
		std::printf("\tsampleRate: 0x%08X\n", sample.sampleRate);
		std::printf("\tunknownOthera: 0x%04X\n", sample.unknownOthera);
		std::printf("\tunknownOtherb: 0x%04X\n", sample.unknownOtherb);
		std::printf("\tunknown7a: 0x%04X\n", sample.unknown7a);
		std::printf("\tunknown7b: 0x%04X\n", sample.unknown7b);
		std::printf("\tunknown8: 0x%08X\n", sample.unknown8);
		std::printf("\tlStart: 0x%08X\n", sample.lStart);
		std::printf("\tlEnd: 0x%08X\n", sample.lEnd);
		std::printf("\tsampleDescription: %s\n", sample.description.data());
		std::printf("\tpriority: 0x%04X\n", sample.priority);
		std::printf("\tunknown9: 0x%04X\n", sample.unknown9);
		std::printf("\tunknown10: 0x%04X\n", sample.unknown10);
		std::printf("\tunknown11: 0x%04X\n", sample.unknown11);
		std::printf("\tloop: 0x%04X\n", sample.loop);
		std::printf("\tstart: 0x%04X\n", sample.start);
		std::printf("\tpan: 0x%02X\n", sample.pan);
		std::printf("\tunknown12: 0x%04X\n", sample.unknown12);
		std::printf("\tpos: (%f, %f, %f)\n", sample.pos[0], sample.pos[1], sample.pos[2]);
		std::printf("\tvolume: 0x%02X\n", sample.volume);
		std::printf("\tuserParam: 0x%04X\n", sample.userParam);
		std::printf("\tpitch: 0x%04X\n", sample.pitch);
		std::printf("\tunknown18: 0x%04X\n", sample.unknown18);
		std::printf("\tpitchDeviation: 0x%04X\n", sample.pitchDeviation);
		std::printf("\tunknown20: 0x%04X\n", sample.unknown20);
		std::printf("\tminDist: %f\n", sample.minDist);
		std::printf("\tmaxDist: %f\n", sample.maxDist);
		std::printf("\tscale: %f\n", sample.scale);
		switch (sample.loopType)
		{
		case openblack::sad::AudioBankLoop::None:
			std::printf("\tloopType: None\n");
			break;
		case openblack::sad::AudioBankLoop::Restart:
			std::printf("\tloopType: Restart\n");
			break;
		case openblack::sad::AudioBankLoop::Once:
			std::printf("\tloopType: Once\n");
			break;
		case openblack::sad::AudioBankLoop::Overlap:
			std::printf("\tloopType: Overlap\n");
			break;
		default:
			std::printf("\tloopType: Unknown (0x%04X)\n", static_cast<uint16_t>(sample.loopType));
			break;
		}
		std::printf("\tunknown21: 0x%04X\n", sample.unknown21);
		std::printf("\tunknown22: 0x%04X\n", sample.unknown22);
		std::printf("\tunknown23: 0x%04X\n", sample.unknown23);
		std::printf("\tatmos: 0x%04X\n", sample.atmos);

		++i;
	}

	return EXIT_SUCCESS;
}

int PrintData(const openblack::sad::SADFile& sad)
{
	const auto& table = sad.GetTable();

	std::printf("file: %s\n", std::get<2>(sad.GetFilenames()).c_str());

	for (size_t i = 0; i < table.size(); ++i)
	{
		const auto& sample = table[i];
		const auto& soundData = sad.GetSoundData(i);
		std::printf("name: %s\n", sample.name.data());
		auto result = PrintRawBytes(soundData.data(), soundData.size() * sizeof(soundData[0]));
		if (result != EXIT_SUCCESS)
		{
			return result;
		}
	}

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode
	{
		Info,
		Table,
		Data,
	};
	Mode mode;
	struct Read
	{
		std::vector<std::string> filenames;
	} read;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("sadtool", "Inspect, read and write files from LionHead .sad files.");

	// clang-format off
	options.add_options()
		("h,help", "Display this help message.")
		("subcommand", "Subcommand.", cxxopts::value<std::string>())
	;
	options.positional_help("[read] [OPTION...] [INFO][+[TABLE][+[DATA]]]");
	options.add_options()
		("i,info", "Print Info Segment Contents.", cxxopts::value<std::vector<std::string>>())
		("t,table", "Print Table Segment Contents.", cxxopts::value<std::vector<std::string>>())
		("d,data", "Print Data Segment Contents.", cxxopts::value<std::vector<std::string>>())
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
			if (result["info"].count() > 0)
			{
				args.mode = Arguments::Mode::Info;
				args.read.filenames = result["info"].as<std::vector<std::string>>();
				return true;
			}
			if (result["table"].count() > 0)
			{
				args.mode = Arguments::Mode::Table;
				args.read.filenames = result["table"].as<std::vector<std::string>>();
				return true;
			}
			if (result["data"].count() > 0)
			{
				args.mode = Arguments::Mode::Data;
				args.read.filenames = result["data"].as<std::vector<std::string>>();
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

	openblack::sad::SADFile sad;

	//	auto exec = [&args, &return_code, &sad]() {
	//		try
	//		{
	//			switch (args.mode)
	//			{
	//			case Arguments::Mode::Info:
	//				return_code |= PrintInfo(sad);
	//				break;
	//			case Arguments::Mode::Table:
	//				return_code |= PrintTable(sad);
	//				break;
	//			case Arguments::Mode::Data:
	//				return_code |= PrintData(sad);
	//				break;
	//			default:
	//				return_code = EXIT_FAILURE;
	//				break;
	//			}
	//		}
	//		catch (std::runtime_error& err)
	//		{
	//			std::cerr << err.what() << std::endl;
	//			return_code |= EXIT_FAILURE;
	//		}
	//	};

	for (const auto& filenames : args.read.filenames)
	{
		// Split the filenames into the info+table+data trio
		std::string infoFilename;
		std::string tableFilename;
		std::string dataFilename;
		size_t start = 0;
		size_t sep = filenames.find_first_of("+", start);
		infoFilename = filenames.substr(start, sep);
		if (sep != std::string::npos)
		{
			start = sep + 1;
			sep = filenames.find_first_of("+", start);
			tableFilename = filenames.substr(start, sep);
			if (sep != std::string::npos)
			{
				start = sep + 1;
				sep = filenames.find_first_of("+", start);
				dataFilename = filenames.substr(start, sep);
			}
		}

		// Read from stream buffer such as when piped, only support one and ignore other inputs
		if (infoFilename == "stdin" || tableFilename == "stdin" || dataFilename == "stdin")
		{
			std::vector<uint8_t> buffer;
			std::array<uint8_t, 0x100> partial;
			while (!std::cin.eof())
			{
				auto read = fread(reinterpret_cast<char*>(partial.data()), 1, partial.size(), stdin);
				if (read == 0)
				{
					break;
				}
				buffer.insert(buffer.end(), &partial[0], &partial[read]);
			}

			std::vector<uint8_t> infoBuffer;
			std::vector<uint8_t> tableBuffer;
			std::vector<uint8_t> dataBuffer;

			if (infoFilename == "stdin")
			{
				infoBuffer = buffer;
			}
			else if (tableFilename == "stdin")
			{
				tableBuffer = buffer;
			}
			else
			{
				dataBuffer = buffer;
			}

			sad.Open(infoBuffer, tableBuffer, dataBuffer);
		}
		else
		{
			// Read sad files with provided optional paths
			sad.Open(infoFilename, tableFilename, dataFilename);
		}

		try
		{
			switch (args.mode)
			{
			case Arguments::Mode::Info:
				return_code |= PrintInfo(sad);
				break;
			case Arguments::Mode::Table:
				return_code |= PrintTable(sad);
				break;
			case Arguments::Mode::Data:
				return_code |= PrintData(sad);
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

	//	// Read from stream buffer such as when piped
	//	if (args.read.info.filenames.size() > 0 && args.read.info.filenames[0] == "stdin")
	//	{
	//		std::vector<uint8_t> buffer;
	//		std::array<uint8_t, 0x100> partial;
	//		while (!std::cin.eof())
	//		{
	//			auto read = fread(reinterpret_cast<char*>(partial.data()), 1, partial.size(), stdin);
	//			if (read == 0)
	//			{
	//				break;
	//			}
	//			buffer.insert(buffer.end(), &partial[0], &partial[read]);
	//		}
	//		sad.Open(buffer, {}, {});
	//		exec();
	//		return return_code;
	//	}
	//
	//	size_t largest = std::max(args.read.info.filenames.size(), static_cast<size_t>(0));
	//
	//	for (size_t i = 0; i < largest; ++i)
	//	{
	//		// Open files
	//		sad.Open(i < args.read.info.filenames.size() ? args.read.info.filenames[i] : std::string(),
	//		         i < args.read.table.filenames.size() ? args.read.table.filenames[i] : std::string(),
	//		         i < args.read.data.filenames.size() ? args.read.data.filenames[i] : std::string());
	//		exec();
	//	}

	return return_code;
}
