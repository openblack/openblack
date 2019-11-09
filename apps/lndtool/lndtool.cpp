/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <lnd_file.h>

#include <cstdlib>
#include <string>
#include <cxxopts.hpp>
#include <fstream>

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
	PrintRawBytes(header.lookUpTable, sizeof(header.lookUpTable));
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
		PrintRawBytes(texture.header.unknown, sizeof(texture.header.unknown));
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
		for (auto& cell : block.cells)
		{
			std::printf("    %u: r %u, g %u, b %u, l %u, altitude %u, saveColor %u, flags 0x%02X 0x%02X\n",
			            j++, cell.r, cell.g, cell.b, cell.l, cell.altitude, cell.saveColor, cell.flags[0], cell.flags[1]);
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
		for (j = 0; j < 4; ++j)
		{
			std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVBefore[j][0], block.transformUVBefore[j][1], block.transformUVBefore[j][2]);
		}
		std::printf("transformUVAfter:\n");
		for (uint8_t j = 0; j < 4; ++j)
		{
			std::printf("     [%6.3f %6.3f %6.3f]\n", block.transformUVAfter[j][0], block.transformUVAfter[j][1], block.transformUVAfter[j][2]);
		}
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
			std::printf("    %3u: indices %3u %3u coefficient 0x%02X\n",
			            j++, material.indices[0], material.indices[1], material.coefficient);
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
		for (uint16_t y = 0; y < openblack::lnd::LNDFile::LNDMaterial::height / subsample; ++y)
		{
			for (uint16_t x = 0; x < openblack::lnd::LNDFile::LNDMaterial::width / subsample; ++x)
			{
				uint32_t red = 0;
				uint32_t green = 0;
				uint32_t blue = 0;
				for (uint16_t j = 0; j < subsample; ++j)
				{
					for (uint16_t i = 0; i < subsample; ++i)
					{
						auto& color = material.texels[x * subsample + i + (y * subsample + j) * openblack::lnd::LNDFile::LNDMaterial::width];
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
	for (uint16_t y = 0; y < openblack::lnd::LNDFile::LNDBumpMap::height / subsample; ++y)
	{
		for (uint16_t x = 0; x < openblack::lnd::LNDFile::LNDBumpMap::width / subsample; ++x)
		{
			uint32_t color = 0;
			for (uint16_t j = 0; j < subsample; ++j)
			{
				for (uint16_t i = 0; i < subsample; ++i)
				{
					color += extra.noise.textureData[x * subsample + i + (y * subsample + j) * openblack::lnd::LNDFile::LNDBumpMap::width];
				}
			}
			color /= magnitude;
			std::printf("\x1b[48;2;%u;%u;%um  \x1b[0m", static_cast<uint8_t>(color), static_cast<uint8_t>(color),
			            static_cast<uint8_t>(color));
		}
		std::printf("\n");
	}
	std::printf("bump:\n");
	for (uint16_t y = 0; y < openblack::lnd::LNDFile::LNDBumpMap::height / subsample; ++y)
	{
		for (uint16_t x = 0; x < openblack::lnd::LNDFile::LNDBumpMap::width / subsample; ++x)
		{
			uint32_t color = 0;
			for (uint16_t j = 0; j < subsample; ++j)
			{
				for (uint16_t i = 0; i < subsample; ++i)
				{
					color += extra.bump.textureData[x * subsample + i + (y * subsample + j) * openblack::lnd::LNDFile::LNDBumpMap::width];
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
	};
	std::vector<std::string> filenames;
	Mode mode;
	std::string outFilename;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("lndtool", "Inspect and extract files from LionHead LND files.");

	options.add_options()
		("h,help", "Display this help message.")
		("H,header", "Print Header Contents.", cxxopts::value<std::vector<std::string>>())
		("l,low-resolution-textures", "Print Low Resolution Texture Contents.", cxxopts::value<std::vector<std::string>>())
		("b,blocks", "Print Block Contents.", cxxopts::value<std::vector<std::string>>())
		("c,country", "Print Country Contents.", cxxopts::value<std::vector<std::string>>())
		("m,material", "Print Material Contents.", cxxopts::value<std::vector<std::string>>())
		("x,extra", "Print Extra Content.", cxxopts::value<std::vector<std::string>>())
		("u,unaccounted", "Print Unaccounted bytes Content.", cxxopts::value<std::vector<std::string>>())
	;

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
		if (result["header"].count() > 0)
		{
			args.mode = Arguments::Mode::Header;
			args.filenames = result["header"].as<std::vector<std::string>>();
			return true;
		}
		if (result["low-resolution-textures"].count() > 0)
		{
			args.mode = Arguments::Mode::LowResolution;
			args.filenames = result["low-resolution-textures"].as<std::vector<std::string>>();
			return true;
		}
		if (result["blocks"].count() > 0)
		{
			args.mode = Arguments::Mode::Blocks;
			args.filenames = result["blocks"].as<std::vector<std::string>>();
			return true;
		}
		if (result["country"].count() > 0)
		{
			args.mode = Arguments::Mode::Countries;
			args.filenames = result["country"].as<std::vector<std::string>>();
			return true;
		}
		if (result["material"].count() > 0)
		{
			args.mode = Arguments::Mode::Materials;
			args.filenames = result["material"].as<std::vector<std::string>>();
			return true;
		}
		if (result["extra"].count() > 0)
		{
			args.mode = Arguments::Mode::Extra;
			args.filenames = result["extra"].as<std::vector<std::string>>();
			return true;
		}
		if (result["unaccounted"].count() > 0)
		{
			args.mode = Arguments::Mode::Unaccounted;
			args.filenames = result["unaccounted"].as<std::vector<std::string>>();
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
