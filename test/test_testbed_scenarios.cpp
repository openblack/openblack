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
#include "Creature/CreatureFeedback.h"
#include "Creature/CreatureFight.h"
#include "Creature/CreatureLayers.h"
#include "Debug/TestbedDispenserGrid.h"
#include "Debug/TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
constexpr float k_Epsilon = 1e-4f;

Command Wait(size_t creature, float delay, bool untilFree)
{
	return {.kind = Command::Kind::Stop, .creature = creature, .delaySeconds = delay, .waitUntilFree = untilFree};
}

const auto k_AlwaysFree = [](size_t) { return true; };
} // namespace

TEST(TestbedScenarios, IdsAreUniqueAndFindable)
{
	std::set<std::string_view> ids;
	for (const auto& scenario : All())
	{
		EXPECT_TRUE(ids.insert(scenario.id).second) << "Duplicate id " << scenario.id;
		EXPECT_EQ(Find(scenario.id), &scenario);
	}
	EXPECT_EQ(Find("no.such.scenario"), nullptr);
}

TEST(TestbedScenarios, EveryFacetHasScenarios)
{
	std::array<size_t, k_FacetCount> counts {};
	for (const auto& scenario : All())
	{
		++counts.at(static_cast<size_t>(scenario.facet));
	}
	for (size_t i = 0; i < k_FacetCount; ++i)
	{
		EXPECT_GT(counts.at(i), 0u) << "No scenario shows " << Name(static_cast<Facet>(i));
		EXPECT_FALSE(Name(static_cast<Facet>(i)).empty());
	}
}

TEST(TestbedScenarios, EveryScenarioIsWellFormed)
{
	for (const auto& scenario : All())
	{
		const auto problems = Problems(scenario);
		std::string all;
		for (const auto& problem : problems)
		{
			all += problem + "\n";
		}
		EXPECT_TRUE(problems.empty()) << scenario.id << ":\n" << all;
		// Ids name their facet, so the list and the command line agree
		EXPECT_EQ(scenario.id.find('.'), scenario.id.find_first_of('.'));
		EXPECT_NE(scenario.id.find('.'), std::string_view::npos) << scenario.id;
	}
}

TEST(TestbedScenarios, CoversTheCreatureFeatures)
{
	// The scenarios asked for by name, which later work must keep
	for (const auto* id : {"idle.fidget",
	                       "idle.hang_around",
	                       "expressions.desire_emotes",
	                       "expressions.faces_gestures",
	                       "senses.watch_walker",
	                       "needs.thirst",
	                       "needs.hunger",
	                       "needs.starving",
	                       "needs.sleep_day",
	                       "needs.sleep_night",
	                       "needs.poo",
	                       "needs.puke",
	                       "needs.exhaustion",
	                       "needs.cold",
	                       "growth.timelapse",
	                       "growth.morph_lineup",
	                       "appearance.alignment_skins",
	                       "growth.species_parade",
	                       "appearance.tattoos_wounds",
	                       "appearance.eyes",
	                       "light.shadows",
	                       "light.reflections",
	                       "movement.course",
	                       "movement.route_obstacles",
	                       "movement.trampling",
	                       "footprints.trail",
	                       "footprints.april_fools",
	                       "audio.surfaces",
	                       "audio.voices"})
	{
		EXPECT_NE(Find(id), nullptr) << id;
	}
	// Every species in the parade
	const auto* parade = Find("growth.species_parade");
	ASSERT_NE(parade, nullptr);
	std::set<CreatureType> species;
	for (const auto& creature : parade->creatures)
	{
		species.insert(creature.species);
	}
	EXPECT_EQ(species.size(), static_cast<size_t>(CreatureType::_COUNT) - 1);
	// Thirst needs water to find: the testbed's lake within the creature's search for it, 30 cells round it
	const auto lake = flat_land::k_LakeCentre - flat_land::k_MapMiddle;
	const auto* thirst = Find("needs.thirst");
	ASSERT_NE(thirst, nullptr);
	ASSERT_FALSE(thirst->creatures.empty());
	EXPECT_LT(glm::distance(thirst->creatures.front().offset, lake) - flat_land::k_LakeHalfExtent.y, 300.0f);
	EXPECT_NE(Find("light.reflections"), nullptr);
	EXPECT_NE(Find("movement.lake"), nullptr);
}

