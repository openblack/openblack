/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>
#include <cstring>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <MindFile.h>
#include <gtest/gtest.h>

#include "Creature/CreatureMindModel.h"

using namespace openblack;
using namespace openblack::creaturemind;

namespace
{
/// A small mind of the current version, every section filled
MindFileData SampleMind()
{
	MindFileData mind;
	mind.version = k_CurrentVersion;
	mind.speciesRow = 2;
	mind.savedAs = {'t', 'e', 's', 't', 0};
	mind.profile = {'P', 0, 'l', 0, 0, 0};
	mind.name = u"Tester";
	mind.counters = {1, 2, 3, 4, 5, 6, 7};
	for (int d = 0; d < 3; ++d)
	{
		mind.desires.push_back({.activated = d % 2,
		                        .value = 0.1f * static_cast<float>(d),
		                        .max = 1.0f,
		                        .increaseSeconds = 20.0f,
		                        .sources = {{.value = 0.5f, .threshold = 0.4f, .type = static_cast<uint32_t>(d)}}});
	}
	mind.treeDesireCount = 1;
	mind.trees = {
	    {.type = 0,
	     .desire = 0,
	     .desireAgain = 0,
	     .episodes = {{.kind = 0,
	                   .desire = 0,
	                   .action = 0,
	                   .belief = {.type = 6, .x = 100, .z = -200, .attributes = {1, 0, 1, 7, 0, 0, 6, 0, 3, 1, 0}},
	                   .feedback = -0.5f}}},
	    {.type = 1, .desire = 0, .desireAgain = 0},
	};
	mind.opinions = std::vector<float> {0.0f, 0.25f, -1.0f};
	mind.desireMemories = {{.value = 1.0f, .count = 2}, {}, {}};
	mind.actionsSeen = {{.count = 3, .turn = 10}};
	mind.miraclesSeen = {{.count = 1, .turn = 20}, {.count = 0, .turn = 0}};
	mind.attitudeToPlayer = 0.3f;
	mind.playerDesires = std::vector<float>(40, 0.5f);
	mind.townDesires = std::vector<float>(17, 0.25f);
	mind.known = {std::vector<MindKnownAction> {{.id = 0}}, std::vector<MindKnownAction> {{.id = 1}}};
	mind.unknown1 = 1;
	mind.alignment = -0.2f;
	mind.developmentTimer = 99;
	mind.developmentPhase = 13;
	mind.physique = {.age = 40,
	                 .strength = 0.6f,
	                 .energy = 0.7f,
	                 .scratch = 0.1f,
	                 .flags = 3,
	                 .needs = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f},
	                 .unknown3 = 4,
	                 .size = 1.5f,
	                 .listA = {1, 2},
	                 .listB = {3}};
	mind.database = std::vector<MindDatabaseEntry> {
	    {.id = 5, .unknown = 6, .size = 3, .pairs = {{.number = 1, .a = 0.5f, .b = 0.25f}, {.number = 3, .a = 1.0f}}}};
	mind.unknown2 = 8;
	mind.unknown3 = std::array<uint32_t, 4> {1, 2, 3, 4};
	mind.tattooHeader = std::array<uint32_t, 8> {};
	mind.tattoo = std::vector<uint8_t> {9, 8, 7};
	mind.learningCounts = std::vector<MindLearningCount>(45, {.a = 1, .b = 2, .c = 3});
	mind.drawnScale = 1.25f;
	mind.unknown4 = 0.5f;
	mind.unknown5 = std::array<int32_t, 2> {7, -7};
	return mind;
}

std::vector<uint8_t> ReadBytes(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

/// A folder of real creature saves to check against, given by OPENBLACK_CREATURE_SAVES; none by default
std::filesystem::path ReferenceFolder()
{
	const char* folder = std::getenv("OPENBLACK_CREATURE_SAVES");
	return folder != nullptr ? std::filesystem::path(folder) : std::filesystem::path {};
}
} // namespace

