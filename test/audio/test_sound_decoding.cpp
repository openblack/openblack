/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// Decodes every sample of every sound bank of a Black & White installation and checks each against what its
// container declares. The game's data is not distributed, so this only runs when OPENBLACK_GAME_PATH points at an
// installation.

#include <cstdlib>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <Audio/SoundDecoder.h>
#include <PackFile.h>
#include <gtest/gtest.h>

using namespace openblack;

TEST(SoundDecoding, EveryGameSampleDecodes)
{
	const auto* gamePath = std::getenv("OPENBLACK_GAME_PATH");
	if (gamePath == nullptr)
	{
		GTEST_SKIP() << "Set OPENBLACK_GAME_PATH to a Black & White installation to decode its sound banks";
	}
	const auto audioPath = std::filesystem::path(gamePath) / "Audio";
	ASSERT_TRUE(std::filesystem::exists(audioPath)) << audioPath;

	size_t decoded = 0;
	size_t trimmed = 0;
	std::map<std::string, size_t> containers;
	std::vector<std::string> problems;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(audioPath))
	{
		if (entry.path().extension() != ".sad")
		{
			continue;
		}

		pack::PackFile pack;
		const auto result = pack.Open(entry.path());
		if (result != pack::PackResult::Success)
		{
			problems.push_back(entry.path().filename().string() + ": " + std::string(pack::ResultToStr(result)));
			continue;
		}

		const auto& headers = pack.GetAudioSampleHeaders();
		const auto& samples = pack.GetAudioSamplesData();
		for (size_t i = 0; i < headers.size(); ++i)
		{
			const auto name = entry.path().filename().string() + "/" + std::to_string(headers[i].id) + " " +
			                  std::filesystem::path(std::string(headers[i].name.data())).filename().string();
			const auto sound = audio::DecodeSound(samples[i], static_cast<int>(headers[i].sampleRate));
			++containers[audio::ToString(sound.container)];
			if (!sound.sound)
			{
				problems.push_back(name + ": " + sound.error);
				continue;
			}
			++decoded;
			trimmed += sound.trimmedFrames != 0 ? 1 : 0;
			for (const auto& warning : sound.warnings)
			{
				problems.push_back(std::string(name).append(": ").append(warning));
			}
		}
	}

	for (const auto& [container, count] : containers)
	{
		std::cout << count << " " << container << " samples\n";
	}
	std::cout << decoded << " samples decoded, " << trimmed << " with block padding removed, " << problems.size()
	          << " problems\n";
	for (const auto& problem : problems)
	{
		ADD_FAILURE() << problem;
	}
}