TEST(TestbedScenarios, NothingStartsOnTheLakesOpenWater)
{
	const auto lake = flat_land::k_LakeCentre - flat_land::k_MapMiddle;
	const auto onOpenWater = [lake](glm::vec2 offset) {
		const auto from = glm::abs(offset - lake);
		return from.x < flat_land::k_LakeHalfExtent.x && from.y < flat_land::k_LakeHalfExtent.y;
	};
	for (const auto& scenario : All())
	{
		for (const auto& creature : scenario.creatures)
		{
			EXPECT_FALSE(onOpenWater(creature.offset)) << scenario.id;
		}
		for (const auto& object : scenario.objects)
		{
			EXPECT_FALSE(onOpenWater(object.offset)) << scenario.id;
		}
		for (const auto& command : scenario.commands)
		{
			if (command.kind == Command::Kind::WalkTo || command.kind == Command::Kind::RunTo)
			{
				EXPECT_FALSE(onOpenWater(command.point)) << scenario.id;
			}
		}
	}
}

TEST(TestbedScenarios, CoversObjectsTheHandAndLeashes)
{
	for (const auto* id :
	     {"objects.reach", "objects.examine", "objects.throw_lob", "objects.eat_held", "objects.knock_down_trees",
	      "objects.point", "objects.curious", "objects.playful", "objects.angry", "hand.stroke", "hand.slap",
	      "hand.status_panel", "leash.types", "leash.pull_to_hand", "leash.tied", "leash.home"})
	{
		EXPECT_NE(Find(id), nullptr) << id;
	}
	const auto commandsOf = [](std::string_view id) {
		std::set<Command::Kind> kinds;
		for (const auto& command : Find(id)->commands)
		{
			kinds.insert(command.kind);
		}
		return kinds;
	};

	// Reaching in every direction round the creature: in front, both sides and behind
	const auto* reach = Find("objects.reach");
	bool front = false;
	bool behind = false;
	bool left = false;
	bool right = false;
	for (const auto& object : reach->objects)
	{
		front = front || object.offset.y < 0.0f;
		behind = behind || object.offset.y > 0.0f;
		left = left || object.offset.x < 0.0f;
		right = right || object.offset.x > 0.0f;
	}
	EXPECT_TRUE(front && behind && left && right);
	EXPECT_TRUE(commandsOf("objects.reach").contains(Command::Kind::PutDown));

	// Every way of looking a thing over
	std::set<size_t> ways;
	for (const auto& command : Find("objects.examine")->commands)
	{
		if (command.kind == Command::Kind::Examine)
		{
			ways.insert(command.value);
		}
	}
	EXPECT_EQ(ways.size(), 4u);
	for (const auto kind : {Command::Kind::ThrowAt, Command::Kind::Lob, Command::Kind::Discard})
	{
		EXPECT_TRUE(commandsOf("objects.throw_lob").contains(kind));
	}
	EXPECT_TRUE(commandsOf("objects.eat_held").contains(Command::Kind::EatHeld));
	EXPECT_TRUE(commandsOf("objects.knock_down_trees").contains(Command::Kind::KnockDown));
	EXPECT_TRUE(commandsOf("objects.point").contains(Command::Kind::PointAt));
	// The moods are the mind's own, with nothing told
	EXPECT_TRUE(Find("objects.curious")->commands.empty());
	EXPECT_FALSE(Find("objects.angry")->creatures.front().desires.empty());

	// Every part of the body stroked; slaps high and low, gentle and hard
	std::set<size_t> parts;
	for (const auto& command : Find("hand.stroke")->commands)
	{
		if (command.kind == Command::Kind::HandStroke)
		{
			parts.insert(command.bodyPart);
		}
	}
	EXPECT_EQ(parts.size(), creature_feedback::k_BodyPartCount);
	std::set<std::pair<bool, bool>> slaps;
	for (const auto& command : Find("hand.slap")->commands)
	{
		if (command.kind == Command::Kind::HandSlap)
		{
			slaps.insert({command.gentle, command.slapHeight < creature_feedback::k_FeetBelow});
		}
	}
	EXPECT_EQ(slaps.size(), 4u);
	EXPECT_TRUE(commandsOf("hand.slap").contains(Command::Kind::HandLetGo));

	// The status panel's creature has needs held to read off it
	const auto& worn = Find("hand.status_panel")->creatures.front();
	EXPECT_TRUE(worn.hold);
	EXPECT_TRUE(worn.needs.life.has_value() && worn.needs.energy.has_value() && worn.needs.exhaustion.has_value());

	// Every leash, tied up and kept at home
	std::set<LeashType> leashes;
	for (const auto& command : Find("leash.types")->commands)
	{
		if (command.kind == Command::Kind::PutOnLeash)
		{
			leashes.insert(command.leash);
		}
	}
	EXPECT_EQ(leashes, (std::set<LeashType> {LeashType::Evil, LeashType::Rope, LeashType::Good}));
	EXPECT_TRUE(commandsOf("leash.tied").contains(Command::Kind::TieLeash));
	EXPECT_TRUE(commandsOf("leash.tied").contains(Command::Kind::UntieLeash));
	EXPECT_TRUE(commandsOf("leash.home").contains(Command::Kind::ConfineToHome));
}

