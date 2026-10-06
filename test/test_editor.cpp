/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <array>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <gtest/gtest.h>

#include "Editor/EditorMath.h"
#include "Editor/EditorOutline.h"
#include "Editor/EditorPalette.h"
#include "Editor/EditorSelection.h"

using namespace openblack;
using namespace openblack::editor;

namespace
{
constexpr float k_Epsilon = 1e-4f;

void ExpectNear(glm::vec3 actual, glm::vec3 expected, float epsilon = k_Epsilon)
{
	EXPECT_NEAR(actual.x, expected.x, epsilon);
	EXPECT_NEAR(actual.y, expected.y, epsilon);
	EXPECT_NEAR(actual.z, expected.z, epsilon);
}
} // namespace

TEST(EditorSelection, PicksClearsAndCountsChanges)
{
	EditorSelection selection;
	EXPECT_TRUE(selection.Empty());
	const auto first = static_cast<entt::entity>(4);
	selection.Select(first);
	EXPECT_TRUE(selection.IsSelected(first));
	EXPECT_EQ(selection.GetPicks(), 1u);
	// Picking the same thing again still counts, so a panel can scroll to it again
	selection.Select(first);
	EXPECT_EQ(selection.GetPicks(), 2u);
	selection.Select(entt::null);
	EXPECT_TRUE(selection.Empty());
	EXPECT_EQ(selection.GetPicks(), 3u);
	// Clearing nothing changes nothing
	selection.Clear();
	EXPECT_EQ(selection.GetPicks(), 3u);
}

TEST(EditorSelection, LetsGoOfWhatIsGone)
{
	EditorSelection selection;
	const auto entity = static_cast<entt::entity>(9);
	selection.Select(entity);
	selection.Validate([](entt::entity) { return true; });
	EXPECT_TRUE(selection.IsSelected(entity));
	selection.Validate([entity](entt::entity other) { return other != entity; });
	EXPECT_TRUE(selection.Empty());
}

TEST(EditorMath, SnapsToSteps)
{
	EXPECT_FLOAT_EQ(Snap(12.4f, 5.0f), 10.0f);
	EXPECT_FLOAT_EQ(Snap(12.6f, 5.0f), 15.0f);
	EXPECT_FLOAT_EQ(Snap(-7.6f, 5.0f), -10.0f);
	EXPECT_FLOAT_EQ(Snap(3.3f, 0.0f), 3.3f);

	Snapping snapping {.enabled = false, .move = 5.0f, .angleDegrees = 15.0f};
	ExpectNear(SnapPoint({12.4f, 3.0f, 7.6f}, snapping), {12.4f, 3.0f, 7.6f});
	snapping.enabled = true;
	// The height is the land's, never snapped
	ExpectNear(SnapPoint({12.4f, 3.3f, 7.6f}, snapping), {10.0f, 3.3f, 10.0f});
	EXPECT_NEAR(SnapAngle(glm::radians(50.0f), snapping), glm::radians(45.0f), k_Epsilon);
}

TEST(EditorMath, WrapsAngles)
{
	EXPECT_NEAR(WrapAngle(0.0f), 0.0f, k_Epsilon);
	EXPECT_NEAR(WrapAngle(glm::two_pi<float>() + 0.5f), 0.5f, k_Epsilon);
	EXPECT_NEAR(WrapAngle(-glm::two_pi<float>() - 0.5f), -0.5f, k_Epsilon);
	EXPECT_NEAR(WrapAngle(glm::pi<float>() + 0.25f), -glm::pi<float>() + 0.25f, k_Epsilon);
}

TEST(EditorMath, OrbitRoundTrips)
{
	const glm::vec3 target {100.0f, 20.0f, -50.0f};
	const Orbit orbit {.yaw = 0.7f, .pitch = 0.4f, .distance = 60.0f};
	const auto origin = OrbitOrigin(target, orbit);
	EXPECT_NEAR(glm::distance(origin, target), 60.0f, 1e-3f);
	EXPECT_GT(origin.y, target.y);
	const auto back = OrbitFrom(origin, target);
	EXPECT_NEAR(back.yaw, orbit.yaw, k_Epsilon);
	EXPECT_NEAR(back.pitch, orbit.pitch, k_Epsilon);
	EXPECT_NEAR(back.distance, orbit.distance, 1e-3f);
}

TEST(EditorMath, OrbitTurnsAndZoomsWithinBounds)
{
	Orbit orbit {.yaw = 0.0f, .pitch = 1.4f, .distance = 40.0f};
	orbit = Turn(orbit, {0.5f, 1.0f});
	EXPECT_NEAR(orbit.yaw, 0.5f, k_Epsilon);
	EXPECT_FLOAT_EQ(orbit.pitch, k_MaxOrbitPitch);
	orbit = Turn(orbit, {0.0f, -10.0f});
	EXPECT_FLOAT_EQ(orbit.pitch, k_MinOrbitPitch);

	EXPECT_NEAR(Zoom(orbit, 1.0f).distance, 36.0f, 1e-3f);
	EXPECT_NEAR(Zoom(orbit, -1.0f).distance, 40.0f / 0.9f, 1e-3f);
	EXPECT_FLOAT_EQ(Zoom(orbit, 1000.0f).distance, k_MinOrbitDistance);
	EXPECT_FLOAT_EQ(Zoom(orbit, -1000.0f).distance, k_MaxOrbitDistance);
}

