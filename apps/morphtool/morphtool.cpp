/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <fstream>
#include <iostream>
#include <string>

#include <MorphFile.h>
#include <cxxopts.hpp>

int ListDetails(openblack::morph::MorphFile& morph)
{
	const auto& header = morph.GetHeader();

	std::printf("base mesh: \"%s\"\n", header.baseMeshName.data());
	uint32_t numVariants = 0;
	for (const auto& meshes : header.variantMeshNames)
	{
		if (std::strlen(meshes.data()) > 0)
		{
			++numVariants;
		}
	}
	std::printf("%u variant meshes:", numVariants);
	for (const auto& meshes : header.variantMeshNames)
	{
		if (std::strlen(meshes.data()) > 0)
		{
			std::printf(" \"%s\"", meshes.data());
		}
	}
	std::printf("\n");

	size_t numAnimationSets = 0;
	const auto& specs = morph.GetAnimationSpecs();
	for (const auto& animSet : specs.animationSets)
	{
		numAnimationSets += animSet.animations.size();
	}
	std::printf("%zu animations from %zu categories:\n", numAnimationSets, specs.animationSets.size());
	std::printf("%u variant animation sets\n", std::min(4u, numVariants));

	std::printf("%zu base animations\n", morph.GetBaseAnimationSet().size());
	for (uint8_t i = 0; i < std::min(4u, numVariants); ++i)
	{
		const auto& animationSet = morph.GetVariantAnimationSet(i);
		std::printf("%zu animations for \"%s\"\n", animationSet.size(), header.variantMeshNames.at(i).data());
	}

	std::printf("%zu hair groups\n", morph.GetHairGroups().size());
	std::printf("sound object %u, sound bank \"%s\"\n", morph.GetHairHeader().soundObject, morph.GetSoundBankName().c_str());

	const auto& extraData = morph.GetExtraData();

	size_t extraDataTotal = 0;
	for (const auto& data : extraData)
	{
		extraDataTotal += data.size();
	}

	std::printf("%zu total extra data segments\n", extraDataTotal);
	std::printf("%zu extra data groups\n", extraData.size());

	return EXIT_SUCCESS;
}

int PrintHeader(openblack::morph::MorphFile& morph)
{
	const auto& header = morph.GetHeader();

	std::printf("unknown0x0: 0x%08X\n", header.unknown0x0);
	std::printf("specFileVersion: %u\n", header.specFileVersion);
	std::printf("binaryVersion: %u\n", header.binaryVersion);
	std::printf("baseMeshName: %s\n", header.baseMeshName.data());
	for (uint8_t i = 0; i < 6; ++i)
	{
		std::printf("variantMeshNames[%u]: %s\n", i, header.variantMeshNames.at(i).data());
	}

	return EXIT_SUCCESS;
}