TEST(TestbedScenarios, CoversFights)
{
	const auto commandsOf = [](std::string_view id) {
		std::set<Command::Kind> kinds;
		const auto* scenario = Find(id);
		EXPECT_NE(scenario, nullptr) << id;
		if (scenario != nullptr)
		{
			for (const auto& command : scenario->commands)
			{
				kinds.insert(command.kind);
			}
		}
		return kinds;
	};
	EXPECT_TRUE(commandsOf("combat.ai_duel").contains(Command::Kind::FightAuto));
	EXPECT_TRUE(commandsOf("combat.player").contains(Command::Kind::FightBlow));
	EXPECT_TRUE(commandsOf("combat.player").contains(Command::Kind::FightStep));
	EXPECT_TRUE(commandsOf("combat.blocking").contains(Command::Kind::FightBlock));
	EXPECT_TRUE(commandsOf("combat.faint_recovery").contains(Command::Kind::KnockOut));
	EXPECT_TRUE(commandsOf("combat.leash").contains(Command::Kind::TieLeashToCreature));

	// Blows let go at once and held the full charge
	std::set<float> charges;
	for (const auto& command : Find("combat.charged")->commands)
	{
		if (command.kind == Command::Kind::FightBlow)
		{
			charges.insert(command.chargeMs);
		}
	}
	EXPECT_EQ(charges, (std::set<float> {0.0f, creature_fight::k_MaxChargeMs}));
	// Only the scenario about it lets anger start fights, so other angry creatures keep to themselves
	for (const auto& scenario : All())
	{
		EXPECT_EQ(scenario.environment.angerStartsFights, scenario.id == "combat.anger") << scenario.id;
	}
	// The worn creature is left to rest
	EXPECT_FALSE(Find("combat.faint_recovery")->creatures.front().hold);
}

TEST(TestbedScenarios, FightCommandsAreChecked)
{
	using Kind = Command::Kind;
	Scenario scenario {
	    .id = "test.fight",
	    .name = "Fight",
	    .description = "A fight",
	    .expected = "Blows",
	    .creatures = {{.species = CreatureType::Tiger}, {.species = CreatureType::Tiger, .offset = {40.0f, 0.0f}}},
	    .commands = {{.kind = Kind::StartFight, .value = 1},
	                 {.kind = Kind::FightBlow, .value = 2, .chargeMs = 1200.0f},
	                 {.kind = Kind::FightStep, .value = 3},
	                 {.kind = Kind::TieLeashToCreature, .value = 1}},
	};
	EXPECT_TRUE(Problems(scenario).empty());
	scenario.commands = {{.kind = Kind::StartFight, .value = 0},
	                     {.kind = Kind::FightBlow, .value = 3},
	                     {.kind = Kind::FightBlow, .chargeMs = 2000.0f},
	                     {.kind = Kind::FightStep, .value = 4},
	                     {.kind = Kind::TieLeashToCreature, .value = 2}};
	EXPECT_EQ(Problems(scenario).size(), 5u);
}

TEST(TestbedScenarios, EveryCommandHasAName)
{
	for (size_t i = 0; i <= static_cast<size_t>(Command::Kind::TieLeashToCreature); ++i)
	{
		EXPECT_FALSE(Name(static_cast<Command::Kind>(i)).empty()) << i;
	}
}

