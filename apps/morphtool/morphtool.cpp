/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <MorphFile.h>

#include <cstdlib>
#include <string>

#include <cxxopts.hpp>
#include <fstream>

int ListDetails(openblack::morph::MorphFile& morph)
{
	auto& header = morph.GetHeader();

	std::printf("file: %s\n", morph.GetFilename().c_str());

	std::printf("base mesh: \"%s\"\n", header.base_mesh_name.data());
	uint32_t num_variants = 0;
	for (const auto& meshes : header.variant_mesh_names)
	{
		if (std::strlen(meshes.data()) > 0)
		{
			++num_variants;
		}
	}
	std::printf("%u variant meshes:", num_variants);
	for (const auto& meshes : header.variant_mesh_names)
	{
		if (std::strlen(meshes.data()) > 0)
		{
			std::printf(" \"%s\"", meshes.data());
		}
	}
	std::printf("\n");

	size_t num_animation_sets = 0;
	auto& specs = morph.GetAnimationSpecs();
	for (auto& anim_set : specs.animation_sets)
	{
		num_animation_sets += anim_set.animations.size();
	}
	std::printf("%zu animations from %zu categories:\n", num_animation_sets, specs.animation_sets.size());
	std::printf("%u variant animation sets\n", std::min(4u, num_variants));

	std::printf("%zu base animations\n", morph.GetBaseAnimationSet().size());
	for (uint8_t i = 0; i < std::min(4u, num_variants); ++i)
	{
		auto& animation_set = morph.GetVariantAnimationSet(i);
		std::printf("%zu animations for \"%s\"\n", animation_set.size(), header.variant_mesh_names[i].data());
	}

	std::printf("%zu hair groups\n", morph.GetHairGroups().size());

	auto& extra_data = morph.GetExtraData();

	size_t extra_data_total = 0;
	for (auto& data : extra_data)
	{
		extra_data_total += data.size();
	}

	std::printf("%zu total extra data segments\n", extra_data_total);
	std::printf("%zu extra data groups\n", extra_data.size());

	return EXIT_SUCCESS;
}

int PrintHeader(openblack::morph::MorphFile& morph)
{
	auto& header = morph.GetHeader();

	std::printf("file: %s\n", morph.GetFilename().c_str());

	std::printf("unknown_0x0: 0x%08X\n", header.unknown_0x0);
	std::printf("spec_file_version: %u\n", header.spec_file_version);
	std::printf("binary_version: %u\n", header.binary_version);
	std::printf("base_mesh_name: %s\n", header.base_mesh_name.data());
	for (uint8_t i = 0; i < 6; ++i)
	{
		std::printf("variant_mesh_names[%u]: %s\n", i, header.variant_mesh_names[i].data());
	}

	return EXIT_SUCCESS;
}

int PrintSpecs(openblack::morph::MorphFile& morph)
{
	auto& specs = morph.GetAnimationSpecs();

	std::printf("file: %s\n", morph.GetFilename().c_str());

	std::printf("specs path: %s\n", specs.path.c_str());
	std::printf("specs version: %u\n", specs.version);

	size_t j = 0;
	for (auto& anim_set : specs.animation_sets)
	{
		j += anim_set.animations.size();
	}
	std::printf("%zu animations from %zu categories:\n", j, specs.animation_sets.size());

	uint32_t i = 0;
	j = 0;
	for (auto& anim_set : specs.animation_sets)
	{
		std::printf("[%u] category \"%s\" with %zu animations:\n", i, anim_set.name.c_str(), anim_set.animations.size());
		i++;
		for (auto& desc : anim_set.animations)
		{
			std::printf("\t[%3zu] type: %c, \"%s\":\n", j, static_cast<char>(desc.type), desc.name.c_str());
			j++;
		}
	}

	return EXIT_SUCCESS;
}

