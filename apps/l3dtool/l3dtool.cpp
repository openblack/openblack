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

#include <l3d_file.h>

#include <cstdlib>
#include <cxxopts.hpp>
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

int PrintHeader(openblack::l3d::L3DFile& l3d)
{
	auto& header = l3d.GetHeader();

	std::printf("file: %s\n", l3d.GetFilename().c_str());
	std::printf("magic: %s\n", std::string((char*)&header.magic, sizeof(header.magic)).c_str());
	std::printf("flags: 0x%08X\n", header.flags);
	std::printf("size: %u\n", header.size);
	std::printf("submesh count: %u\n", header.submeshCount);
	std::printf("submesh start offset: 0x%08X\n", header.submeshOffsetsOffset);
	std::printf("unknown 32 bytes: ");
	for (std::size_t i = 0; i < sizeof(header.padding); i += 4)
	{
		std::printf("%08X", *(uint32_t*)&header.padding[i]);
	}
	std::printf("\n");
	std::printf("unknown offset: 0x%08X\n", header.anotherOffset);
	std::printf("skin count: %u\n", header.skinCount);
	std::printf("skin start offset: 0x%08X\n", header.skinOffsetsOffset);
	std::printf("point count: %u\n", header.pointCount);
	std::printf("point start offset: 0x%08X\n", header.pointOffsetsOffset);
	std::printf("extra start offset: 0x%08X\n", header.extraDataOffset);
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintMeshHeaders(openblack::l3d::L3DFile& l3d)
{
	auto& meshHeaders = l3d.GetSubmeshHeaders();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	uint32_t i = 0;
	for (auto& header : meshHeaders)
	{
		std::printf("mesh #%u\n", ++i);
		std::printf("flags: 0x%08X\n", header.flags);
		std::printf("primitive count: %u\n", header.numPrimitives);
		std::printf("primitives start offset: 0x%08X\n", header.primitivesOffset);
		std::printf("bone count: %u\n", header.numBones);
		std::printf("bones start offset: 0x%08X\n", header.bonesOffset);
		std::printf("\n");
	}

	return EXIT_SUCCESS;
}

int PrintSkins(openblack::l3d::L3DFile& l3d)
{
	auto& skins = l3d.GetSkins();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	for (auto& skin : skins)
	{
		std::printf("skin id: 0x%X\n", skin.id);
		std::printf("data:\n");
		constexpr uint16_t subsample = 8;
		constexpr uint16_t magnitude = (subsample * subsample) / 16;
		for (uint16_t y = 0; y < openblack::l3d::L3DFile::L3DTexture::height / subsample; ++y)
		{
			for (uint16_t x = 0; x < openblack::l3d::L3DFile::L3DTexture::width / subsample; ++x)
			{
				uint32_t red = 0;
				uint32_t green = 0;
				uint32_t blue = 0;
				for (uint16_t j = 0; j < subsample; ++j)
				{
					for (uint16_t i = 0; i < subsample; ++i)
					{
						auto& color = skin.texels[x * subsample + i + (y * subsample + j) * openblack::l3d::L3DFile::L3DTexture::width];
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

	return EXIT_SUCCESS;
}

int PrintPoints(openblack::l3d::L3DFile& l3d)
{
	auto& points = l3d.GetPoints();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	for (auto& point : points) { std::printf("(%.3f, %.3f, %.3f)\n", point.x, point.y, point.z); }

	return EXIT_SUCCESS;
}

int PrintPrimitiveHeaders(openblack::l3d::L3DFile& l3d)
{
	auto& primitiveHeaders = l3d.GetPrimitiveHeaders();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	uint32_t i = 0;
	for (auto& header : primitiveHeaders)
	{
		std::printf("primitive #%u\n", ++i);
		std::printf("unknown_1: 0x%08X\n", header.unknown_1);
		std::printf("unknown_2: 0x%08X\n", header.unknown_2);
		std::printf("skinID: 0x%08X\n", header.skinID);
		std::printf("unknown_2: 0x%08X\n", header.unknown_3);
		std::printf("vertex count: %u\n", header.numVertices);
		std::printf("vertex start offset: 0x%08X\n", header.verticesOffset);
		std::printf("triangle count: %u\n", header.numTriangles);
		std::printf("triangle start offset: 0x%08X\n", header.trianglesOffset);
		std::printf("bone vertex look-up table size: %u\n", header.boneVertLUTSize);
		std::printf("bone vertex look-up table start offset: 0x%08X\n", header.boneVertLUTOffset);
		std::printf("vertex blend count: %u\n", header.numVertexBlends);
		std::printf("vertex blend start offset: 0x%08X\n", header.vertexBlendsOffset);
		std::printf("\n");
	}

	return EXIT_SUCCESS;
}

int PrintBones(openblack::l3d::L3DFile& l3d)
{
	auto bones = l3d.GetBones();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	uint32_t i = 0;
	for (auto& bone : bones)
	{
		std::printf("bone #%u\n", i++);
		if (bone.parent == std::numeric_limits<uint32_t>::max())
		{
			std::printf("parent: None\n");
		}
		else
		{
			std::printf("parent: %X\n", bone.parent);
		}
		if (bone.firstChild == std::numeric_limits<uint32_t>::max())
		{
			std::printf("first child: None\n");
		}
		else
		{
			std::printf("first child: %X\n", bone.firstChild);
		}
		if (bone.rightSibling == std::numeric_limits<uint32_t>::max())
		{
			std::printf("right sibling: None\n");
		}
		else
		{
			std::printf("right sibling: %X\n", bone.rightSibling);
		}
		std::printf("rotation:\n");
		for (uint8_t j = 0; j < 3; ++j)
		{
			std::printf("     [%6.3f %6.3f %6.3f]\n", bone.orientation[j * 3], bone.orientation[j * 3 + 1], bone.orientation[j * 3 + 2]);
		}
		std::printf("position: (%5.1f %5.1f %5.1f)\n", bone.position.x, bone.position.y, bone.position.z);
		std::printf("\n");
	}

	return EXIT_SUCCESS;
}

int PrintVertices(openblack::l3d::L3DFile& l3d)
{
	auto& vertices = l3d.GetVertices();
	std::printf("file: %s\n", l3d.GetFilename().c_str());
	std::printf("|     position     |   uv coord   |        normal        |\n");
	std::printf("|------------------|--------------|----------------------|\n");

	for (auto& vertex : vertices)
	{
		std::printf("|%5.1f %5.1f %5.1f |%6.3f %6.3f | %6.3f %6.3f %6.3f |\n", vertex.position.x, vertex.position.y,
		            vertex.position.z, vertex.texCoord.x, vertex.texCoord.y, vertex.normal.x, vertex.normal.y, vertex.normal.z);
	}

	return EXIT_SUCCESS;
}

int PrintIndices(openblack::l3d::L3DFile& l3d)
{
	constexpr uint8_t indexPerLine = 12;
	auto& indices = l3d.GetIndices();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	for (std::size_t i = 0; i < indices.size(); ++i)
	{
		std::printf("%s%4u", i == 0 ? "" : ((i % indexPerLine != 0) ? " " : "\n"), indices[i]);
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintLookUpTables(openblack::l3d::L3DFile& l3d)
{
	auto& lookUpTable = l3d.GetLookUpTableData();
	std::printf("file: %s\n", l3d.GetFilename().c_str());
	return PrintRawBytes(lookUpTable.data(), lookUpTable.size() * sizeof(lookUpTable[0]));
}

int PrintBlendValues(openblack::l3d::L3DFile& l3d)
{
	auto& blendValues = l3d.GetLookUpTableData();
	std::printf("file: %s\n", l3d.GetFilename().c_str());
	return PrintRawBytes(blendValues.data(), blendValues.size() * sizeof(blendValues[0]));
}

struct Arguments
{
	enum class Mode
	{
		Header,
		MeshHeader,
		Skin,
		Point,
		PrimitiveHeader,
		Bones,
		Vertices,
		Indices,
		LookUpTables,
		VertexBlendValues,
	};
	std::vector<std::string> filenames;
	Mode mode;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("l3dtool", "Inspect and extract files from LionHead L3D files.");

	options.add_options()
		("h,help", "Display this help message.")
		("H,header", "Print Header Contents.", cxxopts::value<std::vector<std::string>>())
		("m,mesh-header", "Print Mesh Headers.", cxxopts::value<std::vector<std::string>>())
		("s,skins", "Print Skins.", cxxopts::value<std::vector<std::string>>())
		("p,points", "Print Points.", cxxopts::value<std::vector<std::string>>())
		("P,primitive-header", "Print Primitive Headers.", cxxopts::value<std::vector<std::string>>())
		("b,bones", "Print Bones.", cxxopts::value<std::vector<std::string>>())
		("V,vertices", "Print Vertices.", cxxopts::value<std::vector<std::string>>())
		("I,indices", "Print Indices.", cxxopts::value<std::vector<std::string>>())
		("L,look-up-tables", "Print Look Up Table Data.", cxxopts::value<std::vector<std::string>>())
		("B,vertex-blend-values", "Print Vertex Blend Values.", cxxopts::value<std::vector<std::string>>())
	;

	try
	{
		auto result = options.parse(argc, argv);
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
		if (result["mesh-header"].count() > 0)
		{
			args.mode = Arguments::Mode::MeshHeader;
			args.filenames = result["mesh-header"].as<std::vector<std::string>>();
			return true;
		}
		if (result["skins"].count() > 0)
		{
			args.mode = Arguments::Mode::Skin;
			args.filenames = result["skins"].as<std::vector<std::string>>();
			return true;
		}
		if (result["points"].count() > 0)
		{
			args.mode = Arguments::Mode::Point;
			args.filenames = result["points"].as<std::vector<std::string>>();
			return true;
		}
		if (result["primitive-header"].count() > 0)
		{
			args.mode = Arguments::Mode::PrimitiveHeader;
			args.filenames = result["primitive-header"].as<std::vector<std::string>>();
			return true;
		}
		if (result["bones"].count() > 0)
		{
			args.mode = Arguments::Mode::Bones;
			args.filenames = result["bones"].as<std::vector<std::string>>();
			return true;
		}
		if (result["vertices"].count() > 0)
		{
			args.mode = Arguments::Mode::Vertices;
			args.filenames = result["vertices"].as<std::vector<std::string>>();
			return true;
		}
		if (result["indices"].count() > 0)
		{
			args.mode = Arguments::Mode::Indices;
			args.filenames = result["indices"].as<std::vector<std::string>>();
			return true;
		}
		if (result["look-up-tables"].count() > 0)
		{
			args.mode = Arguments::Mode::LookUpTables;
			args.filenames = result["look-up-tables"].as<std::vector<std::string>>();
			return true;
		}
		if (result["vertex-blend-values"].count() > 0)
		{
			args.mode = Arguments::Mode::VertexBlendValues;
			args.filenames = result["vertex-blend-values"].as<std::vector<std::string>>();
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
		openblack::l3d::L3DFile l3d;
		try
		{
			// Open file
			l3d.Open(filename);

			switch (args.mode)
			{
			case Arguments::Mode::Header:
				return_code |= PrintHeader(l3d);
				break;
			case Arguments::Mode::MeshHeader:
				return_code |= PrintMeshHeaders(l3d);
				break;
			case Arguments::Mode::Skin:
				return_code |= PrintSkins(l3d);
				break;
			case Arguments::Mode::Point:
				return_code |= PrintPoints(l3d);
				break;
			case Arguments::Mode::PrimitiveHeader:
				return_code |= PrintPrimitiveHeaders(l3d);
				break;
			case Arguments::Mode::Bones:
				return_code |= PrintBones(l3d);
				break;
			case Arguments::Mode::Vertices:
				return_code |= PrintVertices(l3d);
				break;
			case Arguments::Mode::Indices:
				return_code |= PrintIndices(l3d);
				break;
			case Arguments::Mode::LookUpTables:
				return_code |= PrintLookUpTables(l3d);
				break;
			case Arguments::Mode::VertexBlendValues:
				return_code |= PrintBlendValues(l3d);
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