TEST(TestbedScenarios, CommandsOnThingsAreChecked)
{
	using Kind = Command::Kind;
	Scenario broken {
	    .id = "broken.things",
	    .name = "Broken",
	    .description = "Broken on purpose",
	    .expected = "Every problem found",
	    .creatures = {CreatureSetup {}},
	    .objects = {{.type = FeatureInfo::FatPilarChalk}, {.type = MobileObjectInfo::Ball}},
	    .commands = {{.kind = Kind::PickUp, .object = 0},
	                 {.kind = Kind::KnockDown, .object = 0},
	                 {.kind = Kind::TieLeash, .object = 2},
	                 {.kind = Kind::Examine, .value = 4},
	                 {.kind = Kind::HandStroke, .bodyPart = creature_feedback::k_BodyPartCount},
	                 {.kind = Kind::HandSlap, .slapHeight = 2.0f},
	                 {.kind = Kind::PutOnLeash, .leash = LeashType::None},
	                 {.kind = Kind::ConfineToHome, .radius = 0.0f}},
	};
	EXPECT_EQ(Problems(broken).size(), 8u);

	// Picking up a thing, knocking it down and tying to anything are fine
	broken.commands = {{.kind = Kind::PickUp, .object = 1},
	                   {.kind = Kind::KnockDown, .object = 1},
	                   {.kind = Kind::TieLeash, .object = 0},
	                   {.kind = Kind::PutOnLeash, .leash = LeashType::Good},
	                   {.kind = Kind::ConfineToHome, .radius = 10.0f}};
	EXPECT_TRUE(Problems(broken).empty());
}

TEST(TestbedScenarios, PointerCommandsAreChecked)
{
	using Kind = Command::Kind;
	// The mouse needs no creature, but its points are on the screen and its buttons are the mouse's three
	Scenario mouse {
	    .id = "broken.mouse",
	    .name = "Broken",
	    .description = "Broken on purpose",
	    .expected = "Every problem found",
	    .commands = {{.kind = Kind::PointerTo, .point = {1.5f, 0.5f}},
	                 {.kind = Kind::PointerPress, .value = 0},
	                 {.kind = Kind::PointerRelease, .value = 4},
	                 {.kind = Kind::PointerSweep, .point = {0.1f, 0.0f}, .amount = 0.0f}},
	};
	EXPECT_EQ(Problems(mouse).size(), 4u);

	mouse.commands = {{.kind = Kind::PointerTo, .point = {0.75f, 0.5f}},
	                  {.kind = Kind::PointerPress, .value = 2},
	                  {.kind = Kind::PointerSweep, .point = {-0.3f, 0.0f}, .amount = 1.5f},
	                  {.kind = Kind::PointerRelease, .value = 2},
	                  {.kind = Kind::WheelTurn, .value = 3}};
	EXPECT_TRUE(Problems(mouse).empty());
	EXPECT_TRUE(IsPointerCommand(Kind::WheelTurn));
	EXPECT_FALSE(IsPointerCommand(Kind::WalkTo));
}

TEST(TestbedScenarios, CoversTheHandFindingItsWay)
{
	for (const auto* id : {"hand.rotate_release", "hand.two_buttons", "hand.drag", "hand.zoom", "hand.click"})
	{
		const auto* scenario = Find(id);
		ASSERT_NE(scenario, nullptr) << id;
		EXPECT_EQ(scenario->facet, Facet::Hand) << id;
		EXPECT_TRUE(scenario->creatures.empty()) << id;
		EXPECT_TRUE(std::ranges::all_of(scenario->commands, [](const Command& command) {
			return IsPointerCommand(command.kind);
		})) << id;
	}
}

TEST(TestbedScenarios, ProblemsAreFound)
{
	Scenario broken {
	    .id = "broken",
	    .name = "Broken",
	    .description = "Broken on purpose",
	    .expected = "Every problem found",
	    .framing = {.shot = Shot::Follow, .creature = 3},
	    .creatures = {CreatureSetup {.species = CreatureType::Unknown, .desires = {{.fraction = 2.0f}}}},
	    .objects = {{.type = TreeInfo::_COUNT}},
	    .commands = {{.kind = Command::Kind::PlayAction, .creature = 1, .value = creature_layers::animations::k_FirstFace},
	                 {.kind = Command::Kind::Follow, .creature = 0, .value = 0}},
	    .repeatFrom = 5,
	};
	// The framing, species, desire, object, command creature, animation, follow and repeat
	EXPECT_GE(Problems(broken).size(), 8u);
}

TEST(TestbedScenarios, BoundsTakeInEveryPoint)
{
	const std::array<glm::vec2, 3> points {glm::vec2 {-10.0f, 0.0f}, glm::vec2 {30.0f, 20.0f}, glm::vec2 {10.0f, -40.0f}};
	const auto bounds = BoundsOf(points, glm::vec2(5.0f));
	EXPECT_NEAR(bounds.centre.x, 10.0f, k_Epsilon);
	EXPECT_NEAR(bounds.centre.y, -10.0f, k_Epsilon);
	EXPECT_NEAR(bounds.halfSize.x, 20.0f, k_Epsilon);
	EXPECT_NEAR(bounds.halfSize.y, 30.0f, k_Epsilon);
	// A single point, or none, still gets the least size
	const std::array<glm::vec2, 1> one {glm::vec2 {3.0f, 4.0f}};
	EXPECT_EQ(BoundsOf(one, glm::vec2(30.0f)).halfSize, glm::vec2(30.0f));
	EXPECT_EQ(BoundsOf(one, glm::vec2(30.0f)).centre, one.front());
	EXPECT_EQ(BoundsOf({}, glm::vec2(12.0f)).halfSize, glm::vec2(12.0f));
}