void PrintAnimation(const openblack::morph::Animation& animation)
{
	std::printf("\tHeader:\n");
	std::printf("\t\tunknown_0x0: 0x%08X\n", animation.header.unknown_0x0);
	std::printf("\t\tunknown_0x4: 0x%08X\n", animation.header.unknown_0x4);
	std::printf("\t\tunknown_0x8: %f\n", animation.header.unknown_0x8);
	std::printf("\t\tunknown_0xc: %f\n", animation.header.unknown_0xc);
	std::printf("\t\tunknown_0x10: %f\n", animation.header.unknown_0x10);
	std::printf("\t\tunknown_0x14: %f\n", animation.header.unknown_0x14);
	std::printf("\t\tunknown_0x18: %f\n", animation.header.unknown_0x18);
	std::printf("\t\tframe_count: %u\n", animation.header.frame_count);
	std::printf("\t\tmesh_bone_count: %u\n", animation.header.mesh_bone_count);
	std::printf("\t\trotated_joint_count: %u\n", animation.header.rotated_joint_count);
	std::printf("\t\ttranslated_joint_count: %u\n", animation.header.translated_joint_count);

	std::printf("\trotated_joint_indices: [");
	for (uint32_t index : animation.rotated_joint_indices)
	{
		std::printf("%u, ", index);
	}
	std::printf("]\n");

	std::printf("\ttranslated_joint_indices: [");
	for (uint32_t index : animation.translated_joint_indices)
	{
		std::printf("%u, ", index);
	}
	std::printf("]\n");

	std::printf("\tKeyframes:\n");

	uint32_t i = 0;
	for (auto& frame : animation.keyframes)
	{
		std::printf("\t\t[%2u]\n", i);
		std::printf("\t\teuler_angles: [");
		for (auto angles : frame.euler_angles)
		{
			std::printf("(%f, %f, %f), ", angles[0], angles[1], angles[2]);
		}
		std::printf("]\n");
		std::printf("\t\ttranslations: [");
		for (auto translation : frame.translations)
		{
			std::printf("(%f, %f, %f), ", translation[0], translation[1], translation[2]);
		}
		std::printf("]\n");
		++i;
	}
}

int ShowBaseAnimationSet(openblack::morph::MorphFile& morph)
{
	auto& animation_set = morph.GetBaseAnimationSet();

	std::printf("file: %s\n", morph.GetFilename().c_str());
	std::printf("%zu base animations\n", animation_set.size());

	uint32_t i = 0;
	for (const auto& animation : animation_set)
	{
		std::printf("\t[%2u]\n", i);
		PrintAnimation(animation);
		++i;
	}

	return EXIT_SUCCESS;
}

int ShowVariantAnimationSets(openblack::morph::MorphFile& morph)
{
	auto& header = morph.GetHeader();

	std::printf("file: %s\n", morph.GetFilename().c_str());

	uint8_t num_variants = 0;
	for (uint8_t i = 0; i < 4; ++i)
	{
		if (std::strlen(header.variant_mesh_names[i].data()) > 0 && !morph.GetVariantAnimationSet(i).empty())
		{
			++num_variants;
		}
	}
	std::printf("%u animation set variants\n", num_variants);

	for (uint8_t i = 0; i < num_variants; ++i)
	{
		auto& animation_set = morph.GetVariantAnimationSet(i);
		std::printf("%zu animations for \"%s\"\n", animation_set.size(), header.variant_mesh_names[i].data());
		uint32_t j = 0;
		for (const auto& animation : animation_set)
		{
			std::printf("\t[%2u]\n", j);
			PrintAnimation(animation);
			++j;
		}
	}
	return EXIT_SUCCESS;
}