TEST(EditorMath, FollowSitsBehind)
{
	// Facing north (+z), behind is to the south, which is a yaw of pi
	EXPECT_NEAR(std::abs(BehindYaw({0.0f, 1.0f}, 0.0f)), glm::pi<float>(), k_Epsilon);
	// Facing east (+x), behind is to the west
	EXPECT_NEAR(BehindYaw({1.0f, 0.0f}, 0.0f), -glm::half_pi<float>(), k_Epsilon);
	const auto origin =
	    OrbitOrigin({0.0f, 0.0f, 0.0f}, {.yaw = BehindYaw({1.0f, 0.0f}, 0.0f), .pitch = 0.0f, .distance = 10.0f});
	ExpectNear(origin, {-10.0f, 0.0f, 0.0f});
}

TEST(EditorMath, EasesTheSameHoweverTheFramesFall)
{
	const glm::vec3 start {0.0f};
	const glm::vec3 goal {100.0f, 0.0f, 0.0f};
	const auto once = EaseTowards(start, goal, 1.0f, 0.9f);
	auto inSteps = start;
	for (int i = 0; i < 10; ++i)
	{
		inSteps = EaseTowards(inSteps, goal, 0.1f, 0.9f);
	}
	ExpectNear(once, {90.0f, 0.0f, 0.0f}, 1e-3f);
	ExpectNear(inSteps, once, 1e-3f);
	ExpectNear(EaseTowards(start, goal, 0.0f, 0.9f), start);
	ExpectNear(EaseTowards(start, goal, 0.5f, 1.0f), goal);
}

TEST(EditorMath, YawRoundTrips)
{
	for (const auto yaw : {-2.5f, -1.0f, 0.0f, 0.3f, 3.0f})
	{
		EXPECT_NEAR(YawOf(YawRotation(yaw)), yaw, k_Epsilon);
		const auto facing = FacingOf(YawRotation(yaw));
		EXPECT_NEAR(facing.x, std::sin(yaw), k_Epsilon);
		EXPECT_NEAR(facing.y, std::cos(yaw), k_Epsilon);
	}
}

