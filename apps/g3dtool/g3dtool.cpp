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

#include <g3d_file.h>

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

int ListBlocks(openblack::g3d::G3DFile& g3d)
{
	auto& blocks = g3d.GetBlocks();

	std::printf("file: %s\n", g3d.GetFilename().c_str());
	std::printf("%u blocks\n", static_cast<uint32_t>(blocks.size()));
	std::printf("%u textures\n", static_cast<uint32_t>(g3d.GetTextures().size()));
	std::printf("%u meshes\n", static_cast<uint32_t>(g3d.GetMeshes().size()));
	uint32_t i = 0;
	for (auto& [name, data] : blocks)
	{
		std::printf("%u: name \"%s\", size %u\n", ++i, name.c_str(), static_cast<uint32_t>(data.size()));
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int ViewBytes(openblack::g3d::G3DFile& g3d, const std::string& name)
{
	auto& block = g3d.GetBlock(name);

	std::printf("file: %s, block %s\n", g3d.GetFilename().c_str(), name.c_str());

	PrintRawBytes(block.data(), block.size() * sizeof(block[0]));

	return EXIT_SUCCESS;
}

int ViewInfo(openblack::g3d::G3DFile& g3d)
{
	using Lookup = openblack::g3d::G3DInfoBlockLookup;
	auto lookup = g3d.GetInfoBlockLookup();
	std::sort(lookup.begin(), lookup.end(), [](const Lookup& a, Lookup& b) { return a.blockId < b.blockId; });

	std::printf("file: %s\n", g3d.GetFilename().c_str());

	for (auto& item : lookup)
	{
		std::printf("block: %4x, unknown: %u\n", item.blockId, item.unknown);
	}

	return EXIT_SUCCESS;
}

int ViewTextures(openblack::g3d::G3DFile& g3d)
{
	auto& textures = g3d.GetTextures();

	std::printf("file: %s\n", g3d.GetFilename().c_str());

	for (auto& [name, texture] : textures)
	{
		std::printf("%s: type %u, size %u\n", name.c_str(), texture.header.type, texture.header.size);
	}

	return EXIT_SUCCESS;
}

int ViewMeshes(openblack::g3d::G3DFile& g3d)
{
	auto& meshes = g3d.GetMeshes();

	std::printf("file: %s\n", g3d.GetFilename().c_str());

	uint32_t i = 0;
	for (auto& mesh : meshes)
	{
		std::printf("mesh #%-5d size %u\n", i++, static_cast<uint32_t>(mesh.size()));
	}

	return EXIT_SUCCESS;
}

int ViewTexture(openblack::g3d::G3DFile& g3d, const std::string& name, const std::string& outFilename)
{
	auto& texture = g3d.GetTexture(name);

	std::printf("file: %s\n", g3d.GetFilename().c_str());

	std::printf("size: %u\n", static_cast<uint32_t>(texture.header.size));
	std::printf("id: %u\n", static_cast<uint32_t>(texture.header.id));
	std::printf("type: %u\n", static_cast<uint32_t>(texture.header.type));
	std::printf("dds file size: %u\n", static_cast<uint32_t>(texture.header.ddsSize));

	std::printf("dds header:\n");
	std::printf("    size: %u\n", texture.ddsHeader.size);
	std::printf("    flags: 0x%X\n", texture.ddsHeader.flags);
	std::printf("    height: %u\n", texture.ddsHeader.height);
	std::printf("    width: %u\n", texture.ddsHeader.width);
	std::printf("    pitch or linear size: %u\n", texture.ddsHeader.pitchOrLinearSize);
	std::printf("    depth: %u\n", texture.ddsHeader.depth);
	std::printf("    mipMapCount: %u\n", texture.ddsHeader.mipMapCount);
	std::printf("    format:\n");
	std::printf("        size: %u\n", texture.ddsHeader.format.size);
	std::printf("        flags: 0x%X\n", texture.ddsHeader.format.flags);
	std::printf("        fourCC: %s\n",
	            std::string(texture.ddsHeader.format.fourCC, sizeof(texture.ddsHeader.format.fourCC)).c_str());
	std::printf("        bitCount: %u\n", texture.ddsHeader.format.bitCount);
	std::printf("        r-bit mask: 0x%X\n", texture.ddsHeader.format.rBitMask);
	std::printf("        g-bit mask: 0x%X\n", texture.ddsHeader.format.gBitMask);
	std::printf("        b-bit mask: 0x%X\n", texture.ddsHeader.format.bBitMask);
	std::printf("        a-bit mask: 0x%X\n", texture.ddsHeader.format.aBitMask);
	std::printf("    capabilities:\n");
	std::printf("        [0]: 0x%X\n", texture.ddsHeader.capabilities.caps[0]);
	std::printf("        [1]: 0x%X\n", texture.ddsHeader.capabilities.caps[1]);
	std::printf("        ddx: 0x%X\n", texture.ddsHeader.capabilities.ddsx);
	std::printf("        reserved: 0x%X\n", texture.ddsHeader.capabilities.reserved);
	std::printf("    reserved: 0x%X\n", texture.ddsHeader.reserved2);

	if (!outFilename.empty())
	{
		std::ofstream output(outFilename, std::ios::binary);
		constexpr std::string_view magic = "DDS ";
		output.write(reinterpret_cast<const char*>(magic.data()), magic.size());
		output.write(reinterpret_cast<const char*>(&texture.ddsHeader), sizeof(texture.ddsHeader));
		output.write(reinterpret_cast<const char*>(texture.ddsData.data()), texture.ddsData.size());

		std::printf("\nTexture writen to %s\n", outFilename.c_str());
	}

	return EXIT_SUCCESS;
}

int ViewMesh(openblack::g3d::G3DFile& g3d, uint32_t index, const std::string& outFilename)
{
	if (index > g3d.GetMeshes().size())
	{
		return EXIT_FAILURE;
	}

	auto& mesh = g3d.GetMesh(index);

	std::printf("file: %s\n", g3d.GetFilename().c_str());
	std::printf("mesh: %u bytes\n", static_cast<uint32_t>(mesh.size()));

	if (!outFilename.empty())
	{
		std::ofstream output(outFilename, std::ios::binary);
		output.write(reinterpret_cast<const char*>(mesh.data()), mesh.size() * sizeof(mesh[0]));

		std::printf("\nL3D Mesh writen to %s\n", outFilename.c_str());
	}

	return EXIT_SUCCESS;
}

int WriteFile(const std::string& outFilename)
{
	openblack::g3d::G3DFile g3d;

	// TODO(bwrsandman): expand on this to create files with contents

	g3d.Write(outFilename);

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode
	{
		List,
		Bytes,
		Info,
		Textures,
		Meshes,
		Texture,
		Mesh,
		Write,
	};
	std::vector<std::string> filenames;
	Mode mode;
	std::string block;
	uint32_t meshId;
	std::string outFilename;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("g3dtool", "Inspect and extract files from LionHead G3D files.");

	options.add_options()
		("h,help", "Display this help message.")
		("l,list-blocks", "List block statistics.")
		("b,view-bytes", "View raw byte content of block.", cxxopts::value<std::string>())
		("i,info-block", "List INFO block statistics.")
		("M,meshes-block", "List MESHES block statistics.")
		("m,mesh", "List mesh statistics.", cxxopts::value<uint32_t>())
		("T,texture-block", "View texture block statistics.")
		("t,texture", "View texture statistics.", cxxopts::value<std::string>())
		("e,extract", "Extract contents of a block (texture) to filename.", cxxopts::value<std::string>())
		("w,write", "Create Mesh Pack.", cxxopts::value<std::string>())
		("mesh-pack-files", "G3D Mesh Pack Files.", cxxopts::value<std::vector<std::string>>())
	;
	options.parse_positional({ "mesh-pack-files" });
	options.positional_help("mesh-pack-files...");

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
		if (result["write"].count() > 0)
		{
			args.mode = Arguments::Mode::Write;
			args.outFilename = result["write"].as<std::string>();
			return true;
		}
		// Following this, all args require positional arguments
		if (result["mesh-pack-files"].count() == 0)
		{
			throw cxxopts::missing_argument_exception("mesh-pack-files");
		}
		if (result["list-blocks"].count() > 0)
		{
			args.mode = Arguments::Mode::List;
			args.filenames = result["mesh-pack-files"].as<std::vector<std::string>>();
			return true;
		}
		if (result["view-bytes"].count() > 0)
		{
			args.mode = Arguments::Mode::Bytes;
			args.filenames = result["mesh-pack-files"].as<std::vector<std::string>>();
			args.block = result["view-bytes"].as<std::string>();
			return true;
		}
		if (result["info-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Info;
			args.filenames = result["mesh-pack-files"].as<std::vector<std::string>>();
			return true;
		}
		if (result["meshes-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Meshes;
			args.filenames = result["mesh-pack-files"].as<std::vector<std::string>>();
			return true;
		}
		if (result["texture-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Textures;
			args.filenames = result["mesh-pack-files"].as<std::vector<std::string>>();
			return true;
		}
		if (result["mesh"].count() > 0)
		{
			if (result["extract"].count() > 0)
			{
				args.outFilename = result["extract"].as<std::string>();
			}
			args.mode = Arguments::Mode::Mesh;
			args.filenames = result["mesh-pack-files"].as<std::vector<std::string>>();
			args.meshId = result["mesh"].as<uint32_t>();
			return true;
		}
		if (result["texture"].count() > 0)
		{
			if (result["extract"].count() > 0)
			{
				args.outFilename = result["extract"].as<std::string>();
			}
			args.mode = Arguments::Mode::Texture;
			args.filenames = result["mesh-pack-files"].as<std::vector<std::string>>();
			args.block = result["texture"].as<std::string>();
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

	if (args.mode == Arguments::Mode::Write)
	{
		return WriteFile(args.outFilename);
	}

	for (auto& filename : args.filenames)
	{
		openblack::g3d::G3DFile g3d;
		try
		{
			// Open file
			g3d.Open(filename);

			switch (args.mode)
			{
			case Arguments::Mode::List:
				return_code |= ListBlocks(g3d);
				break;
			case Arguments::Mode::Bytes:
				return_code |= ViewBytes(g3d, args.block);
				break;
			case Arguments::Mode::Info:
				return_code |= ViewInfo(g3d);
				break;
			case Arguments::Mode::Textures:
				return_code |= ViewTextures(g3d);
				break;
			case Arguments::Mode::Meshes:
				return_code |= ViewMeshes(g3d);
				break;
			case Arguments::Mode::Texture:
				return_code |= ViewTexture(g3d, args.block, args.outFilename);
				break;
			case Arguments::Mode::Mesh:
				return_code |= ViewMesh(g3d, args.meshId, args.outFilename);
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
