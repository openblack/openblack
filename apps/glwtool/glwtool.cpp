/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <GLWFile.h>
#include <cxxopts.hpp>

#include "json.hpp"

using namespace openblack::glw;

int ListGlows(GLWFile& glw)
{
	const auto& glows = glw.GetGlows();

	for (uint32_t i = 0; i < glows.size(); ++i)
	{
		std::printf("\t%u: name \"%s\"\n", i, glows[i].name.data());
	}

	return EXIT_SUCCESS;
}

int ViewGlow(const std::filesystem::path& path, GLWFile& glw)
{
	for (uint32_t index = 0; index < glw.GetGlows().size(); ++index)
	{
		const auto& glow = glw.GetGlow(index);

		std::printf("file: %s, glow: %u\n", path.generic_string().c_str(), index);
		std::printf("information:\n");
		std::printf("\tname: %s\n", glow.name.data());
		std::printf("\tred: %f\n", glow.red);
		std::printf("\tgreen: %f\n", glow.green);
		std::printf("\tblue: %f\n", glow.blue);
		std::printf("\tPosition XYZ: %f, %f, %f\n", glow.posX, glow.posY, glow.posZ);
	}

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode : uint8_t
	{
		Header,
		ListGlows,
		ViewGlow,
		Write,
		Extract,
	};
	Mode mode;
	struct Read
	{
		std::vector<std::filesystem::path> filenames;
	} read;
	struct Write
	{
		std::filesystem::path outFilename;
		std::filesystem::path jsonFile;
	} write;
	struct Extract
	{
		std::filesystem::path inFilename;
		std::filesystem::path jsonFile;
	} extract;
};