TEST(CreatureMindFile, EncryptionRoundTrips)
{
	std::vector<uint8_t> bytes {'H', 'e', 'l', 'l', 'o', 0};
	const auto plain = bytes;
	EncryptBlock(bytes);
	EXPECT_NE(bytes, plain);
	DecryptBlock(bytes);
	EXPECT_EQ(bytes, plain);
}

TEST(CreatureMindFile, CurrentVersionRoundTrips)
{
	const auto mind = SampleMind();
	const auto bytes = Write(mind);
	MindFileData read;
	ASSERT_EQ(Read(bytes, read), MindResult::Success);
	EXPECT_EQ(read.name, u"Tester");
	EXPECT_EQ(read.SavedAsText(), "test");
	EXPECT_EQ(read.ProfileText(), u"Pl");
	ASSERT_EQ(read.desires.size(), 3u);
	EXPECT_FLOAT_EQ(read.desires[2].value, 0.2f);
	ASSERT_EQ(read.trees.size(), 2u);
	ASSERT_EQ(read.trees[0].episodes.size(), 1u);
	EXPECT_EQ(read.trees[0].episodes[0].belief.z, -200);
	EXPECT_EQ(read.trees[0].episodes[0].belief.attributes.size(), 11u);
	ASSERT_TRUE(read.database.has_value());
	ASSERT_EQ(read.database->size(), 1u);
	EXPECT_EQ(read.database->at(0).pairs.size(), 2u);
	EXPECT_EQ(read.physique.size, 1.5f);
	EXPECT_EQ(Write(read), bytes);
}

TEST(CreatureMindFile, OlderVersionsLeaveOutTheirFields)
{
	auto mind = SampleMind();
	mind.version = 12;
	// Version 12 has no seventh counter, no player desires, no tattoo and no body size
	const auto bytes = Write(mind);
	MindFileData read;
	ASSERT_EQ(Read(bytes, read), MindResult::Success);
	EXPECT_EQ(read.counters.size(), 6u);
	EXPECT_FALSE(read.playerDesires.has_value());
	EXPECT_FALSE(read.tattoo.has_value());
	EXPECT_FALSE(read.physique.size.has_value());
	EXPECT_TRUE(read.opinions.has_value());
	EXPECT_TRUE(read.desires[0].before15.has_value());
	EXPECT_EQ(Write(read), bytes);

	mind.version = 8;
	// Version 8 keeps no sources, no opinions, and a number for each desire only versions 6 to 9 have
	const auto older = Write(mind);
	ASSERT_EQ(Read(older, read), MindResult::Success);
	EXPECT_TRUE(read.desires[0].sources.empty());
	EXPECT_TRUE(read.desires[0].from6To9.has_value());
	EXPECT_FALSE(read.opinions.has_value());
	EXPECT_TRUE(read.from6To9.has_value());
	EXPECT_TRUE(read.before11.has_value());
	EXPECT_EQ(Write(read), older);
}

TEST(CreatureMindFile, RejectsWhatIsNotAMind)
{
	MindFileData read;
	EXPECT_EQ(Read(std::vector<uint8_t> {1, 2, 3}, read), MindResult::ErrTruncated);
	auto bytes = Write(SampleMind());
	// A species row past the game's 17, as the version 17 templates have where there is no row
	bytes[4] = 200;
	EXPECT_EQ(Read(bytes, read), MindResult::ErrNotAMindFile);
	bytes = Write(SampleMind());
	bytes[0] = 99;
	EXPECT_EQ(Read(bytes, read), MindResult::ErrUnsupportedVersion);
	bytes = Write(SampleMind());
	bytes.resize(bytes.size() - 5);
	EXPECT_EQ(Read(bytes, read), MindResult::ErrTruncated);
}

TEST(CreatureMindFile, KeepsTrailingBytes)
{
	auto bytes = Write(SampleMind());
	bytes.push_back(0xab);
	MindFileData read;
	ASSERT_EQ(Read(bytes, read), MindResult::Success);
	EXPECT_EQ(read.trailing.size(), 1u);
	EXPECT_EQ(Write(read), bytes);
}