TEST(TestbedScenarios, OverviewSeesTheWholeBox)
{
	const glm::vec3 centre {100.0f, 5.0f, 200.0f};
	const glm::vec2 halfSize {120.0f, 30.0f};
	const glm::vec2 fieldOfView {1.2f, 0.7f};
	const auto placement = Overview(centre, halfSize, fieldOfView, 1.0f);
	EXPECT_EQ(placement.focus, centre);
	// From the south, above
	EXPECT_LT(placement.origin.z, centre.z);
	EXPECT_GT(placement.origin.y, centre.y);
	// Far enough back that the box's ends are within half the field of view across either side
	const auto distance = glm::distance(placement.origin, centre);
	EXPECT_LE(std::atan(halfSize.x / distance), (fieldOfView.x * 0.5f) + k_Epsilon);
	// A deep box is framed by the view up and down instead
	const auto deep = Overview(centre, {10.0f, 300.0f}, fieldOfView, 1.0f);
	EXPECT_GT(glm::distance(deep.origin, centre), distance);
	// Further for more
	EXPECT_GT(glm::distance(Overview(centre, halfSize, fieldOfView, 2.0f).origin, centre), distance * 1.9f);
}

TEST(TestbedScenarios, FollowAndHeadShotsScaleWithTheCreature)
{
	const glm::vec3 at {10.0f, 0.0f, 10.0f};
	const auto small = Follow(at, CreatureHeight(0.5f), 1.0f);
	const auto big = Follow(at, CreatureHeight(2.0f), 1.0f);
	EXPECT_GT(glm::distance(big.origin, big.focus), glm::distance(small.origin, small.focus));
	EXPECT_LT(small.origin.z, at.z);

	// In front of the face, looking back at it, at head height
	const glm::vec2 ahead {1.0f, 0.0f};
	const auto height = CreatureHeight(1.0f);
	const auto head = Head(at, ahead, height, 1.0f);
	EXPECT_GT(head.origin.x, head.focus.x);
	EXPECT_GT(head.focus.y, at.y + (height * 0.5f));
	EXPECT_NEAR(head.origin.z, at.z, k_Epsilon);
	// An unnormalised or missing facing still works
	EXPECT_NEAR(Head(at, {4.0f, 0.0f}, height, 1.0f).origin.x, head.origin.x, k_Epsilon);
	EXPECT_LT(Head(at, {0.0f, 0.0f}, height, 1.0f).origin.z, at.z);
}

TEST(TestbedScenarios, NeedOverridesApplyOnlyWhatIsGiven)
{
	creature_physiology::Needs needs {};
	needs.energy = 0.5f;
	needs.life = 0.7f;
	Apply(NeedOverrides {.dehydration = 0.9f, .warmth = -3.0f, .age = 12}, needs, 1.0f);
	EXPECT_FLOAT_EQ(needs.dehydration, 0.9f);
	EXPECT_FLOAT_EQ(needs.warmth, -1.0f);
	EXPECT_EQ(needs.age, 12u);
	EXPECT_FLOAT_EQ(needs.energy, 0.5f);
	EXPECT_FLOAT_EQ(needs.life, 0.7f);

	// Energy fills up to the creature's size, as a meal does
	Apply(NeedOverrides {.energy = 1.8f}, needs, 1.0f);
	EXPECT_FLOAT_EQ(needs.energy, 1.0f);
	Apply(NeedOverrides {.energy = 1.8f}, needs, 2.0f);
	EXPECT_FLOAT_EQ(needs.energy, 1.8f);
	EXPECT_TRUE(NeedOverrides {}.Empty());
	EXPECT_FALSE(NeedOverrides {.poo = 0.0f}.Empty());
}

