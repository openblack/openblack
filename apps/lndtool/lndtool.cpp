/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <cstdlib>

#include <fstream>
#include <string>

#include <LNDFile.h>
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

int PrintHeader(openblack::lnd::LNDFile& lnd)
{
	auto& header = lnd.GetHeader();

	std::printf("file: %s\n", lnd.GetFilename().c_str());
	std::printf("block count: %u\n", header.blockCount);
	std::printf("block index look-up table:\n");
	PrintRawBytes(header.lookUpTable.data(), sizeof(header.lookUpTable[0]) * header.lookUpTable.size());
	std::printf("material count: %u\n", header.materialCount);
	std::printf("country count: %u\n", header.countryCount);
	std::printf("block size: %u\n", header.blockSize);
	std::printf("material size: %u\n", header.materialSize);
	std::printf("country size: %u\n", header.countrySize);
	std::printf("low resolution texture count: %u\n", header.lowResolutionCount);
	// Not part of the header, but until these bytes are accounted, they shall
	// be displayed
	std::printf("unaccounted bytes: %zu\n", lnd.GetUnaccounted().size());
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintLowRes(openblack::lnd::LNDFile& lnd)
{
	auto& textures = lnd.GetLowResolutionTextures();

	std::printf("file: %s\n", lnd.GetFilename().c_str());
	uint32_t i = 0;
	for (auto& texture : textures)
	{
		std::printf("texture #%u:\n", i++);
		std::printf("unknown header:\n");
		PrintRawBytes(texture.header.unknown.data(), sizeof(texture.header.unknown[0]) * texture.header.unknown.size());
		std::printf("texel data size: %u\n", texture.header.size);
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintBlocks(openblack::lnd::LNDFile& lnd)
{
	auto& blocks = lnd.GetBlocks();

	std::printf("file: %s\n", lnd.GetFilename().c_str());
	uint32_t i = 0;
	for (auto& block : blocks)
	{
		std::printf("block #%u:\n", i++);
		std::printf("cells:\n");
		uint32_t j = 0;
		auto flagToStr = [](const openblack::lnd::LNDCell::Properties& properties) {
			std::string ret;
			if (properties.hasWater)
			{
				if (!ret.empty())
				{
					ret += "|";
				}
				ret += "hasWater";
			}
			if (properties.coastLine)
			{
				if (!ret.empty())
				{
					ret += "|";
				}
				ret += "coastLine";
			}
			if (properties.fullWater)
			{
				if (!ret.empty())
				{
					ret += "|";
				}
				ret += "fullWater";
			}
			if (properties.split)
			{
				if (!ret.empty())
				{
					ret += "|";
				}
				ret += "split";
			}
			return ret;
		};
		for (auto& cell : block.cells)
		{
			std::printf("    %u: r %u, g %u, b %u, luminosity %u, altitude %u, saveColor %u, country %u properties %s "
			            "flags 0x%02X\n",
			            j++, cell.r, cell.g, cell.b, cell.luminosity, cell.altitude, cell.saveColor, cell.properties.country,
			            flagToStr(cell.properties).c_str(), cell.flags);
		}
		std::printf("index: %u\n", block.index);
		std::printf("mapX: %f\n", block.mapX);
		std::printf("mapZ: %f\n", block.mapZ);
		std::printf("block: %u\n", block.blockX);
		std::printf("block: %u\n", block.blockZ);
		std::printf("clipped: 0x%X\n", block.clipped);
		std::printf("frame visibility: 0x%X\n", block.frameVisibility);
		std::printf("highest altitude: %u\n", block.highestAltitude);
		std::printf("use small bump: 0x%X\n", block.useSmallBump);
		std::printf("force low resolution texture: 0x%X\n", block.forceLowResTex);
		std::printf("mesh level of detail: %u\n", block.meshLOD);
		std::printf("mesh blending: 0x%X\n", block.meshBlending);
		std::printf("texture blending: 0x%X\n", block.textureBlend);
		std::printf("mesh level of detail type: %u\n", block.meshLODType);
		std::printf("fog: 0x%X\n", block.fog);
		std::printf("texPointer: 0x%X\n", block.texPointer);
		std::printf("matPointer: 0x%X\n", block.matPointer);
		std::printf("draw something: 0x%X\n", block.drawSomething);
		std::printf("specMatBeforePtr: 0x%X\n", block.specMatBeforePtr);
		std::printf("specMatAfterPtr: 0x%X\n", block.specMatAfterPtr);
		std::printf("transformUVBefore:\n");
		std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVBefore[0][0], block.transformUVBefore[0][1],
		            block.transformUVBefore[0][2]);
		std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVBefore[1][0], block.transformUVBefore[1][1],
		            block.transformUVBefore[1][2]);
		std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVBefore[2][0], block.transformUVBefore[2][1],
		            block.transformUVBefore[2][2]);
		std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVBefore[3][0], block.transformUVBefore[3][1],
		            block.transformUVBefore[3][2]);
		std::printf("transformUVAfter:\n");
		std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVAfter[0][0], block.transformUVAfter[0][1],
		            block.transformUVAfter[0][2]);
		std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVAfter[1][0], block.transformUVAfter[1][1],
		            block.transformUVAfter[1][2]);
		std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVAfter[2][0], block.transformUVAfter[2][1],
		            block.transformUVAfter[2][2]);
		std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVAfter[3][0], block.transformUVAfter[3][1],
		            block.transformUVAfter[3][2]);
		std::printf("nextSortingPtr: 0x%X\n", block.nextSortingPtr);
		std::printf("valueSorting: %f\n", block.valueSorting);
		std::printf("lowResTexture: %f\n", block.lowResTexture);
		std::printf("fu_lrs: %f\n", block.fu_lrs);
		std::printf("fv_lrs: %f\n", block.fv_lrs);
		std::printf("iu_lrs: %f\n", block.iu_lrs);
		std::printf("iv_lrs: %f\n", block.iv_lrs);
		std::printf("smallTextUpdated: 0x%X\n", block.smallTextUpdated);
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintCountries(openblack::lnd::LNDFile& lnd)
{
	auto& countries = lnd.GetCountries();

	std::printf("file: %s\n", lnd.GetFilename().c_str());
	uint32_t i = 0;
	for (auto& country : countries)
	{
		std::printf("country #%u:\n", i++);
		std::printf("materials:\n");
		uint32_t j = 0;
		for (auto& material : country.materials)
		{
			std::printf("    %3u: indices %3u %3u coefficient 0x%02X\n", j++, material.indices[0], material.indices[1],
			            material.coefficient);
		}
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintMaterials(openblack::lnd::LNDFile& lnd)
{
	auto& materials = lnd.GetMaterials();

	std::printf("file: %s\n", lnd.GetFilename().c_str());
	uint32_t k = 0;
	for (auto& material : materials)
	{
		std::printf("material #%u:\n", k++);
		std::printf("type: #%u:\n", material.type);
		std::printf("data:\n");
		constexpr uint16_t subsample = 8;
		constexpr uint16_t magnitude = (subsample * subsample) / 8;
		for (uint16_t y = 0; y < openblack::lnd::LNDMaterial::height / subsample; ++y)
		{
			for (uint16_t x = 0; x < openblack::lnd::LNDMaterial::width / subsample; ++x)
			{
				uint32_t red = 0;
				uint32_t green = 0;
				uint32_t blue = 0;
				for (uint16_t j = 0; j < subsample; ++j)
				{
					for (uint16_t i = 0; i < subsample; ++i)
					{
						const auto& color =
						    material.texels.at(x * subsample + i + (y * subsample + j) * openblack::lnd::LNDMaterial::width);
						red += color.R;
						green += color.G;
						blue += color.B;
					}
				}
				red /= magnitude;
				green /= magnitude;
				blue /= magnitude;
				std::printf("\x1b[48;2;%u;%u;%um  \x1b[0m", static_cast<uint8_t>(red), static_cast<uint8_t>(green),
				            static_cast<uint8_t>(blue));
			}
			std::printf("\n");
		}
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintExtra(openblack::lnd::LNDFile& lnd)
{
	auto& extra = lnd.GetExtra();

	std::printf("file: %s\n", lnd.GetFilename().c_str());
	constexpr uint16_t subsample = 8;
	constexpr uint16_t magnitude = (subsample * subsample);
	std::printf("noise:\n");
	for (uint16_t y = 0; y < openblack::lnd::LNDBumpMap::height / subsample; ++y)
	{
		for (uint16_t x = 0; x < openblack::lnd::LNDBumpMap::width / subsample; ++x)
		{
			uint32_t color = 0;
			for (uint16_t j = 0; j < subsample; ++j)
			{
				for (uint16_t i = 0; i < subsample; ++i)
				{
					color += extra.noise.texels.at(x * subsample + i + (y * subsample + j) * openblack::lnd::LNDBumpMap::width);
				}
			}
			color /= magnitude;
			std::printf("\x1b[48;2;%u;%u;%um  \x1b[0m", static_cast<uint8_t>(color), static_cast<uint8_t>(color),
			            static_cast<uint8_t>(color));
		}
		std::printf("\n");
	}
	std::printf("bump:\n");
	for (uint16_t y = 0; y < openblack::lnd::LNDBumpMap::height / subsample; ++y)
	{
		for (uint16_t x = 0; x < openblack::lnd::LNDBumpMap::width / subsample; ++x)
		{
			uint32_t color = 0;
			for (uint16_t j = 0; j < subsample; ++j)
			{
				for (uint16_t i = 0; i < subsample; ++i)
				{
					color += extra.bump.texels.at(x * subsample + i + (y * subsample + j) * openblack::lnd::LNDBumpMap::width);
				}
			}
			color /= magnitude;
			std::printf("\x1b[48;2;%u;%u;%um  \x1b[0m", static_cast<uint8_t>(color), static_cast<uint8_t>(color),
			            static_cast<uint8_t>(color));
		}
		std::printf("\n");
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintUnaccounted(openblack::lnd::LNDFile& lnd)
{
	auto& unaccounted = lnd.GetUnaccounted();

	std::printf("file: %s\n", lnd.GetFilename().c_str());
	PrintRawBytes(unaccounted.data(), unaccounted.size());
	std::printf("\n");

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode
	{
		Header,
		LowResolution,
		Blocks,
		Countries,
		Materials,
		Extra,
		Unaccounted,
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
		uint16_t terrainType;
		std::filesystem::path noiseMapFile;
		std::filesystem::path bumpMapFile;
		std::vector<std::filesystem::path> materialArray;
	} write;
};

int WriteFile(const Arguments::Write& args)
{
	openblack::lnd::LNDFile lnd {};

	openblack::lnd::LNDMaterial material;
	material.type = args.terrainType;
	for (auto& filename : args.materialArray)
	{
		std::ifstream stream(filename, std::ios::binary);
		if (!stream.is_open())
		{
			std::cerr << "Could not open " << filename << std::endl;
			return EXIT_FAILURE;
		}
		// Total file size
		size_t fsize = 0;
		if (stream.seekg(0, std::ios_base::end))
		{
			fsize = static_cast<size_t>(stream.tellg());
			stream.seekg(0);
		}
		if (fsize != sizeof(material.texels[0]) * material.texels.size())
		{
			// clang-format off
			std::cerr << "File " << filename
			          << " is not the right size to be a "
			          << openblack::lnd::LNDMaterial::width << "x"
			          << openblack::lnd::LNDMaterial::height
			          << " RGB5A1 texture: size should be "
			          << fsize << std::endl;
			// clang-format on
			return EXIT_FAILURE;
		}
		stream.read(reinterpret_cast<char*>(material.texels.data()), fsize);
		lnd.AddMaterial(material);
	}

	openblack::lnd::LNDBumpMap map;
	{
		std::ifstream stream(args.noiseMapFile, std::ios::binary);
		if (!stream.is_open())
		{
			std::cerr << "Could not open " << args.noiseMapFile << std::endl;
			return EXIT_FAILURE;
		}
		// Total file size
		size_t fsize = 0;
		if (stream.seekg(0, std::ios_base::end))
		{
			fsize = static_cast<size_t>(stream.tellg());
			stream.seekg(0);
		}
		if (fsize != sizeof(map.texels[0]) * map.texels.size())
		{
			// clang-format off
			std::cerr << "File " << args.noiseMapFile
			          << " is not the right size to be a "
			          << openblack::lnd::LNDBumpMap::width << "x"
			          << openblack::lnd::LNDBumpMap::height
			          << " R8 texture: size should be "
			          << fsize << std::endl;
			// clang-format on
			return EXIT_FAILURE;
		}
		stream.read(reinterpret_cast<char*>(map.texels.data()), fsize);
		lnd.AddNoiseMap(map);
	}
	{
		std::ifstream stream(args.bumpMapFile, std::ios::binary);
		if (!stream.is_open())
		{
			std::cerr << "Could not open " << args.bumpMapFile << std::endl;
			return EXIT_FAILURE;
		}
		// Total file size
		size_t fsize = 0;
		if (stream.seekg(0, std::ios_base::end))
		{
			fsize = static_cast<size_t>(stream.tellg());
			stream.seekg(0);
		}
		if (fsize != sizeof(map.texels[0]) * map.texels.size())
		{
			// clang-format off
			std::cerr << "File " << args.bumpMapFile
			          << " is not the right size to be a "
			          << openblack::lnd::LNDBumpMap::width << "x"
			          << openblack::lnd::LNDBumpMap::height
			          << " R8 texture: size should be "
			          << fsize << std::endl;
			// clang-format on
			return EXIT_FAILURE;
		}
		stream.read(reinterpret_cast<char*>(map.texels.data()), fsize);
		lnd.AddBumpMap(map);
	}

	lnd.Write(args.outFilename);

	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("lndtool", "Inspect and extract files from LionHead LND files.");

	// clang-format off
	options.add_options()
		("h,help", "Display this help message.")
		("subcommand", "Subcommand.", cxxopts::value<std::string>())
	;
	options.positional_help("[read|write] [OPTION...]");
	options.add_options("read")
		("H,header", "Print Header Contents.", cxxopts::value<std::vector<std::filesystem::path>>())
		("l,low-resolution-textures", "Print Low Resolution Texture Contents.", cxxopts::value<std::vector<std::filesystem::path>>())
		("b,blocks", "Print Block Contents.", cxxopts::value<std::vector<std::filesystem::path>>())
		("c,country", "Print Country Contents.", cxxopts::value<std::vector<std::filesystem::path>>())
		("m,material", "Print Material Contents.", cxxopts::value<std::vector<std::filesystem::path>>())
		("x,extra", "Print Extra Content.", cxxopts::value<std::vector<std::filesystem::path>>())
		("u,unaccounted", "Print Unaccounted bytes Content.", cxxopts::value<std::vector<std::filesystem::path>>())
	;
	options.add_options("write")
		("o,output", "Output file (required).", cxxopts::value<std::filesystem::path>())
		("terrain-type", "Type of terrain (required).", cxxopts::value<uint16_t>())
		("noise-map", "File with R8 bytes for noise map.", cxxopts::value<std::filesystem::path>())
		("bump-map", "File with R8 bytes for bump map.", cxxopts::value<std::filesystem::path>())
		("material-array", "Files with RGB5A1 bytes for material array (comma-separated).", cxxopts::value<std::vector<std::filesystem::path>>())
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
				args.read.filenames = result["header"].as<std::vector<std::filesystem::path>>();
				return true;
			}
			if (result["low-resolution-textures"].count() > 0)
			{
				args.mode = Arguments::Mode::LowResolution;
				args.read.filenames = result["low-resolution-textures"].as<std::vector<std::filesystem::path>>();
				return true;
			}
			if (result["blocks"].count() > 0)
			{
				args.mode = Arguments::Mode::Blocks;
				args.read.filenames = result["blocks"].as<std::vector<std::filesystem::path>>();
				return true;
			}
			if (result["country"].count() > 0)
			{
				args.mode = Arguments::Mode::Countries;
				args.read.filenames = result["country"].as<std::vector<std::filesystem::path>>();
				return true;
			}
			if (result["material"].count() > 0)
			{
				args.mode = Arguments::Mode::Materials;
				args.read.filenames = result["material"].as<std::vector<std::filesystem::path>>();
				return true;
			}
			if (result["extra"].count() > 0)
			{
				args.mode = Arguments::Mode::Extra;
				args.read.filenames = result["extra"].as<std::vector<std::filesystem::path>>();
				return true;
			}
			if (result["unaccounted"].count() > 0)
			{
				args.mode = Arguments::Mode::Unaccounted;
				args.read.filenames = result["unaccounted"].as<std::vector<std::filesystem::path>>();
				return true;
			}
		}
		else if (result["subcommand"].as<std::string>() == "write")
		{
			args.write.outFilename = "";
			if (result["output"].count() > 0 && result["terrain-type"].count() > 0)
			{
				args.mode = Arguments::Mode::Write;
				args.write.outFilename = result["output"].as<std::filesystem::path>();
				args.write.terrainType = result["terrain-type"].as<uint16_t>();
				if (result["noise-map"].count() > 0)
				{
					args.write.noiseMapFile = result["noise-map"].as<std::filesystem::path>();
				}
				if (result["bump-map"].count() > 0)
				{
					args.write.bumpMapFile = result["bump-map"].as<std::filesystem::path>();
				}
				if (result["material-array"].count() > 0)
				{
					args.write.materialArray = result["material-array"].as<std::vector<std::filesystem::path>>();
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
		openblack::lnd::LNDFile lnd;
		try
		{
			// Open file
			lnd.Open(filename);

			switch (args.mode)
			{
			case Arguments::Mode::Header:
				return_code |= PrintHeader(lnd);
				break;
			case Arguments::Mode::LowResolution:
				return_code |= PrintLowRes(lnd);
				break;
			case Arguments::Mode::Blocks:
				return_code |= PrintBlocks(lnd);
				break;
			case Arguments::Mode::Countries:
				return_code |= PrintCountries(lnd);
				break;
			case Arguments::Mode::Materials:
				return_code |= PrintMaterials(lnd);
				break;
			case Arguments::Mode::Extra:
				return_code |= PrintExtra(lnd);
				break;
			case Arguments::Mode::Unaccounted:
				return_code |= PrintUnaccounted(lnd);
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