int WriteFile(const Arguments::Write& args) noexcept
{
	GLWFile glw {};

	if (args.jsonFile.empty())
	{
		return EXIT_FAILURE;
	}

	std::ifstream f(args.jsonFile);

	if (!f.is_open())
	{
		return EXIT_FAILURE;
	}

	nlohmann::json data;

	data = nlohmann::json::parse(f);

	if (data.is_discarded())
	{
		return EXIT_FAILURE;
	}

	for (auto jsonEmitter : data)
	{
		if (jsonEmitter.is_discarded())
		{
			return EXIT_FAILURE;
		}
		Glow glow;
		glow.size = jsonEmitter["size"];
		glow.type = jsonEmitter["type"];
		glow.red = jsonEmitter["red"];
		glow.green = jsonEmitter["green"];
		glow.blue = jsonEmitter["blue"];
		glow.posX = jsonEmitter["posX"];
		glow.posY = jsonEmitter["posY"];
		glow.posZ = jsonEmitter["posZ"];
		glow.targetX = jsonEmitter["targetX"];
		glow.targetY = jsonEmitter["targetY"];
		glow.targetZ = jsonEmitter["targetZ"];
		glow.targetDirectionX = jsonEmitter["targetDirectionX"];
		glow.targetDirectionY = jsonEmitter["targetDirectionY"];
		glow.targetDirectionZ = jsonEmitter["targetDirectionZ"];
		glow.xAxisX = jsonEmitter["xAxisX"];
		glow.xAxisY = jsonEmitter["xAxisY"];
		glow.xAxisZ = jsonEmitter["xAxisZ"];
		glow.yAxisX = jsonEmitter["yAxisX"];
		glow.yAxisY = jsonEmitter["yAxisY"];
		glow.yAxisZ = jsonEmitter["yAxisZ"];
		glow.zAxisX = jsonEmitter["zAxisX"];
		glow.zAxisY = jsonEmitter["zAxisY"];
		glow.zAxisZ = jsonEmitter["zAxisZ"];
		glow.originX = jsonEmitter["originX"];
		glow.originY = jsonEmitter["originY"];
		glow.originZ = jsonEmitter["originZ"];
		glow.coneLength = jsonEmitter["coneLength"];
		glow.flags = jsonEmitter["flags"];
		glow.hotspotAngle = jsonEmitter["hotspotAngle"];
		glow.falloffAngle = jsonEmitter["falloffAngle"];
		glow.attenuationStart = jsonEmitter["attenuationStart"];
		glow.attenuationEnd = jsonEmitter["attenuationEnd"];
		const std::string name = jsonEmitter["name"];
		assert(name.length() < glow.name.size());
		glow.name.fill(0);
		std::copy(name.begin(), name.end(), glow.name.data());
		glow.emitterSize = jsonEmitter["emitterSize"];
		glw.AddGlow(glow);
	}

	return glw.Write(args.outFilename) == openblack::glw::GLWResult::Success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int ExtractFile(const Arguments::Extract& args) noexcept
{
	GLWFile glw {};

	if (args.jsonFile.empty() || args.inFilename.empty())
	{
		return EXIT_FAILURE;
	}

	const auto result = glw.Open(args.inFilename);

	if (result != openblack::glw::GLWResult::Success)
	{
		std::cerr << openblack::glw::ResultToStr(result) << '\n';
		return EXIT_FAILURE;
	}

	nlohmann::ordered_json data;
	for (const auto& glow : glw.GetGlows())
	{
		nlohmann::ordered_json jsonEmitter;
		jsonEmitter["size"] = glow.size;
		jsonEmitter["type"] = glow.type;
		jsonEmitter["red"] = glow.red;
		jsonEmitter["green"] = glow.green;
		jsonEmitter["blue"] = glow.blue;
		jsonEmitter["posX"] = glow.posX;
		jsonEmitter["posY"] = glow.posY;
		jsonEmitter["posZ"] = glow.posZ;
		jsonEmitter["targetX"] = glow.targetX;
		jsonEmitter["targetY"] = glow.targetY;
		jsonEmitter["targetZ"] = glow.targetZ;
		jsonEmitter["targetDirectionX"] = glow.targetDirectionX;
		jsonEmitter["targetDirectionY"] = glow.targetDirectionY;
		jsonEmitter["targetDirectionZ"] = glow.targetDirectionZ;
		jsonEmitter["xAxisX"] = glow.xAxisX;
		jsonEmitter["xAxisY"] = glow.xAxisY;
		jsonEmitter["xAxisZ"] = glow.xAxisZ;
		jsonEmitter["yAxisX"] = glow.yAxisX;
		jsonEmitter["yAxisY"] = glow.yAxisY;
		jsonEmitter["yAxisZ"] = glow.yAxisZ;
		jsonEmitter["zAxisX"] = glow.zAxisX;
		jsonEmitter["zAxisY"] = glow.zAxisY;
		jsonEmitter["zAxisZ"] = glow.zAxisZ;
		jsonEmitter["originX"] = glow.originX;
		jsonEmitter["originY"] = glow.originY;
		jsonEmitter["originZ"] = glow.originZ;
		jsonEmitter["coneLength"] = glow.coneLength;
		jsonEmitter["flags"] = glow.flags;
		jsonEmitter["hotspotAngle"] = glow.hotspotAngle;
		jsonEmitter["falloffAngle"] = glow.falloffAngle;
		jsonEmitter["attenuationStart"] = glow.attenuationStart;
		jsonEmitter["attenuationEnd"] = glow.attenuationEnd;
		jsonEmitter["name"] = std::string(glow.name.data());
		jsonEmitter["emitterSize"] = glow.emitterSize;
		data.push_back(jsonEmitter);
	}

	auto o = std::ofstream(args.jsonFile);
	o << std::setw(4) << data << '\n';
	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("glwtool", "Inspect and extract information from LionHead GLW files.");

	options.add_options()                                            //
	    ("h,help", "Display this help message.")                     //
	    ("subcommand", "Subcommand.", cxxopts::value<std::string>()) //
	    ;
	options.positional_help("[read|write|extract] [OPTION...]");
	options.add_options()                                                                              //
	    ("l,list-glows", "List glows.", cxxopts::value<std::vector<std::filesystem::path>>())          //
	    ("g,glow-content", "View Glow Contents", cxxopts::value<std::vector<std::filesystem::path>>()) //
	    ;
	options.add_options("write/extract from and to json format")                            //
	    ("o,output", "Output file (required).", cxxopts::value<std::filesystem::path>())    //
	    ("i,input-glow", "Input file (required).", cxxopts::value<std::filesystem::path>()) //
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
				args.write.jsonFile = result["input-glow"].as<std::filesystem::path>();
			}
			return true;
		}
	}
	else if (result["subcommand"].as<std::string>() == "extract")
	{
		args.write.outFilename = "";
		if (result["output"].count() > 0)
		{
			args.mode = Arguments::Mode::Extract;
			args.extract.inFilename = result["input-glow"].as<std::filesystem::path>();
			if (result["input-glow"].count() > 0)
			{
				args.extract.jsonFile = result["output"].as<std::filesystem::path>();
			}
			return true;
		}
	}

	std::cerr << options.help() << '\n';
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

	if (args.mode == Arguments::Mode::Extract)
	{
		return ExtractFile(args.extract);
	}

	for (auto& filename : args.read.filenames)
	{
		GLWFile glw;

		// Open file
		const auto result = glw.Open(filename);
		if (result != openblack::glw::GLWResult::Success)
		{
			std::cerr << openblack::glw::ResultToStr(result) << "\n";
			returnCode |= EXIT_FAILURE;
			continue;
		}

		switch (args.mode)
		{
		case Arguments::Mode::Header:
			break;
		case Arguments::Mode::ListGlows:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= ListGlows(glw);
			break;
		case Arguments::Mode::ViewGlow:
			returnCode |= ViewGlow(filename, glw);
			break;
		default:
			returnCode = EXIT_FAILURE;
			break;
		}
	}

	return returnCode;
}