TEST(TestbedScenarios, DesireOverridesAreFractionsOfTheirMaxima)
{
	creature_desires::Desires desires {};
	auto& water = desires[creature_desires::Desire::Water];
	water.max = 4.0f;
	water.activated = false;
	desires[creature_desires::Desire::Hunger].max = 2.0f;
	const std::array<DesireOverride, 2> overrides {{
	    {.desire = creature_desires::Desire::Water, .fraction = 0.5f},
	    {.desire = creature_desires::Desire::Hunger, .fraction = 1.5f},
	}};
	Apply(overrides, desires);
	EXPECT_TRUE(water.activated);
	EXPECT_FLOAT_EQ(water.value, 2.0f);
	EXPECT_FLOAT_EQ(desires[creature_desires::Desire::Hunger].value, 2.0f);
	EXPECT_FLOAT_EQ(desires[creature_desires::Desire::Fear].value, 0.0f);
}

TEST(TestbedScenarios, TimelineGivesCommandsAsTheirDelaysPass)
{
	const std::vector<Command> commands {Wait(0, 1.0f, false), Wait(0, 0.0f, false), Wait(0, 2.0f, false)};
	Timeline timeline;
	EXPECT_TRUE(Advance(timeline, commands, std::nullopt, 0.5f, k_AlwaysFree).empty());
	// The second follows the first at once
	EXPECT_EQ(Advance(timeline, commands, std::nullopt, 0.5f, k_AlwaysFree), (std::vector<size_t> {0, 1}));
	EXPECT_TRUE(Advance(timeline, commands, std::nullopt, 1.9f, k_AlwaysFree).empty());
	EXPECT_EQ(Advance(timeline, commands, std::nullopt, 0.1f, k_AlwaysFree), (std::vector<size_t> {2}));
	EXPECT_TRUE(timeline.done);
	EXPECT_TRUE(Advance(timeline, commands, std::nullopt, 10.0f, k_AlwaysFree).empty());
}

TEST(TestbedScenarios, TimelineWaitsForTheCreatureToBeFree)
{
	const std::vector<Command> commands {Wait(0, 0.0f, false), Wait(1, 1.0f, true)};
	Timeline timeline;
	bool free = false;
	const auto isFree = [&free](size_t creature) {
		EXPECT_EQ(creature, 1u);
		return free;
	};
	EXPECT_EQ(Advance(timeline, commands, std::nullopt, 0.1f, isFree), (std::vector<size_t> {0}));
	// Busy for a long time: the delay only counts once it is free
	EXPECT_TRUE(Advance(timeline, commands, std::nullopt, 5.0f, isFree).empty());
	free = true;
	EXPECT_TRUE(Advance(timeline, commands, std::nullopt, 0.5f, isFree).empty());
	// Once it has been free, being busy again (an idle fidget) doesn't hold the command up
	free = false;
	EXPECT_TRUE(Advance(timeline, commands, std::nullopt, 0.4f, isFree).empty());
	EXPECT_EQ(Advance(timeline, commands, std::nullopt, 0.2f, isFree), (std::vector<size_t> {1}));
}

TEST(TestbedScenarios, TimelineGivesTheLastCommandTimeToStart)
{
	// A creature just told to walk hasn't started yet; the next command doesn't take it as free at once
	const std::vector<Command> commands {Wait(0, 0.0f, false), Wait(0, 0.0f, true)};
	Timeline timeline;
	EXPECT_EQ(Advance(timeline, commands, std::nullopt, 0.0f, k_AlwaysFree), (std::vector<size_t> {0}));
	EXPECT_TRUE(Advance(timeline, commands, std::nullopt, 0.1f, k_AlwaysFree).empty());
	EXPECT_EQ(Advance(timeline, commands, std::nullopt, 1.0f, k_AlwaysFree), (std::vector<size_t> {1}));
}

TEST(TestbedScenarios, TimelineGoesRoundAgain)
{
	const std::vector<Command> commands {Wait(0, 1.0f, false), Wait(0, 1.0f, false), Wait(0, 1.0f, false)};
	Timeline timeline;
	std::vector<size_t> given;
	for (int i = 0; i < 7; ++i)
	{
		const auto due = Advance(timeline, commands, 1, 1.0f, k_AlwaysFree);
		given.insert(given.end(), due.begin(), due.end());
	}
	EXPECT_EQ(given, (std::vector<size_t> {0, 1, 2, 1, 2, 1, 2}));
	EXPECT_FALSE(timeline.done);

	// Commands without delays that go round again don't run away within one frame
	const std::vector<Command> instant {Wait(0, 0.0f, false), Wait(0, 0.0f, false)};
	Timeline spinning;
	EXPECT_EQ(Advance(spinning, instant, 0, 0.1f, k_AlwaysFree).size(), instant.size());
}

