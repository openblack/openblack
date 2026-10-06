/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "3D/FlatLand.h"
#include "Debug/BenchmarkRecorder.h"
#include "Debug/TestbedCrowd.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;
using namespace openblack::benchmark;

namespace
{
constexpr float k_Epsilon = 1e-4f;

/// Whether a point is on the lake's open water or shallows
bool InLake(glm::vec2 offset)
{
	const auto lake = flat_land::k_LakeCentre - flat_land::k_MapMiddle;
	const auto from = glm::abs(offset - lake);
	const auto half = flat_land::k_LakeHalfExtent + glm::vec2(2.0f * flat_land::k_CellSize);
	return from.x < half.x && from.y < half.y;
}

std::vector<StageInfo> TwoStages()
{
	return {{.name = "Think", .render = false}, {.name = "Draw", .render = true}};
}
} // namespace

TEST(TestbedCrowd, RandomIsTheSameFromTheSameSeed)
{
	CrowdRandom a(7);
	CrowdRandom b(7);
	CrowdRandom c(8);
	bool differs = false;
	for (int i = 0; i < 100; ++i)
	{
		const auto value = a.Next();
		EXPECT_EQ(value, b.Next());
		differs |= value != c.Next();
	}
	EXPECT_TRUE(differs);
}

TEST(TestbedCrowd, RandomStaysInItsRange)
{
	CrowdRandom random(3);
	for (int i = 0; i < 1000; ++i)
	{
		const auto value = random.Between(-2.0f, 5.0f);
		EXPECT_GE(value, -2.0f);
		EXPECT_LT(value, 5.0f);
		EXPECT_LT(random.Below(9), 9u);
	}
}

TEST(TestbedCrowd, SpreadPointsAreTheCountApartAndOffTheLake)
{
	CrowdRandom random(1);
	const auto points = SpreadPoints(2000, 20.0f, 10.0f, 0.0f, random);
	ASSERT_EQ(points.size(), 2000u);
	// Without jitter they are on the grid, every one apart from the others
	std::set<std::pair<int, int>> cells;
	for (const auto& point : points)
	{
		EXPECT_FALSE(NearLake(point, 10.0f));
		EXPECT_NEAR(std::fmod(std::abs(point.x), 20.0f), 0.0f, k_Epsilon);
		cells.insert({static_cast<int>(std::lround(point.x / 20.0f)), static_cast<int>(std::lround(point.y / 20.0f))});
	}
	EXPECT_EQ(cells.size(), points.size());
	// The nearest the middle come first
	EXPECT_LE(glm::length(points.front()), glm::length(points.back()));
}

