/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdio>
#include <cstdlib>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <MindFile.h>

using namespace openblack::creaturemind;

namespace
{
std::string Narrow(const std::u16string& text)
{
	std::string narrow;
	for (const auto c : text)
	{
		narrow.push_back(c < 0x80 ? static_cast<char>(c) : '?');
	}
	return narrow;
}

void Dump(const MindFileData& mind, bool episodes)
{
	std::printf("version %u, species row %u\n", mind.version, mind.speciesRow);
	std::printf("name \"%s\", saved as \"%s\", profile \"%s\"\n", Narrow(mind.name).c_str(), mind.SavedAsText().c_str(),
	            Narrow(mind.ProfileText()).c_str());
	std::printf("development phase %d (timer %u), attitude to player %.3f, alignment %.3f\n", mind.developmentPhase,
	            mind.developmentTimer, static_cast<double>(mind.attitudeToPlayer),
	            static_cast<double>(mind.alignment.value_or(0.0f)));
	std::printf("counters:");
	for (const auto counter : mind.counters)
	{
		std::printf(" %u", counter);
	}
	std::printf("\n%zu desires:\n", mind.desires.size());
	for (size_t i = 0; i < mind.desires.size(); ++i)
	{
		const auto& desire = mind.desires[i];
		std::printf("  %2zu %s value %.3f max %.3f grows in %.1f s, sources:", i, desire.activated != 0 ? "on " : "off",
		            static_cast<double>(desire.value), static_cast<double>(desire.max),
		            static_cast<double>(desire.increaseSeconds));
		for (const auto& source : desire.sources)
		{
			std::printf(" [%u %.2f/%.2f]", source.type, static_cast<double>(source.value),
			            static_cast<double>(source.threshold));
		}
		std::printf("\n");
	}
	size_t total = 0;
	for (const auto& tree : mind.trees)
	{
		total += tree.episodes.size();
	}
	std::printf("%zu trees, %zu examples\n", mind.trees.size(), total);
	if (episodes)
	{
		for (const auto& tree : mind.trees)
		{
			for (const auto& episode : tree.episodes)
			{
				std::printf("  tree %d desire %d: kind %d action %d, thing %u at (%d, %d) feedback %.3f, attributes:",
				            tree.type, tree.desire, episode.kind, episode.action, episode.belief.type, episode.belief.x,
				            episode.belief.z, static_cast<double>(episode.feedback));
				for (const auto value : episode.belief.attributes)
				{
					std::printf(" %u", value);
				}
				std::printf("\n");
			}
		}
	}
	if (mind.opinions.has_value())
	{
		std::printf("%zu action opinions; strongest:", mind.opinions->size());
		for (size_t i = 0; i < mind.opinions->size(); ++i)
		{
			const auto opinion = (*mind.opinions)[i];
			if (opinion > 0.5f || opinion < -0.5f)
			{
				std::printf(" %zu=%.2f", i, static_cast<double>(opinion));
			}
		}
		std::printf("\n");
	}
	std::printf("known actions %zu, known miracles %zu, miracles seen %zu\n", mind.known[0].size(), mind.known[1].size(),
	            mind.miraclesSeen.size());
	std::printf("physique: age %u, strength %.3f, energy %.3f, size %.3f\n", mind.physique.age,
	            static_cast<double>(mind.physique.strength), static_cast<double>(mind.physique.energy),
	            static_cast<double>(mind.physique.size.value_or(0.0f)));
	std::printf("tattoo %zu bytes, database %zu entries, %zu trailing bytes\n",
	            mind.tattoo.has_value() ? mind.tattoo->size() : 0, mind.database.has_value() ? mind.database->size() : 0,
	            mind.trailing.size());
}
} // namespace

int main(int argc, char* argv[]) noexcept
{
	if (argc < 2)
	{
		std::printf("usage: creaturemindtool [--episodes] [--roundtrip] FILE...\n"
		            "Dumps creature mind files; --roundtrip checks that each writes back byte for byte.\n");
		return EXIT_FAILURE;
	}
	bool episodes = false;
	bool roundTrip = false;
	int failures = 0;
	for (const std::string_view arg : std::span(argv + 1, static_cast<size_t>(argc - 1)))
	{
		if (arg == "--episodes")
		{
			episodes = true;
			continue;
		}
		if (arg == "--roundtrip")
		{
			roundTrip = true;
			continue;
		}
		std::ifstream file(std::filesystem::path(arg), std::ios::binary);
		const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		MindFileData mind;
		const auto result = Read(bytes, mind);
		std::printf("== %s (%zu bytes): %s\n", std::string(arg).c_str(), bytes.size(), ResultToStr(result).data());
		if (result != MindResult::Success)
		{
			++failures;
			continue;
		}
		if (roundTrip)
		{
			const bool same = Write(mind) == bytes;
			std::printf("round trip: %s\n", same ? "identical" : "DIFFERENT");
			failures += same ? 0 : 1;
		}
		else
		{
			Dump(mind, episodes);
		}
	}
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