TEST(TestbedScenarios, MapPointsAreFromTheMiddle)
{
	EXPECT_EQ(MapPoint({2560.0f, 2560.0f}, {10.0f, -20.0f}), glm::vec2(2570.0f, 2540.0f));
	EXPECT_FLOAT_EQ(CreatureHeight(1.0f), 15.0f);
	EXPECT_FLOAT_EQ(CreatureHeight(2.0f), 30.0f);
}

TEST(TestbedDispenserGrid, HasOneDispenserOfEveryMiracleTheCreatureSpellsAmongThem)
{
	const auto types = testbed_dispensers::GridMagicTypes();
	const std::set<MagicType> unique(types.begin(), types.end());
	EXPECT_EQ(unique.size(), types.size());
	for (const auto spell : {MagicType::CreatureSpellFreeze, MagicType::CreatureSpellSmall, MagicType::CreatureSpellBig,
	                         MagicType::CreatureSpellWeak, MagicType::CreatureSpellStrong, MagicType::CreatureSpellInvisible,
	                         MagicType::CreatureSpellCompassion, MagicType::CreatureSpellAngry, MagicType::CreatureSpellItchy})
	{
		EXPECT_TRUE(unique.contains(spell));
	}
	for (const auto miracle : {MagicType::Fireball, MagicType::LightningBolt, MagicType::Heal, MagicType::Food, MagicType::Wood,
	                           MagicType::Water, MagicType::Shield})
	{
		EXPECT_TRUE(unique.contains(miracle));
	}
}

TEST(TestbedDispenserGrid, LaysOutRowsSouthOfTheMiddleClearOfTheLake)
{
	// The testbed's lake lies north of the middle, its shore reaching out round its open water
	const float lakeSouthEdge =
	    static_cast<float>(flat_land::k_LakeMinZ - flat_land::k_ShoreCells) * flat_land::k_CellSize - flat_land::k_MapMiddle.y;
	// The testbed's camera starts south of the middle
	constexpr float k_CameraBack = 120.0f;
	using testbed_dispensers::k_GridColumns;
	using testbed_dispensers::k_GridOrigin;
	using testbed_dispensers::k_GridSpacing;
	const auto count = testbed_dispensers::GridMagicTypes().size();
	const auto offsets = testbed_dispensers::GridOffsets(count);
	ASSERT_EQ(offsets.size(), count);
	EXPECT_EQ(offsets.front(), k_GridOrigin);
	// Along a row eastwards, then the next row a step south
	EXPECT_EQ(offsets.at(1), k_GridOrigin + glm::vec2(k_GridSpacing.x, 0.0f));
	EXPECT_EQ(offsets.at(k_GridColumns), k_GridOrigin + glm::vec2(0.0f, k_GridSpacing.y));
	for (size_t i = 0; i < offsets.size(); ++i)
	{
		EXPECT_LT(offsets[i].y, 0.0f) << i;
		EXPECT_LT(offsets[i].y, lakeSouthEdge) << i;
		EXPECT_GT(offsets[i].y, -k_CameraBack + testbed_dispensers::k_GridClearance) << i;
		EXPECT_TRUE(testbed_dispensers::InGridArea(offsets[i])) << i;
		for (size_t j = i + 1; j < offsets.size(); ++j)
		{
			// Far enough apart for the hand to tap each bubble
			EXPECT_GE(glm::distance(offsets[i], offsets[j]),
			          std::min(std::abs(k_GridSpacing.x), std::abs(k_GridSpacing.y)) - k_Epsilon);
		}
	}
	// Centred east to west on the middle
	EXPECT_NEAR(k_GridOrigin.x + k_GridSpacing.x * static_cast<float>(k_GridColumns - 1) * 0.5f, 0.0f, k_Epsilon);
}

TEST(TestbedScenarios, CoversTheMiracles)
{
	std::set<MagicType> cast;
	bool dispensers = false;
	for (const auto& scenario : All())
	{
		if (scenario.facet != Facet::Miracles)
		{
			continue;
		}
		dispensers = dispensers || scenario.id == "miracles.dispensers";
		for (const auto& miracle : scenario.miracles)
		{
			cast.insert(miracle.type);
		}
	}
	EXPECT_TRUE(dispensers);
	for (const auto miracle :
	     {MagicType::Fireball, MagicType::LightningBolt, MagicType::Heal, MagicType::Food, MagicType::Wood, MagicType::Water,
	      MagicType::Shield, MagicType::CreatureSpellFreeze, MagicType::CreatureSpellSmall, MagicType::CreatureSpellBig,
	      MagicType::CreatureSpellWeak, MagicType::CreatureSpellStrong, MagicType::CreatureSpellInvisible,
	      MagicType::CreatureSpellCompassion, MagicType::CreatureSpellAngry, MagicType::CreatureSpellItchy})
	{
		EXPECT_TRUE(cast.contains(miracle)) << static_cast<int>(miracle);
	}
}

