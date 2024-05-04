/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <iostream>
#include <string>

#include <GLWFile.h>
#include <cxxopts.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

using namespace openblack::glw;

int ListGlows(GLWFile& glw)
{
	const auto& glows = glw.GetGlows();

	std::printf("file: %s\n", glw.GetFilename().c_str());

	for (uint32_t i = 0; i < glows.size(); ++i)
	{
		std::printf("\t%u: name \"%s\"\n", i, glows[i].name);
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
		std::printf("\tname: %s\n", glow.name);
		std::printf("\tred: %f\n", glow.red);
		std::printf("\tgreen: %f\n", glow.green);
		std::printf("\tblue: %f\n", glow.blue);
		std::printf("\tPosition XYZ: %f, %f, %f\n", glow.posX, glow.posY, glow.posZ);
	}

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode
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
	//	GLWFile glw {};
	//
	if (args.jsonFile.empty())
	{
		return EXIT_FAILURE;
	}
	//
	//	std::ifstream f(args.jsonFile);
	//	json data = json::parse(f);
	//	data glw.Open();
	//	glw.Write(args.outFilename);
	return EXIT_SUCCESS;
}

int ExtractFile(const Arguments::Extract& args) noexcept
{
	GLWFile glw {};

	if (args.jsonFile.empty() || args.inFilename.empty())
	{
		return EXIT_FAILURE;
	}

	glw.Open(args.inFilename);
	nlohmann::ordered_json data;
	for (auto& glow : glw.GetGlows())
	{
		nlohmann::ordered_json json_emitter;
		json_emitter["size"] = glow.size;
		json_emitter["unk1"] = glow.unk1;
		json_emitter["red"] = glow.red;
		json_emitter["green"] = glow.green;
		json_emitter["blue"] = glow.blue;
		json_emitter["posX"] = glow.posX;
		json_emitter["posY"] = glow.posY;
		json_emitter["posZ"] = glow.posZ;
		json_emitter["unkX"] = glow.unkX;
		json_emitter["unkY"] = glow.unkY;
		json_emitter["unkZ"] = glow.unkZ;
		json_emitter["unkX2"] = glow.unkX2;
		json_emitter["unkY2"] = glow.unkY2;
		json_emitter["unkZ2"] = glow.unkZ2;
		json_emitter["unk14"] = glow.unk14;
		json_emitter["unk15"] = glow.unk15;
		json_emitter["unk16"] = glow.unk16;
		json_emitter["unk17"] = glow.unk17;
		json_emitter["unk18"] = glow.unk18;
		json_emitter["unk19"] = glow.unk19;
		json_emitter["unk20"] = glow.unk20;
		json_emitter["unk21"] = glow.unk21;
		json_emitter["unk22"] = glow.unk22;
		json_emitter["unk23"] = glow.unk23;
		json_emitter["unk24"] = glow.unk24;
		json_emitter["unk25"] = glow.unk25;
		json_emitter["unk26"] = glow.unk26;
		json_emitter["name"] = std::string((char*)glow.name);
		json_emitter["emitterSize"] = glow.emitterSize;
		data.push_back(json_emitter);
	}

	std::ofstream o(args.jsonFile);
	o << std::setw(4) << data << std::endl;
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
	else if (args.mode == Arguments::Mode::Extract)
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
			std::cerr << err.what() << std::endl;
			returnCode |= EXIT_FAILURE;
		}
	}

	return returnCode;
}