TEST(EditorMath, RayMeetsBoxes)
{
	const AxisAlignedBoundingBox box {.minima = {-1.0f, 0.0f, -1.0f}, .maxima = {1.0f, 2.0f, 1.0f}};
	const auto hit = RayBox({0.0f, 1.0f, -10.0f}, {0.0f, 0.0f, 1.0f}, box);
	ASSERT_TRUE(hit.has_value());
	EXPECT_NEAR(*hit, 9.0f, k_Epsilon);
	EXPECT_FALSE(RayBox({0.0f, 5.0f, -10.0f}, {0.0f, 0.0f, 1.0f}, box).has_value());
	// Pointing away
	EXPECT_FALSE(RayBox({0.0f, 1.0f, -10.0f}, {0.0f, 0.0f, -1.0f}, box).has_value());
	// Parallel to a face, outside it
	EXPECT_FALSE(RayBox({3.0f, 1.0f, -10.0f}, {0.0f, 0.0f, 1.0f}, box).has_value());
	// From inside, it is met at once
	const auto inside = RayBox({0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, box);
	ASSERT_TRUE(inside.has_value());
	EXPECT_NEAR(*inside, 0.0f, k_Epsilon);
}

TEST(EditorMath, CarriesBoxesIntoTheWorld)
{
	const AxisAlignedBoundingBox box {.minima = {-1.0f, 0.0f, -2.0f}, .maxima = {1.0f, 3.0f, 2.0f}};
	const auto world = WorldBox(box, {10.0f, 5.0f, 20.0f}, YawRotation(glm::half_pi<float>()), glm::vec3(2.0f));
	// A quarter turn swaps its width and depth
	ExpectNear(world.minima, {6.0f, 5.0f, 18.0f}, 1e-3f);
	ExpectNear(world.maxima, {14.0f, 11.0f, 22.0f}, 1e-3f);
}

TEST(EditorMath, RaysMeetLevelPlanes)
{
	const auto hit = RayLevel({0.0f, 10.0f, 0.0f}, glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f)), 0.0f);
	ASSERT_TRUE(hit.has_value());
	ExpectNear(*hit, {10.0f, 0.0f, 0.0f}, 1e-3f);
	EXPECT_FALSE(RayLevel({0.0f, 10.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.0f).has_value());
	EXPECT_FALSE(RayLevel({0.0f, 10.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.0f).has_value());
}

TEST(EditorOutline, SearchesEveryWordIgnoringCase)
{
	EXPECT_TRUE(MatchesSearch("Celtic Farmer", ""));
	EXPECT_TRUE(MatchesSearch("Celtic Farmer", "farm"));
	EXPECT_TRUE(MatchesSearch("Celtic Farmer", "FARMER celt"));
	EXPECT_FALSE(MatchesSearch("Celtic Farmer", "farmer norse"));
	EXPECT_TRUE(MatchesSearch("Celtic Farmer", "  farmer  "));
}

TEST(EditorOutline, GroupsByKindInOrder)
{
	const std::vector<OutlineEntry> entries {
	    {.entity = static_cast<entt::entity>(1), .kind = EntityKind::Tree, .label = "Oak"},
	    {.entity = static_cast<entt::entity>(2), .kind = EntityKind::Creature, .label = "Tiger"},
	    {.entity = static_cast<entt::entity>(3), .kind = EntityKind::Tree, .label = "Pine"},
	    {.entity = static_cast<entt::entity>(4), .kind = EntityKind::Villager, .label = "Celtic Farmer"},
	};
	const auto groups = Group(entries, "");
	ASSERT_EQ(groups.size(), 3u);
	EXPECT_EQ(groups.at(0).kind, EntityKind::Creature);
	EXPECT_EQ(groups.at(1).kind, EntityKind::Villager);
	EXPECT_EQ(groups.at(2).kind, EntityKind::Tree);
	EXPECT_EQ(groups.at(2).entries.size(), 2u);
	EXPECT_EQ(groups.at(2).entries.at(0)->label, "Oak");

	// A search keeps the kinds with any of their kind, counting all they have
	const auto searched = Group(entries, "pine");
	ASSERT_EQ(searched.size(), 3u);
	EXPECT_TRUE(searched.at(0).entries.empty());
	EXPECT_EQ(searched.at(2).entries.size(), 1u);
	EXPECT_EQ(searched.at(2).total, 2u);

	for (size_t i = 0; i < k_EntityKindCount; ++i)
	{
		EXPECT_FALSE(Name(static_cast<EntityKind>(i)).empty());
	}
	EXPECT_EQ(RowLabel(entries.at(0)), "Oak  #1");
}

TEST(EditorOutline, GroupsTheMiraclesDispensersAndBubblesTogether)
{
	const std::vector<OutlineEntry> entries {
	    {.entity = static_cast<entt::entity>(1), .kind = EntityKind::Miracle, .label = "Fireball"},
	    {.entity = static_cast<entt::entity>(2), .kind = EntityKind::Tree, .label = "Oak"},
	    {.entity = static_cast<entt::entity>(3), .kind = EntityKind::Miracle, .label = "Creature Spell Big"},
	};
	const auto groups = Group(entries, "");
	ASSERT_EQ(groups.size(), 2u);
	EXPECT_EQ(groups.at(1).kind, EntityKind::Miracle);
	EXPECT_EQ(groups.at(1).entries.size(), 2u);
	EXPECT_EQ(Name(EntityKind::Miracle), "Miracles");
	EXPECT_EQ(Group(entries, "big").at(0).entries.size(), 0u);
}

TEST(EditorPalette, MakesNamesReadable)
{
	EXPECT_EQ(TitleCase("OAK_TREE_A"), "Oak Tree A");
	EXPECT_EQ(TitleCase("__ODD__NAME_"), "Odd Name");
	EXPECT_EQ(TitleCase(""), "");

	const std::array<std::string_view, 3> features {"FEATURE_ROCK_A", "FEATURE_ROCK_B", "FEATURE_PILLAR"};
	EXPECT_EQ(CommonWordPrefix(features), std::string_view("FEATURE_").size());
	const auto names = ReadableNames(features);
	ASSERT_EQ(names.size(), 3u);
	EXPECT_EQ(names.at(0), "Rock A");
	EXPECT_EQ(names.at(2), "Pillar");

	// A prefix that would leave a name empty is kept
	const std::array<std::string_view, 2> short_ {"TREE_", "TREE_OAK"};
	EXPECT_EQ(CommonWordPrefix(short_), 0u);
	const std::array<std::string_view, 1> single {"TREE_OAK"};
	EXPECT_EQ(CommonWordPrefix(single), 0u);
}

TEST(EditorPalette, LaysOutInSquareRows)
{
	EXPECT_TRUE(GridLayout(0, 10.0f).empty());
	const auto one = GridLayout(1, 10.0f);
	ASSERT_EQ(one.size(), 1u);
	EXPECT_FLOAT_EQ(one.at(0).x, 0.0f);
	EXPECT_FLOAT_EQ(one.at(0).y, 0.0f);

	const auto five = GridLayout(5, 10.0f);
	ASSERT_EQ(five.size(), 5u);
	// Three columns of two rows, centred, north first
	EXPECT_FLOAT_EQ(five.at(0).x, -10.0f);
	EXPECT_FLOAT_EQ(five.at(0).y, 5.0f);
	EXPECT_FLOAT_EQ(five.at(2).x, 10.0f);
	EXPECT_FLOAT_EQ(five.at(3).y, -5.0f);
	std::set<std::pair<float, float>> distinct;
	for (const auto& point : GridLayout(30, 4.0f))
	{
		distinct.emplace(point.x, point.y);
	}
	EXPECT_EQ(distinct.size(), 30u);
}