TEST(TestbedDispenserGrid, StaysUnlessAScenarioAsksForNoneOrStandsOnIt)
{
	Scenario scenario;
	EXPECT_TRUE(KeepsDispenserGrid(scenario));
	EXPECT_FALSE(testbed_dispensers::InGridArea({0.0f, 0.0f}));
	scenario.creatures.push_back({.offset = {0.0f, 30.0f}});
	EXPECT_TRUE(KeepsDispenserGrid(scenario));
	scenario.objects.push_back({.offset = testbed_dispensers::k_GridOrigin});
	EXPECT_FALSE(KeepsDispenserGrid(scenario));
	scenario.objects.clear();
	scenario.environment.dispenserGrid = false;
	EXPECT_FALSE(KeepsDispenserGrid(scenario));
	// The scenario of the dispensers keeps them
	const auto* dispensers = Find("miracles.dispensers");
	ASSERT_NE(dispensers, nullptr);
	EXPECT_TRUE(KeepsDispenserGrid(*dispensers));
}

TEST(TestbedScenarios, BenchmarksMeasureCrowdsOfEachSize)
{
	const std::array<std::pair<const char*, size_t>, 8> benchmarks {{
	    {"benchmark.creatures_10", 10},
	    {"benchmark.creatures_25", 25},
	    {"benchmark.creatures_50", 50},
	    {"benchmark.creatures_100", 100},
	    {"benchmark.villagers_100", 100},
	    {"benchmark.villagers_250", 250},
	    {"benchmark.villagers_500", 500},
	    {"benchmark.villagers_1000", 1'000},
	}};
	for (const auto& [id, count] : benchmarks)
	{
		const auto* scenario = Find(id);
		ASSERT_NE(scenario, nullptr) << id;
		EXPECT_EQ(scenario->facet, Facet::Benchmark);
		ASSERT_TRUE(scenario->crowd.has_value()) << id;
		EXPECT_EQ(scenario->crowd->count, count);
		EXPECT_GT(scenario->crowd->perFrame, 0u);
		EXPECT_EQ(scenario->crowd->kind, std::string_view(id).find("creatures") != std::string_view::npos
		                                     ? Crowd::Kind::Creatures
		                                     : Crowd::Kind::Villagers);
		EXPECT_TRUE(Problems(*scenario).empty()) << id;
		// Nothing else on the land, so that only the crowd's size changes between them, and the same layout each run
		EXPECT_TRUE(scenario->creatures.empty());
		EXPECT_TRUE(scenario->objects.empty());
		EXPECT_TRUE(scenario->commands.empty());
		EXPECT_FALSE(scenario->environment.dispenserGrid);
		EXPECT_EQ(scenario->crowd->seed, Find("benchmark.creatures_100")->crowd->seed);
	}
	EXPECT_EQ(Name(Facet::Benchmark), "Benchmark");
	// No more than 100 creatures or 1,000 villagers
	size_t crowds = 0;
	for (const auto& scenario : All())
	{
		if (scenario.crowd.has_value())
		{
			++crowds;
			EXPECT_LE(scenario.crowd->count, scenario.crowd->kind == Crowd::Kind::Creatures ? 100u : 1'000u) << scenario.id;
		}
	}
	EXPECT_EQ(crowds, benchmarks.size());
	// Only benchmarks have crowds
	for (const auto& scenario : All())
	{
		EXPECT_EQ(scenario.crowd.has_value(), scenario.facet == Facet::Benchmark) << scenario.id;
	}
}

TEST(TestbedScenarios, CrowdsAreChecked)
{
	Scenario scenario {.id = "benchmark.bad", .name = "Bad", .description = "-", .expected = "-"};
	scenario.environment.dispenserGrid = false;
	EXPECT_FALSE(Problems(scenario).empty());
	scenario.crowd = Crowd {.count = 10, .perFrame = 5};
	EXPECT_TRUE(Problems(scenario).empty());
	scenario.crowd->perFrame = 0;
	EXPECT_FALSE(Problems(scenario).empty());
	scenario.crowd = Crowd {.count = 0, .perFrame = 5};
	EXPECT_FALSE(Problems(scenario).empty());
}
