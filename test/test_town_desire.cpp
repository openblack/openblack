/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "ECS/Components/TownDesire.h"
#include "ECS/TownDesire.h"
#include "Enums.h"
#include "InfoConstants.h"

using namespace openblack;
using namespace openblack::ecs::components;
namespace td = openblack::ecs::town_desire;

namespace
{
constexpr size_t D(TownDesireInfo d)
{
	return static_cast<size_t>(static_cast<int>(d));
}

/// The campaign's hours of the visual time the sky turns at: full night, dusk's start and end, full day
constexpr std::array<float, 4> k_Thresholds = {0.786f, 1.206f, 1.626f, 2.046f};

/// The sky as the game counts it, 2 at night to 0 by day
float SkyType(float hour)
{
	if (hour > 12.0f)
	{
		hour = 24.0f - hour;
	}
	const auto [a, b, c, d] = k_Thresholds;
	if (hour < a)
	{
		return 2.0f;
	}
	if (hour < b)
	{
		return 2.0f - (hour - a) / (b - a);
	}
	if (hour < c)
	{
		return 1.0f;
	}
	if (hour < d)
	{
		return 1.0f - (hour - c) / (d - c);
	}
	return 0.0f;
}

class TownDesireTest: public ::testing::Test
{
protected:
	void SetUp() override
	{
		// The game's info for towns and their desires
		_town.bedTimeMod = 0.5f;
		_town.foodWantedMultiplier = 5.0f;
		_town.minimumWoodForDesire = 500.0f;
		_town.maximumWoodForDesire = 5000.0f;
		_town.numOfBuildingsForDesiredWood = 10.0f;
		_town.relaxationMod = 2.0f;
		_town.thresholdToStartRepairing = 0.9f;
		_town.divisorForAverageDesires = 5.0f;
		_town.thresholdForAverageDesiresHelpSprites = 0.6f;
		for (auto& info : _info)
		{
			info.showsAfterPercent = 0.1f;
			info.desireTriggersVillagerAction = 0.01f;
			info.tribeMultiplier.fill(1.0f);
		}
		_info.at(D(TownDesireInfo::ForChildren)).desireTriggersVillagerAction = 0.25f;
		_info.at(D(TownDesireInfo::ToBuildWonder)).desireTriggersVillagerAction = 0.75f;
		_info.at(D(TownDesireInfo::ForRelaxation)).desireTriggersVillagerAction = 0.0f;
		_in.dayFull = k_Thresholds[3];
		SetHour(12.0f);
	}

	void SetHour(float hour)
	{
		_in.visualHour = hour;
		_in.skyType = SkyType(hour);
	}

	td::DesireContext Context() { return td::DesireContext {_desire, _in, _town, _info, 150, 250}; }

	static std::string Sorted(const std::array<float, td::k_Count>& values)
	{
		std::array<DesireSort, td::k_Count> entries {};
		for (size_t i = 0; i < td::k_Count; ++i)
		{
			entries.at(i) = {0.0f, values.at(i), static_cast<uint32_t>(i)};
		}
		td::MsvcQsort(entries);
		std::string text;
		for (const auto& e : entries)
		{
			text += (text.empty() ? "" : " ") + std::to_string(e.index);
		}
		return text;
	}

	GTownInfo _town {};
	std::array<GTownDesireInfo, 17> _info {};
	TownDesire _desire {};
	td::DesireInputs _in {};
};
} // namespace

TEST_F(TownDesireTest, NamesInOrder)
{
	const auto& table = td::Table();
	EXPECT_STREQ(table.at(D(TownDesireInfo::ForFood)).name, "Food");
	EXPECT_STREQ(table.at(D(TownDesireInfo::ForSleep)).name, "Sleep");
	EXPECT_EQ(td::FindDesire("sleep"), 16);
	EXPECT_EQ(td::FindDesire("Suppy_Workshop"), 13);
	EXPECT_EQ(td::FindDesire("Nothing"), -1);
}

