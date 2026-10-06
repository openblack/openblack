/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "InfoConstants.h"
#include "Magic/WorshipBattery.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
/// A site where each dancer chants 2 a turn, with a battery of 1000 and 100 more for each dancer
WorshipBatteryRules Rules()
{
	return {.chantsPerVillager = 2.0f,
	        .chantsToFillBattery = 1000.0f,
	        .eachVillagerAddToFillBattery = 100.0f,
	        .chantsToReserveForMaintaining = 50.0f};
}
} // namespace

TEST(WorshipBattery, rulesFromTables)
{
	GWorshipSiteInfo info {};
	info.chantsPerVillager = 2.0f;
	info.chantsToFillBattery = 1000.0f;
	info.eachVillagerAddToFillBattery = 100.0f;
	info.chantsToReserveForMaintaining = 50.0f;
	const auto rules = WorshipBatteryRulesFor(info, 1.5f);
	EXPECT_FLOAT_EQ(rules.chantsPerVillager, 2.0f);
	EXPECT_FLOAT_EQ(rules.chantsToFillBattery, 1000.0f);
	EXPECT_FLOAT_EQ(rules.eachVillagerAddToFillBattery, 100.0f);
	EXPECT_FLOAT_EQ(rules.chantsToReserveForMaintaining, 50.0f);
	EXPECT_FLOAT_EQ(rules.tribalPower, 1.5f);
}

TEST(WorshipBattery, capacityAndSize)
{
	auto rules = Rules();
	EXPECT_FLOAT_EQ(WorshipCapacity(rules, 10), 20.0f);
	EXPECT_FLOAT_EQ(WorshipMaxBattery(rules, 10), 2000.0f);
	EXPECT_FLOAT_EQ(WorshipMaxBattery(rules, 0), 1000.0f);
	rules.tribalPower = 2.0f;
	EXPECT_FLOAT_EQ(WorshipCapacity(rules, 10), 40.0f);
}

TEST(WorshipBattery, drawingChants)
{
	WorshipBattery site {.available = 100.0f};
	EXPECT_FLOAT_EQ(UseWorshipChants(site, 30.0f), 30.0f);
	EXPECT_FLOAT_EQ(WorshipAvailable(site), 70.0f);
	// more than is left: only what is left, but the whole request counts
	EXPECT_FLOAT_EQ(UseWorshipChants(site, 100.0f), 70.0f);
	EXPECT_FLOAT_EQ(site.used, 100.0f);
	EXPECT_FLOAT_EQ(site.requested, 130.0f);
	EXPECT_FLOAT_EQ(UseWorshipChants(site, -5.0f), 0.0f);
	EXPECT_FLOAT_EQ(site.requested, 130.0f);
}

TEST(WorshipBattery, cheats)
{
	WorshipBattery site {.available = 10.0f};
	site.infinite = true;
	EXPECT_FLOAT_EQ(WorshipAvailable(site), k_InfiniteChants);
	EXPECT_FLOAT_EQ(UseWorshipChantsIfNotInfinite(site, 500.0f), 500.0f);
	EXPECT_FLOAT_EQ(site.used, 0.0f);
	site.infinite = false;
	site.freeMaintenance = true;
	EXPECT_FLOAT_EQ(MaintainSpellFromWorship(site, 500.0f), 500.0f);
	EXPECT_FLOAT_EQ(site.used, 0.0f);
	site.freeMaintenance = false;
	EXPECT_FLOAT_EQ(MaintainSpellFromWorship(site, 500.0f), 10.0f);
}

TEST(WorshipBattery, iconsLeaveAReserveWhileSeedsAreOut)
{
	const auto rules = Rules();
	WorshipBattery site {.available = 120.0f};
	EXPECT_FLOAT_EQ(WorshipAvailableForIcons(site, rules, false), 120.0f);
	EXPECT_FLOAT_EQ(WorshipAvailableForIcons(site, rules, true), 70.0f);
	site.available = 20.0f;
	EXPECT_FLOAT_EQ(WorshipAvailableForIcons(site, rules, true), 0.0f);
}