int ShowHairGroups(openblack::morph::MorphFile& morph)
{
	auto& hair_groups = morph.GetHairGroups();

	std::printf("file: %s\n", morph.GetFilename().c_str());
	std::printf("%zu hair groups\n", hair_groups.size());

	uint32_t i = 0;
	for (const auto& group : hair_groups)
	{
		std::printf("[%2u]\n", i);
		std::printf("\tHeader:\n");
		std::printf("\t\tunknown_0x0: 0x%08X\n", group.header.unknown_0x0);
		std::printf("\t\thair_count: %d\n", group.header.hair_count);
		std::printf("\t\tunknown_0x8: 0x%08X\n", group.header.unknown_0x8);
		std::printf("\t\tunknown_0xc: 0x%08X\n", group.header.unknown_0xc);
		for (size_t j = 0; j < group.header.unknown_0x10.size(); ++j)
		{
			std::printf("\t\tunknown_0x10[%2zu]:\n", j);
			std::printf("\t\t\tunknown_0x0: 0x%08X\n", group.header.unknown_0x10[j].unknown_0x0);
			std::printf("\t\t\tunknown_0x4: 0x%08X\n", group.header.unknown_0x10[j].unknown_0x4);
			std::printf("\t\t\tunknown_0x8: 0x%08X\n", group.header.unknown_0x10[j].unknown_0x8);
			std::printf("\t\t\tunknown_0xc: %f\n", group.header.unknown_0x10[j].unknown_0xc);
			std::printf("\t\t\tunknown_0x10: %f\n", group.header.unknown_0x10[j].unknown_0x10);
			std::printf("\t\t\tunknown_0x14: %f\n", group.header.unknown_0x10[j].unknown_0x14);
			std::printf("\t\t\tunknown_0x18: %f\n", group.header.unknown_0x10[j].unknown_0x18);
		}
		std::printf("\tHairs:\n");
		for (size_t j = 0; j < group.hairs.size(); ++j)
		{
			std::printf("\t[%2zu]\n", j);
			std::printf("\t\tunknown_0x0: 0x%08X\n", group.hairs[j].unknown_0x0);

			std::printf("\t\tintersection:\n");
			std::printf("\t\t\tunknown_0x0: 0x%08X\n", group.hairs[j].intersection.unknown_0x0);
			std::printf("\t\t\tunknown_0x4: 0x%08X\n", group.hairs[j].intersection.unknown_0x4);
			std::printf("\t\t\tunknown_0x8: 0x%08X\n", group.hairs[j].intersection.unknown_0x8);
			std::printf("\t\t\tunknown_0xc: 0x%08X\n", group.hairs[j].intersection.unknown_0xc);
			std::printf("\t\t\tunknown_0x10: 0x%08X\n", group.hairs[j].intersection.unknown_0x10);
			std::printf("\t\t\tunknown_0x14: 0x%08X\n", group.hairs[j].intersection.unknown_0x14);
			std::printf("\t\t\tunknown_0x18: 0x%08X\n", group.hairs[j].intersection.unknown_0x18);
			std::printf("\t\t\tunknown_0x1c: %f\n", group.hairs[j].intersection.unknown_0x1c);
			std::printf("\t\t\tunknown_0x20: %f\n", group.hairs[j].intersection.unknown_0x20);

			std::printf("\t\txs: [%f, %f, %f]\n", group.hairs[j].xs[0], group.hairs[j].xs[1], group.hairs[j].xs[2]);
			std::printf("\t\tys: [%f, %f, %f]\n", group.hairs[j].ys[0], group.hairs[j].ys[1], group.hairs[j].ys[2]);
			std::printf("\t\tzs: [%f, %f, %f]\n", group.hairs[j].zs[0], group.hairs[j].zs[1], group.hairs[j].zs[2]);
		}
		++i;
	}

	return EXIT_SUCCESS;
}

int ShowExtraData(openblack::morph::MorphFile& morph)
{
	auto& extra_data = morph.GetExtraData();
	auto& specs = morph.GetAnimationSpecs();

	size_t extra_data_total = 0;
	for (auto& data : extra_data)
	{
		extra_data_total += data.size();
	}

	std::printf("file: %s\n", morph.GetFilename().c_str());
	std::printf("%zu total extra data segments\n", extra_data_total);
	std::printf("%zu extra data groups (one per animation)\n", extra_data.size());

	uint32_t i = 0;
	uint32_t category_index = 0;
	size_t animation_index = 0;
	for (auto& list : extra_data)
	{
		uint32_t j = 0;
		std::printf("[%2u]: %s (%s)\n", i, specs.animation_sets[category_index].animations[animation_index].name.c_str(),
		            specs.animation_sets[category_index].name.c_str());

		std::printf("\t%zu segments\n", list.size());
		for (auto& data : list)
		{
			std::printf("\t[%2u]\n", j);
			std::printf("\t\tunknown_0x0: 0x%08X\n", data.unknown_0x0);
			std::printf("\t\tunknown_0x4: 0x%08X\n", data.unknown_0x4);
			std::printf("\t\tunknown_0x8: 0x%08X\n", data.unknown_0x8);
			std::printf("\t\tunknown_0xc: 0x%08X\n", data.unknown_0xc);
			++j;
		}
		++i;
		++animation_index;
		if (animation_index >= specs.animation_sets[category_index].animations.size())
		{
			animation_index = 0;
			category_index++;
		}
	}

	return EXIT_SUCCESS;
}

struct Arguments
{
	enum class Mode
	{
		List,
		Header,
		ListAnimationSet,
		ShowBaseAnimationSet,
		ShowVariantAnimationSets,
		ShowHairGroups,
		ShowExtraData,
	};
	Mode mode;
	std::string spec_directory;
	struct Read
	{
		std::vector<std::string> filenames;
	} read;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("morphtool", "Inspect and read data files from LionHead CBN and HBN files internal segment (use "
	                                      "\"stdin\" if piping from packtool).");

