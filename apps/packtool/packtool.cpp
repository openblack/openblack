/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <cassert>
#include <cstdlib>

#include <fstream>
#include <limits>
#include <string>

#include <PackFile.h>
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

int ListBlocks(openblack::pack::PackFile& pack)
{
	const auto& blocks = pack.GetBlocks();

	std::printf("file: %s\n", pack.GetFilename().c_str());
	std::printf("%u blocks\n", static_cast<uint32_t>(blocks.size()));
	std::printf("%u textures\n", static_cast<uint32_t>(pack.GetTextures().size()));
	std::printf("%u meshes\n", static_cast<uint32_t>(pack.GetMeshes().size()));
	std::printf("%u animations\n", static_cast<uint32_t>(pack.GetAnimations().size()));
	uint32_t i = 0;
	for (const auto& [name, data] : blocks)
	{
		std::printf("%u: name \"%s\", size %u\n", ++i, name.c_str(), static_cast<uint32_t>(data.size()));
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int ListBlock(openblack::pack::PackFile& pack, const std::string& name, const std::filesystem::path& outFilename)
{
	auto* outputLogStream = stdout;
	if (outFilename == std::filesystem::path("stdout"))
	{
		outputLogStream = stderr;
	}

	const auto& blocks = pack.GetBlocks();

	auto block = blocks.find(name);
	if (block == blocks.end())
	{
		std::fprintf(stderr, "no \"%s\" block in file: %s\n", name.c_str(), pack.GetFilename().c_str());
		return EXIT_FAILURE;
	}

	std::fprintf(outputLogStream, "file: %s\n", pack.GetFilename().c_str());
	std::fprintf(outputLogStream, "name \"%s\", size %u\n", name.c_str(), static_cast<uint32_t>(block->second.size()));
	std::fprintf(outputLogStream, "\n");

	if (!outFilename.empty())
	{
		if (outFilename == std::filesystem::path("stdout"))
		{
			std::cout.write(reinterpret_cast<const char*>(block->second.data()), block->second.size());
		}
		else
		{
			std::ofstream output(outFilename, std::ios::binary);
			output.write(reinterpret_cast<const char*>(block->second.data()), block->second.size());
		}

		std::fprintf(outputLogStream, "\nBlock writen to %s\n", outFilename.string().c_str());
	}

	return EXIT_SUCCESS;
}

int ViewBytes(openblack::pack::PackFile& pack, const std::string& name)
{
	const auto& block = pack.GetBlock(name);

	std::printf("file: %s, block %s\n", pack.GetFilename().c_str(), name.c_str());

	PrintRawBytes(block.data(), block.size() * sizeof(block[0]));

	return EXIT_SUCCESS;
}

int ViewInfo(openblack::pack::PackFile& pack)
{
	using Lookup = openblack::pack::InfoBlockLookup;
	auto lookup = pack.GetInfoBlockLookup();
	std::sort(lookup.begin(), lookup.end(), [](const Lookup& a, Lookup& b) { return a.blockId < b.blockId; });

	std::printf("file: %s\n", pack.GetFilename().c_str());

	for (auto& item : lookup)
	{
		std::printf("block: %4x, unknown: %u\n", item.blockId, item.unknown);
	}

	return EXIT_SUCCESS;
}

int ViewBody(openblack::pack::PackFile& pack)
{
	const auto& lookup = pack.GetBodyBlockLookup();

	std::printf("file: %s\n", pack.GetFilename().c_str());

	for (uint32_t i = 0; i < lookup.size(); ++i)
	{
		std::printf("block: Julien%-3u offset: %4x unknown: %u\n", i, lookup[i].offset, lookup[i].unknown);
	}

	return EXIT_SUCCESS;
}

int ViewTextures(openblack::pack::PackFile& pack)
{
	const auto& textures = pack.GetTextures();

	std::printf("file: %s\n", pack.GetFilename().c_str());

	for (const auto& [name, texture] : textures)
	{
		std::printf("%s: type %u, size %u\n", name.c_str(), texture.header.type, texture.header.size);
	}

	return EXIT_SUCCESS;
}

int ViewMeshes(openblack::pack::PackFile& pack)
{
	const auto& meshes = pack.GetMeshes();

	std::printf("file: %s\n", pack.GetFilename().c_str());

	uint32_t i = 0;
	for (const auto& mesh : meshes)
	{
		std::printf("mesh #%-5d size %u\n", i++, static_cast<uint32_t>(mesh.size()));
	}

	return EXIT_SUCCESS;
}

int ViewAnimations(openblack::pack::PackFile& pack)
{
	const auto& animations = pack.GetAnimations();

	std::printf("file: %s\n", pack.GetFilename().c_str());

	uint32_t i = 0;
	for (const auto& animation : animations)
	{
		std::printf("animation #%-5d %-32s size %u\n", i++, animation.data(), static_cast<uint32_t>(animation.size()));
	}

	return EXIT_SUCCESS;
}

int ViewTexture(openblack::pack::PackFile& pack, const std::string& name, const std::filesystem::path& outFilename)
{
	const auto& texture = pack.GetTexture(name);

	std::printf("file: %s\n", pack.GetFilename().c_str());

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
	std::printf("        fourCC: %c%c%c%c\n", texture.ddsHeader.format.fourCC[0], texture.ddsHeader.format.fourCC[1],
	            texture.ddsHeader.format.fourCC[2], texture.ddsHeader.format.fourCC[3]);
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

		std::printf("\nTexture writen to %s\n", outFilename.string().c_str());
	}

	return EXIT_SUCCESS;
}

int ViewMesh(openblack::pack::PackFile& pack, uint32_t index, const std::filesystem::path& outFilename)
{
	if (index > pack.GetMeshes().size())
	{
		return EXIT_FAILURE;
	}

	const auto& mesh = pack.GetMesh(index);

	std::printf("file: %s\n", pack.GetFilename().c_str());
	std::printf("mesh: %u bytes\n", static_cast<uint32_t>(mesh.size()));

	if (!outFilename.empty())
	{
		std::ofstream output(outFilename, std::ios::binary);
		output.write(reinterpret_cast<const char*>(mesh.data()), mesh.size() * sizeof(mesh[0]));

		std::printf("\nL3D Mesh writen to %s\n", outFilename.string().c_str());
	}

	return EXIT_SUCCESS;
}

int ViewAnimation(openblack::pack::PackFile& pack, uint32_t index, const std::filesystem::path& outFilename)
{
	if (index > pack.GetAnimations().size())
	{
		return EXIT_FAILURE;
	}

	const auto& animation = pack.GetAnimation(index);

	std::printf("file: %s\n", pack.GetFilename().c_str());
	std::printf("animation: %-32s %u bytes\n", animation.data(), static_cast<uint32_t>(animation.size()));

	if (!outFilename.empty())
	{
		std::ofstream output(outFilename, std::ios::binary);
		output.write(reinterpret_cast<const char*>(animation.data()), animation.size() * sizeof(animation[0]));

		std::printf("\nANM file writen to %s\n", outFilename.string().c_str());
	}

	return EXIT_SUCCESS;
}

int WriteRaw(const std::filesystem::path& outFilename, const std::vector<std::filesystem::path>& inFilenames) noexcept
{
	openblack::pack::PackFile pack;

	for (const auto& filename : inFilenames)
	{
		std::ifstream file(filename, std::ios::binary);
		if (!file.is_open())
		{
			std::fprintf(stderr, "Could not open source file \"%s\"\n", filename.string().c_str());
			return EXIT_FAILURE;
		}
		file.seekg(0, std::ios_base::end);
		const auto size = file.tellg();
		file.seekg(0, std::ios_base::beg);
		std::vector<uint8_t> data(static_cast<size_t>(size));
		try
		{
			file.read(reinterpret_cast<char*>(data.data()), data.size());
			file.close();
		}
		catch (const std::ios_base::failure& e)
		{
			std::fprintf(stderr, "I/O error while reading \"%s\": %s\n", filename.string().c_str(), e.what());
			return EXIT_FAILURE;
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			return EXIT_FAILURE;
		}

		pack.CreateRawBlock(filename.filename().string(), std::move(data));
	}

	pack.Write(outFilename);

	return EXIT_SUCCESS;
}

int WriteMeshFile(const std::filesystem::path& outFilename, const std::vector<std::filesystem::path>& inFilenames) noexcept
{
	openblack::pack::PackFile pack;

	for (const auto& filename : inFilenames)
	{
		std::vector<uint8_t> data;
		if (!filename.empty())
		{
			std::ifstream file(filename, std::ios::binary);
			if (!file.is_open())
			{
				std::fprintf(stderr, "Could not open source file \"%s\"\n", filename.string().c_str());
				return EXIT_FAILURE;
			}
			file.seekg(0, std::ios_base::end);
			const auto size = file.tellg();
			file.seekg(0, std::ios_base::beg);
			data.resize(static_cast<size_t>(size));
			try
			{
				file.read(reinterpret_cast<char*>(data.data()), data.size());
				file.close();
			}
			catch (const std::ios_base::failure& e)
			{
				std::fprintf(stderr, "I/O error while reading \"%s\": %s\n", filename.string().c_str(), e.what());
				return EXIT_FAILURE;
			}
		}
		pack.InsertMesh(data);
	}

	// TODO(bwrsandman): expand on this to create files with contents
	// CreateTextureBlocks();  // TODO(bwrsandman): Implement CreateTextureBlocks
	pack.CreateMeshBlock();
	pack.CreateInfoBlock();
	pack.Write(outFilename);

	return EXIT_SUCCESS;
}

int WriteAnimationFile(const std::filesystem::path& outFilename) noexcept
{
	openblack::pack::PackFile pack;

	// TODO(bwrsandman): expand on this to create files with contents
	pack.CreateBodyBlock();
	pack.Write(outFilename);

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode
	{
		List,
		Block,
		Bytes,
		Info,
		Textures,
		Meshes,
		Animations,
		Texture,
		Mesh,
		Animation,
		Body,
		WriteRaw,
		WriteMeshPack,
		WriteAnimationPack,
	};
	std::vector<std::filesystem::path> filenames;
	Mode mode;
	std::string block;
	uint32_t blockId;
	std::filesystem::path outFilename;
};

[[nodiscard]] std::string parseRange(std::string range, uint32_t currentSize, uint32_t& start, uint32_t& length)
{
	// Split ':' after all file seps. Ignore drives like 'C:/', should be followed by either numbers or other ':'
	auto colCount = std::count(range.begin(), range.end(), ':');
	if (colCount > 2)
	{
		throw cxxopts::option_syntax_exception("pack-files entry cannot have more than two ':' for file.l3d[[:START]:LENGTH]");
	}
	// If there are none, it's like :1
	if (colCount == 0)
	{
		range += ":1";
		colCount = 1;
	}
	// If there is only one, it's followed by length and is like :currentSize:length
	if (colCount == 1)
	{
		const auto pos = range.find(':');
		range = range.substr(0, pos + 1) + std::to_string(currentSize) + range.substr(pos);
	}
	assert(std::count(range.begin(), range.end(), ':') == 2);

	const auto index1 = range.find(':');
	const auto index2 = range.find(':', index1 + 1);

	const auto startStr = range.substr(index1 + 1, index2 - index1 - 1);
	const auto lengthStr = range.substr(index2 + 1);

	auto ul = std::strtoul(startStr.c_str(), nullptr, 0);
	if (ul == std::numeric_limits<decltype(ul)>::max())
	{
		throw cxxopts::option_syntax_exception("couldn't parse pack-file's start index as a number: " + startStr);
	}
	start = static_cast<uint32_t>(ul);

	ul = std::strtoul(lengthStr.c_str(), nullptr, 0);
	if (ul == std::numeric_limits<decltype(ul)>::max())
	{
		throw cxxopts::option_syntax_exception("couldn't parse pack-file's length as a number: " + lengthStr);
	}
	length = static_cast<uint32_t>(ul);

	return range.substr(0, index1);
}

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("packtool", "Inspect and extract files from LionHead pack files.");

	try
	{
		options.add_options()                                                                                   //
		    ("h,help", "Display this help message.")                                                            //
		    ("l,list-blocks", "List all blocks statistics.")                                                    //
		    ("b,view-bytes", "View raw byte content of block.")                                                 //
		    ("s,block", "List statistics of block.", cxxopts::value<std::string>())                             //
		    ("i,info-block", "List INFO block statistics.")                                                     //
		    ("B,body-block", "List Body block statistics.")                                                     //
		    ("M,meshes-block", "List MESHES block statistics.")                                                 //
		    ("m,mesh", "List mesh statistics.", cxxopts::value<uint32_t>())                                     //
		    ("T,texture-block", "View texture block statistics.")                                               //
		    ("t,texture", "View texture statistics.", cxxopts::value<std::string>())                            //
		    ("A,animation-block", "List animation block statistics.")                                           //
		    ("a,animation", "List animation statistics.", cxxopts::value<uint32_t>())                           //
		    ("e,extract", "Extract contents of a block to filename (use \"stdout\" for piping to other tool).", //
		     cxxopts::value<std::filesystem::path>())                                                           //
		    ("w,write-raw", "Create Raw Data Pack.", cxxopts::value<std::filesystem::path>())                   //
		    ("write-mesh", "Create Mesh Pack (file.l3d[[:START]:LENGTH]...).",                                  //
		     cxxopts::value<std::filesystem::path>())                                                           //
		    ("write-animation", "Create Mesh Pack.", cxxopts::value<std::filesystem::path>())                   //
		    ("pack-files", "Pack Files.", cxxopts::value<std::vector<std::filesystem::path>>())                 //
		    ;

		options.parse_positional({"pack-files"});
		options.positional_help("pack-files...");
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
		args.outFilename = "";
		if (result["help"].as<bool>())
		{
			std::cout << options.help() << std::endl;
			returnCode = EXIT_SUCCESS;
			return false;
		}
		if (result["write-animation"].count() > 0)
		{
			args.mode = Arguments::Mode::WriteAnimationPack;
			args.outFilename = result["write-animation"].as<std::filesystem::path>();
			return true;
		}
		// Following this, all args require positional arguments
		if (result["pack-files"].count() == 0)
		{
			throw cxxopts::missing_argument_exception("pack-files");
		}
		if (result["write-mesh"].count() > 0)
		{
			args.mode = Arguments::Mode::WriteMeshPack;
			args.outFilename = result["write-mesh"].as<std::filesystem::path>();
			const auto ranges = result["pack-files"].as<std::vector<std::filesystem::path>>();
			// Get size of expanded out filename
			uint32_t currentSize = 0;
			for (const auto& range : ranges)
			{
				uint32_t start;
				uint32_t length;
				const auto filename = parseRange(range.filename().string(), currentSize, start, length);
				currentSize = std::max(start + length, currentSize);
			}
			// Expand out filename
			std::vector<std::filesystem::path> expandedOutFilename;
			expandedOutFilename.resize(static_cast<std::vector<std::filesystem::path>::size_type>(currentSize));
			currentSize = 0;
			for ([[maybe_unused]] const auto& range : result["pack-files"].as<std::vector<std::filesystem::path>>())
			{
				uint32_t start;
				uint32_t length;
				const auto filename = parseRange(range.filename().string(), currentSize, start, length);
				const auto parent = range.parent_path();
				for (uint32_t i = 0; i < length; ++i)
				{
					expandedOutFilename[start + i] = parent / filename;
				}
			}
			args.filenames = expandedOutFilename;
			return true;
		}
		if (result["list-blocks"].count() > 0)
		{
			args.mode = Arguments::Mode::List;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["view-bytes"].count() > 0)
		{
			args.mode = Arguments::Mode::Bytes;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			args.block = result["view-bytes"].as<std::string>();
			return true;
		}
		if (result["info-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Info;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["body-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Body;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["animation-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Animations;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["meshes-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Meshes;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["texture-block"].count() > 0)
		{
			args.mode = Arguments::Mode::Textures;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["block"].count() > 0)
		{
			if (result["extract"].count() > 0)
			{
				args.outFilename = result["extract"].as<std::filesystem::path>();
			}
			args.mode = Arguments::Mode::Block;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			args.block = result["block"].as<std::string>();
			return true;
		}
		if (result["write-raw"].count() > 0)
		{
			args.outFilename = result["write-raw"].as<std::filesystem::path>();
			args.mode = Arguments::Mode::WriteRaw;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["animation"].count() > 0)
		{
			if (result["extract"].count() > 0)
			{
				args.outFilename = result["extract"].as<std::filesystem::path>();
			}
			args.mode = Arguments::Mode::Animation;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			args.blockId = result["animation"].as<uint32_t>();
			return true;
		}
		if (result["mesh"].count() > 0)
		{
			if (result["extract"].count() > 0)
			{
				args.outFilename = result["extract"].as<std::filesystem::path>();
			}
			args.mode = Arguments::Mode::Mesh;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			args.blockId = result["mesh"].as<uint32_t>();
			return true;
		}
		if (result["texture"].count() > 0)
		{
			if (result["extract"].count() > 0)
			{
				args.outFilename = result["extract"].as<std::filesystem::path>();
			}
			args.mode = Arguments::Mode::Texture;
			args.filenames = result["pack-files"].as<std::vector<std::filesystem::path>>();
			args.block = result["texture"].as<std::string>();
			return true;
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

	if (args.mode == Arguments::Mode::WriteRaw)
	{
		return WriteRaw(args.outFilename, args.filenames);
	}

	if (args.mode == Arguments::Mode::WriteMeshPack)
	{
		return WriteMeshFile(args.outFilename, args.filenames);
	}

	if (args.mode == Arguments::Mode::WriteAnimationPack)
	{
		return WriteAnimationFile(args.outFilename);
	}

	for (auto& filename : args.filenames)
	{
		openblack::pack::PackFile pack;
		try
		{
			// Open file
			pack.Open(filename);

			switch (args.mode)
			{
			case Arguments::Mode::List:
				returnCode |= ListBlocks(pack);
				break;
			case Arguments::Mode::Block:
				returnCode |= ListBlock(pack, args.block, args.outFilename);
				break;
			case Arguments::Mode::Bytes:
				returnCode |= ViewBytes(pack, args.block);
				break;
			case Arguments::Mode::Info:
				returnCode |= ViewInfo(pack);
				break;
			case Arguments::Mode::Body:
				returnCode |= ViewBody(pack);
				break;
			case Arguments::Mode::Textures:
				returnCode |= ViewTextures(pack);
				break;
			case Arguments::Mode::Meshes:
				returnCode |= ViewMeshes(pack);
				break;
			case Arguments::Mode::Animations:
				returnCode |= ViewAnimations(pack);
				break;
			case Arguments::Mode::Texture:
				returnCode |= ViewTexture(pack, args.block, args.outFilename);
				break;
			case Arguments::Mode::Mesh:
				returnCode |= ViewMesh(pack, args.blockId, args.outFilename);
				break;
			case Arguments::Mode::Animation:
				returnCode |= ViewAnimation(pack, args.blockId, args.outFilename);
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