TEST(TestbedCrowd, SpreadPointsKeepToTheMap)
{
	CrowdRandom random(1);
	// More than fit on the map: as many as fit, and none off it
	const auto points = SpreadPoints(200'000, 20.0f, 0.0f, 0.5f, random);
	EXPECT_LT(points.size(), 200'000u);
	for (const auto& point : points)
	{
		EXPECT_LE(std::abs(point.x), 2400.0f);
		EXPECT_LE(std::abs(point.y), 2400.0f);
	}
	EXPECT_TRUE(SpreadPoints(0, 20.0f, 0.0f, 0.0f, random).empty());
}

TEST(TestbedCrowd, CreaturesAreLaidOutTheSameEachTime)
{
	const auto first = LayOutCreatures(500, 2026);
	const auto second = LayOutCreatures(500, 2026);
	ASSERT_EQ(first.size(), 500u);
	ASSERT_EQ(second.size(), first.size());
	for (size_t i = 0; i < first.size(); ++i)
	{
		EXPECT_EQ(first[i].offset, second[i].offset);
		EXPECT_EQ(first[i].species, second[i].species);
		EXPECT_EQ(first[i].owner, second[i].owner);
	}
	EXPECT_NE(LayOutCreatures(500, 2027).front().offset, first.front().offset);
}

TEST(TestbedCrowd, CreaturesMixSpeciesAndOwnersAndKeepApart)
{
	const auto creatures = LayOutCreatures(1000, 2026);
	std::set<CreatureType> species;
	std::set<PlayerNames> owners;
	for (size_t i = 0; i < creatures.size(); ++i)
	{
		const auto& creature = creatures[i];
		species.insert(creature.species);
		owners.insert(creature.owner);
		EXPECT_FALSE(InLake(creature.offset));
		EXPECT_GE(creature.facingDegrees, 0.0f);
		EXPECT_LT(creature.facingDegrees, 360.0f);
		// Neighbours on the grid, jittered by at most 30% of the spacing each, stay well apart
		if (i > 0)
		{
			EXPECT_GT(glm::distance(creature.offset, creatures[i - 1].offset), 0.3f * k_CreatureSpacing);
		}
	}
	EXPECT_EQ(species.size(), static_cast<size_t>(CreatureType::_COUNT) - 1);
	EXPECT_EQ(owners.size(), k_CrowdOwners.size());
	EXPECT_EQ(species.count(CreatureType::Unknown), 0u);
}

TEST(TestbedCrowd, VillagersLiveInTownsOfHomes)
{
	const auto layout = LayOutVillagers(1234, 2026);
	ASSERT_EQ(layout.villagers.size(), 1234u);
	EXPECT_EQ(layout.towns.size(), (1234u + k_VillagersPerTown - 1) / k_VillagersPerTown);
	std::vector<size_t> people(layout.abodes.size());
	std::set<Tribe> tribes;
	std::set<size_t> roles;
	size_t children = 0;
	for (const auto& villager : layout.villagers)
	{
		ASSERT_LT(villager.abode, layout.abodes.size());
		const auto& home = layout.abodes[villager.abode];
		ASSERT_LT(home.town, layout.towns.size());
		const auto& town = layout.towns[home.town];
		EXPECT_TRUE(home.home);
		++people[villager.abode];
		// Of the town's tribe, in a home of its tribe, near it
		const auto type = static_cast<size_t>(villager.type);
		EXPECT_EQ(villager.type, VillagerOf(town.tribe, type % 7));
		EXPECT_EQ(home.type, HutOf(town.tribe));
		EXPECT_LE(glm::distance(villager.offset, home.offset), 15.0f + k_Epsilon);
		EXPECT_LE(glm::distance(villager.offset, town.offset), k_VillagerSpread + k_Epsilon);
		EXPECT_FALSE(InLake(villager.offset));
		tribes.insert(town.tribe);
		roles.insert(type % 7);
		children += villager.age < 18 ? 1 : 0;
		EXPECT_GE(villager.age, 5u);
		EXPECT_LT(villager.age, 60u);
	}
	for (size_t i = 0; i < layout.abodes.size(); ++i)
	{
		EXPECT_LE(people[i], k_VillagersPerHome);
		// Each town's store stands in its middle with no one living in it
		if (!layout.abodes[i].home)
		{
			EXPECT_EQ(people[i], 0u);
			EXPECT_EQ(layout.abodes[i].type, StoragePitOf(layout.towns[layout.abodes[i].town].tribe));
			EXPECT_EQ(layout.abodes[i].offset, layout.towns[layout.abodes[i].town].offset);
		}
	}
	EXPECT_GT(tribes.size(), 5u);
	EXPECT_EQ(roles.size(), 7u);
	EXPECT_GT(children, 0u);
	EXPECT_LT(children, layout.villagers.size() / 3);
}

TEST(TestbedCrowd, TribesNameTheirVillagersAndAbodes)
{
	EXPECT_EQ(VillagerOf(Tribe::CELTIC, 0), VillagerInfo::CelticHousewifeFemale);
	EXPECT_EQ(VillagerOf(Tribe::AFRICAN, 3), VillagerInfo::AfricanFarmerMale);
	EXPECT_EQ(VillagerOf(Tribe::TIBETAN, 6), VillagerInfo::TibetanTraderMale);
	EXPECT_EQ(HutOf(Tribe::NORSE), AbodeInfo::NorseHut);
	EXPECT_EQ(StoragePitOf(Tribe::AZTEC), AbodeInfo::AztecStoragePit);
	EXPECT_EQ(StoragePitOf(Tribe::TIBETAN), AbodeInfo::TibetanStoragePit);
}

TEST(BenchmarkRecorder, SummarisesFramesAndStages)
{
	Recorder recorder(TwoStages(), 100);
	for (int i = 1; i <= 100; ++i)
	{
		// The first stage runs every other frame, taking 2 ms
		const std::array<float, 2> stages {i % 2 == 0 ? 2.0f : 0.0f, 0.5f};
		recorder.Add(static_cast<float>(i), 1.0f, static_cast<float>(i) - 1.0f, stages, 10.0f);
	}
	const auto results = recorder.Summarise();
	EXPECT_EQ(results.frame.count, 100u);
	EXPECT_NEAR(results.frame.average, 50.5f, k_Epsilon);
	EXPECT_EQ(results.frame.p95, 95.0f);
	EXPECT_EQ(results.frame.max, 100.0f);
	EXPECT_NEAR(results.update.average, 1.0f, k_Epsilon);
	EXPECT_NEAR(results.draws.average, 10.0f, k_Epsilon);
	ASSERT_EQ(results.stages.size(), 2u);
	// The costliest per frame first
	EXPECT_EQ(results.stages[0].stage, 0u);
	EXPECT_NEAR(results.stages[0].perFrame.average, 1.0f, k_Epsilon);
	EXPECT_EQ(results.stages[0].framesRun, 50u);
	EXPECT_NEAR(results.stages[0].meanWhenRun, 2.0f, k_Epsilon);
	EXPECT_EQ(results.stages[1].framesRun, 100u);
}

TEST(BenchmarkRecorder, KeepsOnlyTheLastFrames)
{
	Recorder recorder(TwoStages(), 10);
	const std::array<float, 2> stages {1.0f, 0.0f};
	for (int i = 0; i < 25; ++i)
	{
		recorder.Add(static_cast<float>(i), 0.0f, 0.0f, stages);
	}
	EXPECT_EQ(recorder.Count(), 10u);
	const auto results = recorder.Summarise();
	// Frames 15 to 24
	EXPECT_NEAR(results.frame.average, 19.5f, k_Epsilon);
	EXPECT_EQ(results.frame.max, 24.0f);
	// A stage that never ran isn't listed
	ASSERT_EQ(results.stages.size(), 1u);
	recorder.Clear();
	EXPECT_EQ(recorder.Count(), 0u);
	EXPECT_EQ(recorder.Summarise().frame.count, 0u);
}

TEST(BenchmarkRecorder, WritesJsonAndCsv)
{
	Recorder recorder(TwoStages(), 4);
	const std::array<float, 2> stages {3.0f, 1.0f};
	recorder.Add(5.0f, 4.0f, 1.0f, stages, 12.0f);
	const RunInfo run {
	    .scenarioId = "benchmark.test",
	    .scenarioName = "A \"quoted\" name",
	    .build = "debug",
	    .width = 2560,
	    .height = 1440,
	    .crowd = 100,
	    .spawnMs = 12.5,
	    .spawnFrames = 2,
	    .warmUpFrames = 10,
	    .entityCounts = {{"creatures", 100}},
	};
	const auto json = ToJson(run, recorder.Summarise(), recorder.Stages());
	EXPECT_NE(json.find(R"("scenario": "benchmark.test")"), std::string::npos);
	EXPECT_NE(json.find(R"(A \"quoted\" name)"), std::string::npos);
	EXPECT_NE(json.find(R"("resolution": [2560, 1440])"), std::string::npos);
	EXPECT_NE(json.find(R"("entities": {"creatures": 100})"), std::string::npos);
	EXPECT_NE(json.find(R"("frameMs": {"mean": 5.0000)"), std::string::npos);
	EXPECT_NE(json.find(R"({"name": "Think", "kind": "update", "mean": 3.0000)"), std::string::npos);
	EXPECT_NE(json.find(R"("kind": "render")"), std::string::npos);

	const auto csv = ToCsv(run, recorder.Summarise(), recorder.Stages());
	EXPECT_EQ(csv.find("scenario,build,crowd,metric,kind,mean_ms"), 0u);
	EXPECT_NE(csv.find("benchmark.test,debug,100,Frame,total,5.0000"), std::string::npos);
	EXPECT_NE(csv.find("benchmark.test,debug,100,Draw,render,1.0000"), std::string::npos);
	// Header, frame, update, render and the two stages
	EXPECT_EQ(std::count(csv.begin(), csv.end(), '\n'), 6);
}

TEST(BenchmarkRecorder, ScalingExponentTellsLinearFromQuadratic)
{
	EXPECT_NEAR(ScalingExponent(100.0, 1.0, 1000.0, 10.0), 1.0f, k_Epsilon);
	EXPECT_NEAR(ScalingExponent(100.0, 1.0, 1000.0, 100.0), 2.0f, k_Epsilon);
	EXPECT_NEAR(ScalingExponent(100.0, 2.0, 1000.0, 2.0), 0.0f, k_Epsilon);
	EXPECT_EQ(ScalingExponent(100.0, 0.0, 1000.0, 5.0), 0.0f);
	EXPECT_EQ(ScalingExponent(100.0, 1.0, 100.0, 5.0), 0.0f);
}