TEST(WorshipBattery, strainAndIconShare)
{
	const auto rules = Rules();
	WorshipBattery site {.available = 100.0f, .requested = 30.0f};
	UpdateWorshipStrain(site, rules, 10); // capacity 20
	EXPECT_FLOAT_EQ(site.strain, 0.5f);
	EXPECT_FLOAT_EQ(WorshipIconShare(site, rules, 2, 40.0f, false), 0.0f);
	site.requested = 10.0f;
	UpdateWorshipStrain(site, rules, 10);
	EXPECT_FLOAT_EQ(site.strain, -0.5f);
	EXPECT_FLOAT_EQ(WorshipIconShare(site, rules, 2, 40.0f, false), 20.0f);  // all they need
	EXPECT_FLOAT_EQ(WorshipIconShare(site, rules, 2, 400.0f, false), 50.0f); // all there is
	EXPECT_FLOAT_EQ(WorshipIconShare(site, rules, 0, 40.0f, false), 0.0f);
	// no dancers: strained by any request at all
	UpdateWorshipStrain(site, rules, 0);
	EXPECT_FLOAT_EQ(site.strain, 1.0f);
	site.requested = 0.0f;
	UpdateWorshipStrain(site, rules, 0);
	EXPECT_FLOAT_EQ(site.strain, 0.0f);
}

TEST(WorshipBattery, idleSiteFillsItsBattery)
{
	// Nobody draws: the dance runs at the battery boost (0.5 empty, falling as it fills, at least 0.2 while it is not
	// full) and what is chanted goes into the battery
	const auto rules = Rules();
	WorshipBattery site;
	EndWorshipTurn(site, rules, 10);
	EXPECT_FLOAT_EQ(site.danceIntensity, 0.5f);
	EXPECT_FLOAT_EQ(site.battery, 10.0f);
	EXPECT_FLOAT_EQ(site.chantsPerDancer, 1.0f);
	EXPECT_FLOAT_EQ(site.available, 30.0f);

	site.battery = 1800.0f; // a boost of 0.05, raised to 0.2
	EndWorshipTurn(site, rules, 10);
	EXPECT_FLOAT_EQ(site.danceIntensity, 0.2f);
	EXPECT_FLOAT_EQ(site.battery, 1804.0f);

	site.battery = 2000.0f; // full: no boost, so an idle dance stops
	EndWorshipTurn(site, rules, 10);
	EXPECT_FLOAT_EQ(site.danceIntensity, 0.0f);
	EXPECT_FLOAT_EQ(site.battery, 2000.0f);
}

TEST(WorshipBattery, drawingSpeedsTheDanceAndDrainsTheBattery)
{
	const auto rules = Rules();
	WorshipBattery site {.battery = 2000.0f, .available = 2020.0f};
	UseWorshipChants(site, 50.0f); // more than the 20 the dancers make
	EndWorshipTurn(site, rules, 10);
	EXPECT_FLOAT_EQ(site.danceIntensity, 1.0f);
	EXPECT_FLOAT_EQ(site.battery, 2000.0f - 50.0f + 20.0f);
	EXPECT_FLOAT_EQ(site.used, 0.0f);
	EXPECT_FLOAT_EQ(site.requested, 0.0f);
	EXPECT_FLOAT_EQ(site.available, site.battery + 20.0f);

	// the battery never goes below empty
	site.battery = 10.0f;
	site.used = 500.0f;
	EndWorshipTurn(site, rules, 10);
	EXPECT_FLOAT_EQ(site.battery, 0.0f);

	// with no dancers nothing is chanted and the dance is at full intensity
	WorshipBattery empty {.battery = 5.0f};
	EndWorshipTurn(empty, rules, 0);
	EXPECT_FLOAT_EQ(empty.danceIntensity, 1.0f);
	EXPECT_FLOAT_EQ(empty.chantsPerDancer, 0.0f);
	EXPECT_FLOAT_EQ(empty.battery, 5.0f);
}