TEST_F(TownDesireTest, SleepComesWithTheNight)
{
	const std::vector<std::pair<float, float>> curve = {
	    {12.0f, 0.0f}, {21.0f, 0.0f},     {22.0f, 0.371519f}, {22.5f, 2.25f}, {23.5f, 6.25f},
	    {0.5f, 2.25f}, {1.0f, 0.981043f}, {1.5f, 0.25f},      {3.0f, 0.0f},
	};
	for (const auto& [hour, expected] : curve)
	{
		SetHour(hour);
		EXPECT_NEAR(td::DesireForSleep(Context()), expected, 1e-4f) << hour;
	}
}

TEST_F(TownDesireTest, RelaxationThroughTheEvening)
{
	const std::vector<std::pair<float, float>> curve = {{12.0f, 0.1f}, {20.5f, 0.546f}, {21.5f, 1.0f}, {22.5f, 0.1f}};
	for (const auto& [hour, expected] : curve)
	{
		SetHour(hour);
		EXPECT_NEAR(td::DesireForRelaxation(Context()), expected, 1e-4f) << hour;
	}
}

TEST_F(TownDesireTest, FoodByWhatIsInStore)
{
	// Ten adults needing 850 for dinner want five times that
	_in.stats.adults = 10;
	_in.stats.foodForDinner = 850.0f;
	_in.storageFood = 2000;
	EXPECT_NEAR(td::DesireForFood(Context()), 0.529412f, 1e-5f);
	// Without a storage pit, a temporary pot's
	_in.storageFood.reset();
	_in.potFood = 300;
	EXPECT_NEAR(td::DesireForFood(Context()), 1.0f - 300.0001f / 4250.0001f, 1e-5f);
	// Enough is none
	_in.storageFood = 5000;
	EXPECT_FLOAT_EQ(td::DesireForFood(Context()), 0.0f);
}

TEST_F(TownDesireTest, AbodesByHowFullThePlacesAre)
{
	_in.stats.adults = 10;
	_in.stats.adultPlaces = 12;
	_in.stats.children = 3;
	_in.stats.childPlaces = 6;
	const float a = 10.0f / 12.00001f;
	EXPECT_NEAR(td::DesireForAbodes(Context()), a * a * a * a, 1e-5f);
	// Less while the town wants to build
	_desire.raw.at(9) = 0.5f;
	EXPECT_NEAR(td::DesireForAbodes(Context()), 0.5f * a * a * a * a, 1e-5f);
	_desire = {};
	_in.stats.adults = 15;
	_in.stats.adultPlaces = 10;
	EXPECT_FLOAT_EQ(td::DesireForAbodes(Context()), 1.0f);
}

TEST_F(TownDesireTest, PlaytimeOnceAllIsWell)
{
	_in.turn = 4000;
	EXPECT_FLOAT_EQ(td::DesireForPlaytime(Context()), 0.0f);
	_in.turn = 4001;
	EXPECT_FLOAT_EQ(td::DesireForPlaytime(Context()), 0.1f);
	_desire.desire.at(9) = 0.01f;
	EXPECT_FLOAT_EQ(td::DesireForPlaytime(Context()), 0.0f);
}

TEST_F(TownDesireTest, SortedAsTheGameSorts)
{
	// The game's unstable quicksort leaves ties in its own order
	std::array<float, 17> zero {};
	EXPECT_EQ(Sorted(zero), "8 1 2 3 4 5 6 7 0 9 10 11 12 13 14 15 16");
	std::array<float, 17> a {};
	a.at(0) = 0.3f;
	a.at(1) = 1.0f;
	a.at(8) = 0.2f;
	a.at(15) = 0.1f;
	EXPECT_EQ(Sorted(a), "1 0 8 15 5 6 7 2 4 9 3 11 12 13 14 10 16");
	const std::array<float, 17> c = {0.5f, -0.2f, 0.1f, 0.1f, 0.0f, 0.9f,  -1.0f, 0.1f, 0.25f,
	                                 0.5f, 0.0f,  0.0f, 0.3f, 0.0f, 0.75f, 0.1f,  6.25f};
	EXPECT_EQ(Sorted(c), "16 5 14 9 0 12 8 3 7 15 2 11 13 10 4 1 6");
}