TEST(CreatureMindFile, ModelKeepsAFileItWasLoadedFrom)
{
	// Loaded into a mind and written back without anything learnt since, a file comes out the same
	auto mind = SampleMind();
	mind.desires.resize(40, mind.desires[0]);
	mind.desireMemories.resize(40);
	mind.treeDesireCount = 40;
	mind.trees.clear();
	for (int32_t d = 0; d < 40; ++d)
	{
		mind.trees.push_back({.type = 0, .desire = d, .desireAgain = d});
		mind.trees.push_back({.type = 1, .desire = d, .desireAgain = d});
	}
	mind.trees[2].episodes.push_back({.desire = 1,
	                                  .belief = {.type = 8, .x = 1, .z = 2, .attributes = {1, 0, 1, 0, 0, 12, 8, 2, 3, 23, 29}},
	                                  .feedback = 0.5f});
	mind.opinions = std::vector<float>(3, 0.5f);
	creature_desires::Desires fresh;
	for (auto& desire : fresh.desires)
	{
		desire.sources = {{.type = 0}};
	}
	creature_watching::Knowledge knowledge {
	    .skillsSeen = std::vector<creature_watching::Sighting>(1),
	    .miraclesSeen = std::vector<creature_watching::Sighting>(2),
	    .skillsKnown = std::vector<bool>(1, false),
	    .miraclesKnown = std::vector<bool>(2, false),
	};
	const auto freshLearnt = creature_mind_model::Fresh(fresh, 3, knowledge);
	const auto loaded = creature_mind_model::FromFile(mind, fresh, freshLearnt);
	EXPECT_EQ(loaded.learnt.name, u"Tester");
	EXPECT_EQ(loaded.developmentPhase, 13u);
	EXPECT_TRUE(loaded.learnt.knowledge.skillsKnown[0]);
	EXPECT_TRUE(loaded.learnt.knowledge.miraclesKnown[1]);
	EXPECT_EQ(loaded.learnt.episodes[0][1].size(), 1u);
	const auto saved = creature_mind_model::ToFile(loaded.desires, loaded.learnt, loaded.developmentPhase,
	                                               loaded.attitudeToPlayer, loaded.speciesRow);
	EXPECT_EQ(Write(saved), Write(mind));
}

TEST(CreatureMindFile, FreshMindWritesACurrentFile)
{
	creature_desires::Desires desires;
	const auto learnt = creature_mind_model::Fresh(desires, 328, {});
	const auto file = creature_mind_model::ToFile(desires, learnt, 2, 0.0f, 3);
	const auto bytes = Write(file);
	MindFileData read;
	ASSERT_EQ(Read(bytes, read), MindResult::Success);
	EXPECT_EQ(read.version, k_CurrentVersion);
	EXPECT_EQ(read.desires.size(), 40u);
	EXPECT_EQ(read.trees.size(), 80u);
	EXPECT_EQ(read.opinions->size(), 328u);
	EXPECT_EQ(read.developmentPhase, 2);
}

/// Real creature saves are players' own files, not part of the project: this only runs on a folder of them given by
/// OPENBLACK_CREATURE_SAVES
TEST(CreatureMindFile, ReferenceSavesParseAndRoundTrip)
{
	const auto folder = ReferenceFolder();
	if (folder.empty() || !std::filesystem::is_directory(folder))
	{
		GTEST_SKIP() << "Set OPENBLACK_CREATURE_SAVES to a folder of creature saves to check them";
	}
	size_t files = 0;
	for (const auto& entry : std::filesystem::directory_iterator(folder))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}
		const auto bytes = ReadBytes(entry.path());
		MindFileData mind;
		ASSERT_EQ(Read(bytes, mind), MindResult::Success) << entry.path();
		EXPECT_EQ(mind.version, k_CurrentVersion) << entry.path();
		EXPECT_EQ(mind.desires.size(), 40u);
		EXPECT_EQ(mind.trees.size(), 80u);
		EXPECT_EQ(Write(mind), bytes) << entry.path() << " doesn't write back the same";
		++files;
	}
	EXPECT_GT(files, 0u);
}