int PrintSpecs(openblack::morph::MorphFile& morph)
{
	const auto& specs = morph.GetAnimationSpecs();

	std::printf("specs path: %s\n", specs.path.string().c_str());
	std::printf("specs version: %u\n", specs.version);

	size_t j = 0;
	for (const auto& animSet : specs.animationSets)
	{
		j += animSet.animations.size();
	}
	std::printf("%zu animations from %zu categories:\n", j, specs.animationSets.size());

	uint32_t i = 0;
	j = 0;
	for (const auto& animSet : specs.animationSets)
	{
		std::printf("[%u] category \"%s\" with %zu animations:\n", i, animSet.name.c_str(), animSet.animations.size());
		i++;
		for (const auto& desc : animSet.animations)
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
	std::printf("\t\tduration: %u ms\n", animation.header.duration);
	std::printf("\t\tlooping: %u\n", animation.header.looping);
	std::printf("\t\tstrideRate: %f\n", static_cast<double>(animation.header.strideRate));
	std::printf("\t\tstrideLength: %f\n", static_cast<double>(animation.header.strideLength));
	std::printf("\t\tdisplacement: %f, %f, %f\n", static_cast<double>(animation.header.displacement[0]),
	            static_cast<double>(animation.header.displacement[1]), static_cast<double>(animation.header.displacement[2]));
	std::printf("\t\tframeCount: %u\n", animation.header.frameCount);
	std::printf("\t\tmeshBoneCount: %u\n", animation.header.meshBoneCount);
	std::printf("\t\trotatedJointCount: %u\n", animation.header.rotatedJointCount);
	std::printf("\t\ttranslatedJointCount: %u\n", animation.header.translatedJointCount);

	std::printf("\trotatedJointIndices: [");
	for (uint32_t index : animation.rotatedJointIndices)
	{
		std::printf("%u, ", index);
	}
	std::printf("]\n");

	std::printf("\ttranslatedJointIndices: [");
	for (uint32_t index : animation.translatedJointIndices)
	{
		std::printf("%u, ", index);
	}
	std::printf("]\n");

	std::printf("\tKeyframes:\n");

	uint32_t i = 0;
	for (const auto& frame : animation.keyframes)
	{
		std::printf("\t\t[%2u]\n", i);
		std::printf("\t\teulerAngles: [");
		for (auto angles : frame.eulerAngles)
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
	const auto& animationSet = morph.GetBaseAnimationSet();

	std::printf("%zu base animations\n", animationSet.size());

	uint32_t i = 0;
	for (const auto& animation : animationSet)
	{
		std::printf("\t[%2u]\n", i);
		PrintAnimation(animation);
		++i;
	}

	return EXIT_SUCCESS;
}

int ShowVariantAnimationSets(openblack::morph::MorphFile& morph)
{
	const auto& header = morph.GetHeader();

	uint8_t numVariants = 0;
	for (uint8_t i = 0; i < 4; ++i)
	{
		if (std::strlen(header.variantMeshNames.at(i).data()) > 0 && !morph.GetVariantAnimationSet(i).empty())
		{
			++numVariants;
		}
	}
	std::printf("%u animation set variants\n", numVariants);

	for (uint8_t i = 0; i < numVariants; ++i)
	{
		const auto& animationSet = morph.GetVariantAnimationSet(i);
		std::printf("%zu animations for \"%s\"\n", animationSet.size(), header.variantMeshNames.at(i).data());
		uint32_t j = 0;
		for (const auto& animation : animationSet)
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
	const auto& hairGroups = morph.GetHairGroups();

	std::printf("%zu hair groups\n", hairGroups.size());

	uint32_t i = 0;
	for (const auto& group : hairGroups)
	{
		std::printf("[%2u]\n", i);
		std::printf("\tHeader:\n");
		std::printf("\t\tsegmentsVersion: %u\n", group.header.segmentsVersion);
		std::printf("\t\thairCount: %u\n", group.header.hairCount);
		std::printf("\t\tsegmentCount: %u\n", group.header.segmentCount);
		std::printf("\t\tmappingIndex: %u\n", group.header.mappingIndex);
		constexpr std::array<const char*, 3> k_VariantNames {"neutral", "evil", "good"};
		for (size_t j = 0; j < group.header.variants.size(); ++j)
		{
			const auto& variant = group.header.variants.at(j);
			std::printf("\t\t%s:\n", k_VariantNames.at(j));
			std::printf("\t\t\tcolour: %d, %d, %d\n", variant.red, variant.green, variant.blue);
			std::printf("\t\t\tlength: %f\n", static_cast<double>(variant.length));
			std::printf("\t\t\tdamping: %f\n", static_cast<double>(variant.damping));
			std::printf("\t\t\tstiffness: %f\n", static_cast<double>(variant.stiffness));
			std::printf("\t\t\tthickness: %f\n", static_cast<double>(variant.thickness));
		}
		std::printf("\tHairs:\n");
		for (size_t j = 0; j < group.hairs.size(); ++j)
		{
			const auto& hair = group.hairs[j];
			std::printf("\t[%2zu]\n", j);
			std::printf("\t\tflags: 0x%08X\n", hair.flags);
			std::printf("\t\tintersection: primitive %u, vertices %u %u %u, groups %u %u %u, u %f, v %f\n",
			            hair.intersection.primitive, hair.intersection.vertices[0], hair.intersection.vertices[1],
			            hair.intersection.vertices[2], hair.intersection.vertexGroups[0], hair.intersection.vertexGroups[1],
			            hair.intersection.vertexGroups[2], static_cast<double>(hair.intersection.u),
			            static_cast<double>(hair.intersection.v));
			for (size_t k = 0; k < hair.angles.size(); ++k)
			{
				std::printf("\t\t%s angles: [%f, %f, %f]\n", k_VariantNames.at(k), static_cast<double>(hair.angles.at(k)[0]),
				            static_cast<double>(hair.angles.at(k)[1]), static_cast<double>(hair.angles.at(k)[2]));
			}
		}
		++i;
	}

	return EXIT_SUCCESS;
}

int ShowExtraData(openblack::morph::MorphFile& morph)
{
	const auto& extraData = morph.GetExtraData();
	const auto& specs = morph.GetAnimationSpecs();

	size_t extraDataTotal = 0;
	for (const auto& data : extraData)
	{
		extraDataTotal += data.size();
	}

	std::printf("%zu total extra data segments\n", extraDataTotal);
	std::printf("%zu extra data groups (one per animation)\n", extraData.size());

	uint32_t i = 0;
	uint32_t categoryIndex = 0;
	size_t animationIndex = 0;
	for (const auto& list : extraData)
	{
		uint32_t j = 0;
		std::printf("[%2u]: %s (%s)\n", i, specs.animationSets[categoryIndex].animations[animationIndex].name.c_str(),
		            specs.animationSets[categoryIndex].name.c_str());

		std::printf("\t%zu segments\n", list.size());
		for (const auto& data : list)
		{
			std::printf("\t[%2u]\n", j);
			std::printf("\t\ttype: %u\n", data.type);
			std::printf("\t\tframe: %u ms\n", data.frame);
			std::printf("\t\taction: %u\n", data.action);
			std::printf("\t\tmode: %u\n", data.mode);
			++j;
		}
		++i;
		++animationIndex;
		if (animationIndex >= specs.animationSets[categoryIndex].animations.size())
		{
			animationIndex = 0;
			categoryIndex++;
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
	std::filesystem::path specDirectory;
	struct Read
	{
		std::vector<std::filesystem::path> filenames;
	} read;
};

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("morphtool", "Inspect and read data files from LionHead CBN and HBN files internal segment (use "
	                                      "\"stdin\" if piping from packtool).");

	options.add_options()                                            //
	    ("h,help", "Display this help message.")                     //
	    ("subcommand", "Subcommand.", cxxopts::value<std::string>()) //
	    ;
	options.positional_help("[read|write] [OPTION...]");
	options.add_options()                                                                                            //
	    ("l,list-details", "Print Content Details.", cxxopts::value<std::vector<std::filesystem::path>>())           //
	    ("H,header", "Print Header Contents.", cxxopts::value<std::vector<std::filesystem::path>>())                 //
	    ("d,spec-files-directory", "Directory of spec files (required).", cxxopts::value<std::filesystem::path>())   //
	    ("s,list-animation-set", "List content of spec file.", cxxopts::value<std::vector<std::filesystem::path>>()) //
	    ("b,show-base-animation-set", "Display animation data for the base animation.",                              //
	     cxxopts::value<std::vector<std::filesystem::path>>())                                                       //
	    ("V,show-variant-animation-sets", "Display animation data for the variant animations.",                      //
	     cxxopts::value<std::vector<std::filesystem::path>>())                                                       //
	    ("g,show-hair-groups", "Display hair group data.", cxxopts::value<std::vector<std::filesystem::path>>())     //
	    ("e,show-extra-data", "Display extra data.", cxxopts::value<std::vector<std::filesystem::path>>())           //
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
	if (result["spec-files-directory"].count() == 0)
	{
		std::cerr << options.help() << '\n';
		returnCode = EXIT_FAILURE;
		return false;
	}

	args.specDirectory = result["spec-files-directory"].as<std::filesystem::path>();
	if (result["subcommand"].as<std::string>() == "read")
	{
		if (result["list-details"].count() > 0)
		{
			args.mode = Arguments::Mode::List;
			args.read.filenames = result["list-details"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["header"].count() > 0)
		{
			args.mode = Arguments::Mode::Header;
			args.read.filenames = result["header"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["list-animation-set"].count() > 0)
		{
			args.mode = Arguments::Mode::ListAnimationSet;
			args.read.filenames = result["list-animation-set"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["show-base-animation-set"].count() > 0)
		{
			args.mode = Arguments::Mode::ShowBaseAnimationSet;
			args.read.filenames = result["show-base-animation-set"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["show-variant-animation-sets"].count() > 0)
		{
			args.mode = Arguments::Mode::ShowVariantAnimationSets;
			args.read.filenames = result["show-variant-animation-sets"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["show-hair-groups"].count() > 0)
		{
			args.mode = Arguments::Mode::ShowHairGroups;
			args.read.filenames = result["show-hair-groups"].as<std::vector<std::filesystem::path>>();
			return true;
		}
		if (result["show-extra-data"].count() > 0)
		{
			args.mode = Arguments::Mode::ShowExtraData;
			args.read.filenames = result["show-extra-data"].as<std::vector<std::filesystem::path>>();
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

	for (auto& filename : args.read.filenames)
	{
		openblack::morph::MorphFile morph;
		// Open file
		openblack::morph::MorphResult result;
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
			result = morph.Open(buffer, args.specDirectory);
		}
		else
		{
			result = morph.Open(filename, args.specDirectory);
		}

		if (result != openblack::morph::MorphResult::Success)
		{
			std::cerr << openblack::morph::ResultToStr(result) << "\n";
			returnCode |= EXIT_FAILURE;
			continue;
		}

		switch (args.mode)
		{
		case Arguments::Mode::List:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= ListDetails(morph);
			break;
		case Arguments::Mode::Header:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= PrintHeader(morph);
			break;
		case Arguments::Mode::ListAnimationSet:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= PrintSpecs(morph);
			break;
		case Arguments::Mode::ShowBaseAnimationSet:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= ShowBaseAnimationSet(morph);
			break;
		case Arguments::Mode::ShowVariantAnimationSets:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= ShowVariantAnimationSets(morph);
			break;
		case Arguments::Mode::ShowHairGroups:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= ShowHairGroups(morph);
			break;
		case Arguments::Mode::ShowExtraData:
			std::printf("file: %s\n", filename.generic_string().c_str());
			returnCode |= ShowExtraData(morph);
			break;
		default:
			returnCode = EXIT_FAILURE;
			break;
		}
	}

	return returnCode;
}
