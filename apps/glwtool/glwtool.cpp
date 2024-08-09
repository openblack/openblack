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

	std::printf("file: %s\n", glw.GetFilename().c_str());

	for (uint32_t i = 0; i < glows.size(); ++i)
	{
		std::printf("\t%u: name \"%s\"\n", i, glows[i].name.data());
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

	try
	{
		data = nlohmann::json::parse(f);
	}
	catch (const nlohmann::json::parse_error&)
	{
		return EXIT_FAILURE;
	}

	try
	{
		for (auto jsonEmitter : data)
		{
			Glow glow;
			glow.size = jsonEmitter["size"];
			glow.unk1 = jsonEmitter["unk1"];
			glow.red = jsonEmitter["red"];
			glow.green = jsonEmitter["green"];
			glow.blue = jsonEmitter["blue"];
			glow.posX = jsonEmitter["posX"];
			glow.posY = jsonEmitter["posY"];
			glow.posZ = jsonEmitter["posZ"];
			glow.unkX = jsonEmitter["unkX"];
			glow.unkY = jsonEmitter["unkY"];
			glow.unkZ = jsonEmitter["unkZ"];
			glow.unkX2 = jsonEmitter["unkX2"];
			glow.unkY2 = jsonEmitter["unkY2"];
			glow.unkZ2 = jsonEmitter["unkZ2"];
			glow.unk14 = jsonEmitter["unk14"];
			glow.unk15 = jsonEmitter["unk15"];
			glow.unk16 = jsonEmitter["unk16"];
			glow.unk17 = jsonEmitter["unk17"];
			glow.unk18 = jsonEmitter["unk18"];
			glow.unk19 = jsonEmitter["unk19"];
			glow.unk20 = jsonEmitter["unk20"];
			glow.unk21 = jsonEmitter["unk21"];
			glow.unk22 = jsonEmitter["unk22"];
			glow.unk23 = jsonEmitter["unk23"];
			glow.unk24 = jsonEmitter["unk24"];
			glow.unk25 = jsonEmitter["unk25"];
			glow.unk26 = jsonEmitter["unk26"];
			glow.dirX = jsonEmitter["dirX"];
			glow.dirY = jsonEmitter["dirY"];
			glow.dirZ = jsonEmitter["dirZ"];
			glow.unk27 = jsonEmitter["unk27"];
			glow.unk28 = jsonEmitter["unk28"];
			const std::string name = jsonEmitter["name"];
			assert(name.length() < glow.name.size());
			glow.name.fill(0);
			std::copy(name.begin(), name.end(), glow.name.data());
			glow.emitterSize = jsonEmitter["emitterSize"];
			glw.AddGlow(glow);
		}
	}
	catch (const nlohmann::json::exception& error)
	{
		return error.id;
	}

	glw.Write(args.outFilename);

	return EXIT_SUCCESS;
}

int ExtractFile(const Arguments::Extract& args) noexcept
{
	GLWFile glw {};

	if (args.jsonFile.empty() || args.inFilename.empty())
	{
		return EXIT_FAILURE;
	}

	try
	{
		glw.Open(args.inFilename);
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << err.what() << '\n';
		return EXIT_FAILURE;
	}

	nlohmann::ordered_json data;
	for (const auto& glow : glw.GetGlows())
	{
		nlohmann::ordered_json jsonEmitter;
		jsonEmitter["size"] = glow.size;
		jsonEmitter["unk1"] = glow.unk1;
		jsonEmitter["red"] = glow.red;
		jsonEmitter["green"] = glow.green;
		jsonEmitter["blue"] = glow.blue;
		jsonEmitter["posX"] = glow.posX;
		jsonEmitter["posY"] = glow.posY;
		jsonEmitter["posZ"] = glow.posZ;
		jsonEmitter["unkX"] = glow.unkX;
		jsonEmitter["unkY"] = glow.unkY;
		jsonEmitter["unkZ"] = glow.unkZ;
		jsonEmitter["unkX2"] = glow.unkX2;
		jsonEmitter["unkY2"] = glow.unkY2;
		jsonEmitter["unkZ2"] = glow.unkZ2;
		jsonEmitter["unk14"] = glow.unk14;
		jsonEmitter["unk15"] = glow.unk15;
		jsonEmitter["unk16"] = glow.unk16;
		jsonEmitter["unk17"] = glow.unk17;
		jsonEmitter["unk18"] = glow.unk18;
		jsonEmitter["unk19"] = glow.unk19;
		jsonEmitter["unk20"] = glow.unk20;
		jsonEmitter["unk21"] = glow.unk21;
		jsonEmitter["unk22"] = glow.unk22;
		jsonEmitter["unk23"] = glow.unk23;
		jsonEmitter["unk24"] = glow.unk24;
		jsonEmitter["unk25"] = glow.unk25;
		jsonEmitter["unk26"] = glow.unk26;
		jsonEmitter["dirX"] = glow.dirX;
		jsonEmitter["dirY"] = glow.dirY;
		jsonEmitter["dirZ"] = glow.dirZ;
		jsonEmitter["unk27"] = glow.unk27;
		jsonEmitter["unk28"] = glow.unk28;
		jsonEmitter["name"] = std::string(glow.name.data());
		jsonEmitter["emitterSize"] = glow.emitterSize;
		try
		{
			data.push_back(jsonEmitter);
		}
		catch (nlohmann::json::exception& err)
		{
			std::cerr << err.what() << '\n';
			return EXIT_FAILURE;
		}
	}

	auto o = std::ofstream(args.jsonFile);
	o << std::setw(4) << data << '\n';
	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("glwtool", "Inspect and extract information from LionHead GLW files.");

	try
	{
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
	}
	catch (const std::exception& err)
	{
		std::cerr << err.what() << '\n';
		returnCode = EXIT_FAILURE;
		return false;
	}

	try
	{
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
	}
	catch (const std::exception& err)
	{
		std::cerr << err.what() << '\n';
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
			std::cerr << err.what() << '\n';
			returnCode |= EXIT_FAILURE;
		}
	}

	return returnCode;
}