TEST_F(TownDesireTest, VillagersOfferedMostWantedFirst)
{
	std::vector<size_t> asked;
	uint32_t answer = 0;
	const auto satisfy = [&](size_t d) {
		asked.push_back(d);
		return answer;
	};
	// Sleep wanted most, then food; nobody serving either
	_desire.sorted.at(0) = {0.0f, 1.0f, 16};
	_desire.sorted.at(1) = {0.0f, 0.5f, 0};
	for (size_t k = 2; k < 17; ++k)
	{
		_desire.sorted.at(k) = {0.0f, 0.0f, static_cast<uint32_t>(k == 16 ? 1 : k)};
	}
	EXPECT_EQ(td::CheckVillagerNeeded(_desire, _info, 10, false, 0.3f, satisfy), 0u);
	EXPECT_EQ(asked, (std::vector<size_t> {16, 0}));
	asked.clear();
	answer = 1;
	EXPECT_EQ(td::CheckVillagerNeeded(_desire, _info, 10, false, 0.3f, satisfy), 1u);
	EXPECT_EQ(asked, (std::vector<size_t> {16}));
	// A child can't serve food
	asked.clear();
	answer = 0;
	EXPECT_EQ(td::CheckVillagerNeeded(_desire, _info, 10, true, 0.3f, satisfy), 0u);
	EXPECT_EQ(asked, (std::vector<size_t> {16}));
	// A desire no villager can satisfy is passed over
	asked.clear();
	answer = 1;
	_desire.sorted.at(0) = {0.0f, 0.0f, 8};
	_desire.sorted.at(1) = {0.0f, 1.0f, 16};
	EXPECT_EQ(td::CheckVillagerNeeded(_desire, _info, 10, false, 0.3f, satisfy), 1u);
	EXPECT_EQ(asked, (std::vector<size_t> {16}));
	// Wanted no more than the trigger: nobody is asked
	asked.clear();
	_desire.sorted.at(0) = {0.0f, 1.0f, 16};
	EXPECT_EQ(td::CheckVillagerNeeded(_desire, _info, 10, false, 0.995f, satisfy), 0u);
	EXPECT_TRUE(asked.empty());
	// What villagers took up this turn for the desire numbered as the place counts against the place
	_desire.sorted.at(1) = {0.0f, 0.5f, 0};
	_desire.doingNow.at(0) = 10.0f;
	EXPECT_EQ(td::CheckVillagerNeeded(_desire, _info, 10, false, 0.3f, satisfy), 0u);
	EXPECT_TRUE(asked.empty());
}

TEST_F(TownDesireTest, ProcessesTheTurn)
{
	_in.stats.adults = 10;
	_in.stats.adultPlaces = 12;
	_in.stats.children = 3;
	_in.stats.childPlaces = 6;
	_in.stats.foodForDinner = 850.0f;
	_in.storageFood = 4250;
	_in.storageWood = 5000;
	_in.worshipping = 2;
	_in.onWayToWorship = 1;
	_in.turn = 51;
	_desire.desire.at(6) = 0.5f;
	_desire.doingNow.at(2) = -1.0f;
	_desire.doingNow.at(16) = 2.0f;
	const float a = 10.0f / 12.00001f;
	EXPECT_FALSE(td::Process(_desire, Context()).has_value());
	EXPECT_FLOAT_EQ(_desire.population, 10.0f);
	// Abodes read last turn's civic desire, worked out after them
	EXPECT_NEAR(_desire.raw.at(5), 0.5f * a * a * a * a, 1e-5f);
	EXPECT_FLOAT_EQ(_desire.doingNow.at(2), 0.0f);
	EXPECT_FLOAT_EQ(_desire.doingNowAtStart.at(16), 2.0f);
	EXPECT_FLOAT_EQ(_desire.desire.at(15), 0.1f);
	// Every 50 turns, how unhappy the villagers are
	_in.turn = 50;
	EXPECT_TRUE(td::Process(_desire, Context()).has_value());
}

TEST_F(TownDesireTest, NightPutsSleepFirst)
{
	_in.stats.adults = 10;
	_in.stats.adultPlaces = 12;
	_in.stats.foodForDinner = 850.0f;
	_in.storageFood = 10000;
	_in.storageWood = 10000;
	SetHour(23.5f);
	td::Process(_desire, Context());
	EXPECT_EQ(_desire.sorted.at(0).index, D(TownDesireInfo::ForSleep));
	SetHour(12.0f);
	td::Process(_desire, Context());
	EXPECT_NE(_desire.sorted.at(0).index, D(TownDesireInfo::ForSleep));
}