	// clang-format off
	options.add_options()
		("h,help", "Display this help message.")
		("subcommand", "Subcommand.", cxxopts::value<std::string>())
	;
	options.positional_help("[read|write] [OPTION...]");
	options.add_options()
		("l,list-details", "Print Content Details.", cxxopts::value<std::vector<std::string>>())
		("H,header", "Print Header Contents.", cxxopts::value<std::vector<std::string>>())
		("d,spec-files-directory", "Directory of spec files (required).", cxxopts::value<std::string>())
		("s,list-animation-set", "List content of spec file.", cxxopts::value<std::vector<std::string>>())
		("b,show-base-animation-set", "Display animation data for the base animation.", cxxopts::value<std::vector<std::string>>())
		("V,show-variant-animation-sets", "Display animation data for the variant animations.", cxxopts::value<std::vector<std::string>>())
		("g,show-hair-groups", "Display hair group data.", cxxopts::value<std::vector<std::string>>())
		("e,show-extra-data", "Display extra data.", cxxopts::value<std::vector<std::string>>())
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
		if (result["spec-files-directory"].count() == 0)
		{
			std::cerr << options.help() << std::endl;
			return_code = EXIT_FAILURE;
			return false;
		}
		else
		{
			args.spec_directory = result["spec-files-directory"].as<std::string>();
		}
		if (result["subcommand"].as<std::string>() == "read")
		{
			if (result["list-details"].count() > 0)
			{
				args.mode = Arguments::Mode::List;
				args.read.filenames = result["list-details"].as<std::vector<std::string>>();
				return true;
			}
			if (result["header"].count() > 0)
			{
				args.mode = Arguments::Mode::Header;
				args.read.filenames = result["header"].as<std::vector<std::string>>();
				return true;
			}
			if (result["list-animation-set"].count() > 0)
			{
				args.mode = Arguments::Mode::ListAnimationSet;
				args.read.filenames = result["list-animation-set"].as<std::vector<std::string>>();
				return true;
			}
			if (result["show-base-animation-set"].count() > 0)
			{
				args.mode = Arguments::Mode::ShowBaseAnimationSet;
				args.read.filenames = result["show-base-animation-set"].as<std::vector<std::string>>();
				return true;
			}
			if (result["show-variant-animation-sets"].count() > 0)
			{
				args.mode = Arguments::Mode::ShowVariantAnimationSets;
				args.read.filenames = result["show-variant-animation-sets"].as<std::vector<std::string>>();
				return true;
			}
			if (result["show-hair-groups"].count() > 0)
			{
				args.mode = Arguments::Mode::ShowHairGroups;
				args.read.filenames = result["show-hair-groups"].as<std::vector<std::string>>();
				return true;
			}
			if (result["show-extra-data"].count() > 0)
			{
				args.mode = Arguments::Mode::ShowExtraData;
				args.read.filenames = result["show-extra-data"].as<std::vector<std::string>>();
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

	for (auto& filename : args.read.filenames)
	{
		openblack::morph::MorphFile morph;
		try
		{
			// Open file
			if (filename == "stdin")
			{
				std::vector<uint8_t> buffer;
				while (!std::cin.eof())
				{
					char byte;
					std::cin.get(byte);
					if (!std::cin.eof())
					{
						buffer.push_back(static_cast<uint8_t>(byte));
					}
				}
				printf("got %zu bytes from stdin\n", buffer.size());
				morph.Open(buffer, args.spec_directory);
			}
			else
			{
				morph.Open(filename, args.spec_directory);
			}

			switch (args.mode)
			{
			case Arguments::Mode::List:
				return_code |= ListDetails(morph);
				break;
			case Arguments::Mode::Header:
				return_code |= PrintHeader(morph);
				break;
			case Arguments::Mode::ListAnimationSet:
				return_code |= PrintSpecs(morph);
				break;
			case Arguments::Mode::ShowBaseAnimationSet:
				return_code |= ShowBaseAnimationSet(morph);
				break;
			case Arguments::Mode::ShowVariantAnimationSets:
				return_code |= ShowVariantAnimationSets(morph);
				break;
			case Arguments::Mode::ShowHairGroups:
				return_code |= ShowHairGroups(morph);
				break;
			case Arguments::Mode::ShowExtraData:
				return_code |= ShowExtraData(morph);
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
