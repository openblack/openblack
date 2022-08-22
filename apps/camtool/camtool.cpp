/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <CAMFile.h>
#include <cxxopts.hpp>

struct Arguments
{
	enum class Mode : uint8_t
	{
		Header,
		PrintPoints,
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
		uint32_t movementSpeed;
		std::vector<std::string> pointStrings;
	} write;
};

int PrintHeader(const openblack::cam::CAMFile& cam)
{
	const auto& header = cam.GetHeader();

	std::printf("file: %s\n", cam.GetFilename().c_str());
	std::printf("file size: %u bytes\n", header.size);
	std::printf("movement speed: %u\n", header.durationMilliseconds);
	std::printf("point count: %u\n", header.pointCount);
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintPoints(const openblack::cam::CAMFile& cam) noexcept
{
	const auto& points = cam.GetPoints();

	std::printf("file: %s\n", cam.GetFilename().c_str());
	std::printf("%u points {position, heading}\n", cam.GetHeader().pointCount);
	for (uint32_t i = 0; const auto& point : points)
	{
		std::printf("[%u]: {(%f, %f, %f), (%f, %f, %f)}\n", i++, point.position[0], point.position[1], point.heading[2],
		            point.heading[0], point.heading[1], point.heading[2]);
	}

	return EXIT_SUCCESS;
}

int WriteFile(const Arguments::Write& args) noexcept
{
	openblack::cam::CAMFile cam {};

	cam.SetMovementSpeed(args.movementSpeed);

	if (args.pointStrings.empty())
	{
		return EXIT_FAILURE;
	}
	std::vector<openblack::cam::CAMPoint> points;
	points.reserve(args.pointStrings.size());
	for (const auto& str : args.pointStrings)
	{
		std::smatch match;
		try
		{
			std::regex_match(
			    str, match,
			    std::regex {R"( *(-?\d+\.\d+) ?(-?\d+\.\d+) ?(-?\d+\.\d+) ?(-?\d+\.\d+) ?(-?\d+\.\d+) ?(-?\d+\.\d+))"});
		}
		catch (...)
		{
			return EXIT_FAILURE;
		}
		if (match.size() != 7)
		{
			return EXIT_FAILURE;
		}
		auto& point = points.emplace_back();
		point.position[0] = std::stof(match[1]);
		point.position[1] = std::stof(match[2]);
		point.position[2] = std::stof(match[3]);
		point.heading[0] = std::stof(match[4]);
		point.heading[1] = std::stof(match[5]);
		point.heading[2] = std::stof(match[6]);
	}

	cam.SetPoints(std::move(points));

	try
	{
		cam.Write(args.outFilename);
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("camtool", "Inspect and extract files from LionHead camera files.");

	try
	{
		options.add_options()                                            //
		    ("h,help", "Display this help message.")                     //
		    ("subcommand", "Subcommand.", cxxopts::value<std::string>()) //
		    ;
		options.positional_help("[read|write] [OPTION...]");
		options.add_options("read")                                                                           //
		    ("H,header", "Print Header Contents.", cxxopts::value<std::vector<std::filesystem::path>>())      //
		    ("p,print-points", "Print List of Points.", cxxopts::value<std::vector<std::filesystem::path>>()) //
		    ;
		options.add_options("write")                                                          //
		    ("o,output", "Output file (required).", cxxopts::value<std::filesystem::path>())  //
		    ("s,movement-speed", "Value for the movement speed.", cxxopts::value<uint32_t>()) //
		    ("points", "Point in format \"p1 p2 p3 h1 h2 h3\" array (comma-separated).",      //
		     cxxopts::value<std::vector<std::string>>())                                      //
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
			if (result["header"].count() > 0)
			{
				args.mode = Arguments::Mode::Header;
				args.read.filenames = result["header"].as<std::vector<std::filesystem::path>>();
				return true;
			}
			if (result["print-points"].count() > 0)
			{
				args.mode = Arguments::Mode::PrintPoints;
				args.read.filenames = result["print-points"].as<std::vector<std::filesystem::path>>();
				return true;
			}
		}
		else if (result["subcommand"].as<std::string>() == "write")
		{
			args.write.outFilename = "";
			if (result["output"].count() > 0 && result["movement-speed"].count() > 0 && result["points"].count() > 0)
			{
				args.mode = Arguments::Mode::Write;
				args.write.outFilename = result["output"].as<std::filesystem::path>();
				args.write.movementSpeed = result["movement-speed"].as<uint32_t>();
				args.write.pointStrings = result["points"].as<std::vector<std::string>>();
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
		openblack::cam::CAMFile cam;
		try
		{
			// Open file
			cam.Open(filename);

			switch (args.mode)
			{
			case Arguments::Mode::Header:
				returnCode |= PrintHeader(cam);
				break;
			case Arguments::Mode::PrintPoints:
				returnCode |= PrintPoints(cam);
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
