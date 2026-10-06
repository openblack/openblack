/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TestbedScenarioRegistry.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <numbers>

#include <fmt/format.h>
#include <glm/geometric.hpp>

#include "3D/FlatLand.h"
#include "Creature/CreatureCave.h"
#include "Creature/CreatureFace.h"
#include "Creature/CreatureFeedback.h"
#include "Creature/CreatureFight.h"
#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureObjectActions.h"
#include "Particles/ParticleTypes.h"
#include "TestbedDispenserGrid.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

// How to add a scenario: append one to the list Build() returns, in the section of its facet. Give it a new id
// ("facet.what", never reused or renamed, as tests and the command line pick scenarios by it), a name, its facet, a
// description of what it sets up and what to look for, and then its data: the environment (land, hour, weather, body
// time), the framing, the creatures with the bodies, needs and desires they start with, the objects on the land, and
// the commands, which play in turn (commands on objects name them by their place in the scenario's objects). Offsets are from
// the middle of the map, x east and y north; the testbed's camera looks north from 120 units south of the middle, and the lake
// lies beyond the middle (k_Lake, its open water 100 units a side, ringed by 20 units of shallows and a bank). Keep other
// scenarios clear of it. A new facet goes into the Facet enum and its Name. test_testbed_scenarios
// checks the data of every scenario; anything a scenario needs that the runner can't yet do goes into these types and
// TestbedScenarioRunner together.

namespace
{
using Kind = Command::Kind;
using creature_desires::Desire;
namespace animations = creature_layers::animations;

/// How far apart the creatures of a lineup stand
constexpr float k_RowSpacing = 32.0f;
/// The highest hour of the day
constexpr float k_HoursPerDay = 24.0f;
/// Offsets further from the middle of the map than this would be off it
constexpr float k_MaxOffset = 2400.0f;
/// A command that waits for its creature to be free first gives the last command this long to get it going
constexpr float k_SettleSeconds = 0.5f;
/// The last stage of growing up, and how many skills, miracles and deeds to copy the game's tables have
constexpr size_t k_LastPhase = 13;
constexpr size_t k_Skills = 6;
constexpr size_t k_Miracles = 42;
constexpr size_t k_Deeds = 46;
/// The seed of every benchmark's crowd, so that runs lay it out the same
constexpr uint32_t k_BenchmarkSeed = 2026;
/// The testbed's lake, from the middle of the map: the middle of its open water, half its width, and the width of the
/// shallows round it
constexpr glm::vec2 k_Lake = flat_land::k_LakeCentre - flat_land::k_MapMiddle;
constexpr glm::vec2 k_LakeHalf = flat_land::k_LakeHalfExtent;
constexpr float k_ShallowsWidth = 20.0f;
/// The near and far shallows of the lake, as the testbed's camera looks north across it
constexpr float k_NearShallows = k_Lake.y - k_LakeHalf.y - (k_ShallowsWidth * 0.5f);
constexpr float k_FarShallows = k_Lake.y + k_LakeHalf.y + (k_ShallowsWidth * 0.5f);

/// The lake's open water, for the overview to keep in view
std::vector<glm::vec2> LakeInView()
{
	return {k_Lake - k_LakeHalf, k_Lake + k_LakeHalf};
}

Command Go(Kind kind, size_t creature, glm::vec2 point, float delay = 0.0f, bool wait = true)
{
	return {.kind = kind, .creature = creature, .delaySeconds = delay, .waitUntilFree = wait, .point = point};
}

Command Play(Kind kind, size_t creature, size_t animation, float delay, bool wait)
{
	return {.kind = kind, .creature = creature, .delaySeconds = delay, .waitUntilFree = wait, .value = animation};
}

Command Act(Kind kind, size_t creature, float delay = 0.0f, bool wait = false)
{
	return {.kind = kind, .creature = creature, .delaySeconds = delay, .waitUntilFree = wait};
}

Command AtHour(float hour, float delay)
{
	return {.kind = Kind::SetHour, .delaySeconds = delay, .hour = hour};
}

/// A command on one of the scenario's objects: picking it up, knocking it down, tying the leash to it
Command OnObject(Kind kind, size_t creature, size_t object, float delay = 0.0f, bool wait = true)
{
	return {.kind = kind, .creature = creature, .delaySeconds = delay, .waitUntilFree = wait, .object = object};
}

Command Stroke(size_t creature, creature_feedback::BodyPart part, float delay)
{
	return {.kind = Kind::HandStroke,
	        .creature = creature,
	        .delaySeconds = delay,
	        .waitUntilFree = true,
	        .bodyPart = static_cast<size_t>(part)};
}

Command Slap(size_t creature, float height, bool gentle, bool sweepsRight, float delay)
{
	return {.kind = Kind::HandSlap,
	        .creature = creature,
	        .delaySeconds = delay,
	        .waitUntilFree = true,
	        .slapHeight = height,
	        .gentle = gentle,
	        .sweepsRight = sweepsRight};
}

/// Pulling the face the creature's mind would for a feeling
Command Feel(size_t creature, creature_face::Cue cue, float delay)
{
	return {.kind = Kind::ShowFeeling, .creature = creature, .delaySeconds = delay, .value = static_cast<size_t>(cue)};
}

Command Leash(size_t creature, LeashType type, float delay)
{
	return {.kind = Kind::PutOnLeash, .creature = creature, .delaySeconds = delay, .leash = type};
}

/// A body wanting nothing: full, rested, watered, comfortable
NeedOverrides Content()
{
	return {.energy = 1.0f, .exhaustion = 0.0f, .dehydration = 0.0f, .poo = 0.0f, .life = 1.0f, .warmth = 0.0f};
}

/// Every desire gone but one, which is as strong as it gets
std::vector<DesireOverride> OnlyDesire(Desire wanted)
{
	std::vector<DesireOverride> overrides;
	for (size_t i = 0; i < creature_desires::k_DesireCount; ++i)
	{
		const auto desire = static_cast<Desire>(i);
		overrides.push_back({.desire = desire, .fraction = desire == wanted ? 1.0f : 0.0f});
	}
	return overrides;
}

/// A creature kept content, so that its needs stay out of what the scenario shows
CreatureSetup Content(CreatureType species, glm::vec2 offset, float facing = 0.0f, std::string_view label = {})
{
	return {.label = label, .species = species, .offset = offset, .facingDegrees = facing, .needs = Content(), .hold = true};
}

/// A content creature whose mind is paused, so that it only does what the scenario tells it: its idle mind would
/// otherwise wander off or sit down in the middle of a walk
CreatureSetup Posed(CreatureType species, glm::vec2 offset, float facing = 0.0f, std::string_view label = {})
{
	auto creature = Content(species, offset, facing, label);
	creature.pauseMind = true;
	return creature;
}

/// How far east of the middle the item of a row stands, the first furthest west so that the testbed's camera,
/// looking north, sees the row in order from left to right
float RowX(size_t index, size_t count, float spacing = k_RowSpacing)
{
	return (static_cast<float>(index) - ((static_cast<float>(count) - 1.0f) * 0.5f)) * spacing;
}

/// Walking round the points in turn, each once it has arrived at the last
void WalkRound(std::vector<Command>& commands, size_t creature, std::span<const glm::vec2> points, bool run = false)
{
	for (const auto& point : points)
	{
		commands.push_back(Go(run ? Kind::RunTo : Kind::WalkTo, creature, point, 0.0f, true));
	}
}

std::vector<glm::vec2> Circle(glm::vec2 centre, float radius, size_t points)
{
	std::vector<glm::vec2> circle;
	for (size_t i = 0; i < points; ++i)
	{
		const auto angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(points);
		circle.emplace_back(centre + (radius * glm::vec2(std::cos(angle), std::sin(angle))));
	}
	return circle;
}

void AddIdle(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "idle.fidget",
	    .name = "Idle fidgeting",
	    .facet = Facet::Idle,
	    .description = "A content tiger in the middle of the testbed with nothing to do and nothing it needs.",
	    .expected = "It stands breathing and looks about, pulls faces now and then, sits down for a while and gets up "
	                "again, and every so often wanders off a little way to sit somewhere else.",
	    .framing = {.shot = Shot::Follow, .distance = 1.4f},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 0.0f})},
	});

	std::vector<Command> hangAround;
	const std::array<glm::vec2, 3> spots {glm::vec2 {50.0f, 40.0f}, glm::vec2 {-45.0f, 55.0f}, glm::vec2 {0.0f, -10.0f}};
	for (const auto& spot : spots)
	{
		hangAround.push_back(Go(Kind::WalkTo, 0, spot, 0.5f, true));
		hangAround.push_back(Act(Kind::SitDown, 0, 0.5f, true));
		hangAround.push_back(Act(Kind::StandUp, 0, 6.0f));
	}
	all.push_back({
	    .id = "idle.hang_around",
	    .name = "Hang around",
	    .facet = Facet::Idle,
	    .description = "A tiger walks off a little way and sits there, then on to another spot, as its idle mind does "
	                   "when it hangs around; here its mind is paused and it is told to, so it happens every time.",
	    .expected = "It walks to each of three spots near the middle, sits down there for a while, gets up and goes on "
	                "to the next.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, -10.0f}, 180.0f)},
	    .commands = hangAround,
	    .repeatFrom = 0,
	});
}

void AddExpressions(std::vector<Scenario>& all)
{
	// The desires a creature shows with an emote but doesn't act on by itself
	constexpr std::array<Desire, 10> k_Shown {
	    Desire::Impress,   Desire::Compassion, Desire::Anger,
	    Desire::Play,      Desire::Fear,       Desire::Curiosity,
	    Desire::BeFriends, Desire::Sadness,    Desire::AttractAttention,
	    Desire::GetWarmer,
	};
	std::vector<CreatureSetup> emoters;
	for (size_t i = 0; i < k_Shown.size(); ++i)
	{
		const auto row = i / 5;
		auto creature = Content(CreatureType::Tiger, {RowX(i % 5, 5, 36.0f), static_cast<float>(row) * 40.0f}, 0.0f,
		                        creature_desires::Name(k_Shown.at(i)));
		creature.desires = OnlyDesire(k_Shown.at(i));
		emoters.push_back(creature);
	}
	all.push_back({
	    .id = "expressions.desire_emotes",
	    .name = "Desire emotes",
	    .facet = Facet::Expressions,
	    .description = "Ten tigers in two rows, each with one desire as strong as it gets and every other desire gone: "
	                   "impress, compassion, anger, play, fear (front row, left to right), curiosity, be friends, "
	                   "sadness, attract attention and get warmer (back row).",
	    .expected = "Every few seconds each shows its desire with its emote: summoning, feeling nice, taunting, feeling "
	                "playful, frightened, confused, happy, sad, waving and cold. The readout names each one's desire.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = emoters,
	});

	std::vector<Command> faces;
	for (size_t i = 0; i < animations::k_IdleFaceCount; ++i)
	{
		faces.push_back(Play(Kind::PullFace, 0, animations::k_FirstFace + i, 2.5f, false));
	}
	for (size_t i = 0; i < animations::k_GestureCount; ++i)
	{
		faces.push_back(Play(Kind::PlayGesture, 0, animations::k_FirstGesture + i, 3.0f, false));
	}
	all.push_back({
	    .id = "expressions.faces_gestures",
	    .name = "Faces and gestures gallery",
	    .facet = Facet::Expressions,
	    .description = "A tiger whose mind is paused, close up, pulls each of its ten faces in turn, then plays each "
	                   "gesture on top of its body.",
	    .expected =
	        "Smile, grimace, growl, scared, sad, amazed, puzzled, laugh, ooh and aah, each held then easing out of the last, "
	        "then a nod, a shake of the head, a yawn, thirst, squirting water and talking.",
	    .framing = {.shot = Shot::Head, .distance = 1.6f},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger, .needs = Content(), .hold = true, .pauseMind = true}},
	    .commands = faces,
	    .repeatFrom = 0,
	});

	// Each shows one feeling in its face as the game's minds do: as its activity starts and again every four seconds
	struct Feeling
	{
		std::string_view label;
		std::optional<creature_face::Cue> cue;
	};
	const std::array<Feeling, 7> k_Feelings {{
	    {"stroked", std::nullopt},
	    {"slapped", std::nullopt},
	    {"frightened", creature_face::Cue::Fear},
	    {"angry", creature_face::Cue::Anger},
	    {"exhausted", creature_face::Cue::Exhausted},
	    {"amazed", creature_face::Cue::Amazed},
	    {"curious", creature_face::Cue::Curiosity},
	}};
	std::vector<CreatureSetup> feelers;
	std::vector<Command> feelings {Act(Kind::Stroke, 0, 0.5f), Act(Kind::Slap, 1, 0.1f)};
	for (size_t i = 0; i < k_Feelings.size(); ++i)
	{
		auto creature = Content(CreatureType::Tiger, {RowX(i, k_Feelings.size(), 24.0f), 0.0f}, 0.0f, k_Feelings.at(i).label);
		creature.pauseMind = k_Feelings.at(i).cue.has_value();
		feelers.push_back(creature);
		if (const auto cue = k_Feelings.at(i).cue)
		{
			feelings.push_back(Feel(i, *cue, 0.1f));
		}
	}
	feelings.push_back(Act(Kind::Stop, 0, creature_mind::k_FaceRepeatSeconds));
	all.push_back({
	    .id = "expressions.faces_emotions",
	    .name = "Faces and emotions",
	    .facet = Facet::Expressions,
	    .description = "Seven tigers in a row, each feeling something different: one stroked and one slapped over and "
	                   "over with their minds running, then (paused) frightened, angry, exhausted, amazed and curious, "
	                   "each pulling its feeling's face every four seconds as its mind would.",
	    .expected = "The stroked tiger shows it is happy with a smile, the slapped one sad with a sad face. The "
	                "frightened one is scared or goes ooh, the angry one growls or grimaces, the exhausted one grimaces, "
	                "the amazed one is amazed, the curious one puzzled, amazed or aah. Each face plays once and holds, "
	                "then eases back; no mouth opens and closes over and over. The spawner's mind panel names each "
	                "face's reason.",
	    .framing = {.shot = Shot::Overview, .distance = 0.8f},
	    .creatures = feelers,
	    .commands = feelings,
	    .repeatFrom = 0,
	});

	std::vector<Command> actions;
	for (size_t i = 0; i < animations::k_ActionCount; ++i)
	{
		actions.push_back(Play(Kind::PlayAction, 0, animations::k_FirstAction + i, 0.5f, true));
	}
	all.push_back({
	    .id = "expressions.actions",
	    .name = "Action gallery",
	    .facet = Facet::Expressions,
	    .description = "A tiger whose mind is paused plays every one of its actions in turn, from summoning to the "
	                   "friendly wave and pick me, each once the last has finished.",
	    .expected = "Each action plays through once and the body goes back to standing between them; the spawner's mind "
	                "panel names the one playing.",
	    .framing = {.shot = Shot::Follow},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger, .needs = Content(), .hold = true, .pauseMind = true}},
	    .commands = actions,
	    .repeatFrom = 0,
	});
}

void AddSenses(std::vector<Scenario>& all)
{
	const std::array<glm::vec2, 4> loop {glm::vec2 {45.0f, 45.0f}, glm::vec2 {-45.0f, 45.0f}, glm::vec2 {-45.0f, -35.0f},
	                                     glm::vec2 {45.0f, -35.0f}};
	std::vector<Command> commands {Act(Kind::SitDown, 0, 1.0f)};
	WalkRound(commands, 1, loop);
	all.push_back({
	    .id = "senses.watch_walker",
	    .name = "Look-around: watching a walker",
	    .facet = Facet::Senses,
	    .description = "A tiger sits in the middle while a cow walks round and round it.",
	    .expected = "The tiger's head and eyes turn to follow the cow as it goes round, looking away to something else "
	                "now and then and back again; once it loses interest it watches something new.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 0.0f}, 0.0f, "watcher"),
	                  Posed(CreatureType::Cow, loop.at(3), 180.0f, "walker")},
	    .commands = commands,
	    .repeatFrom = 0,
	});
}

void AddNeeds(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "needs.thirst",
	    .name = "Thirsty creature finds water",
	    .facet = Facet::Needs,
	    .description = "A parched tiger starts north of the middle, short of the testbed's lake, with its desire for "
	                   "water as strong as it gets.",
	    .expected = "It walks down the bank to the nearest edge of the lake, bends down and drinks; its thirst clears "
	                "and the desire for water is held back for a while.",
	    .framing = {.shot = Shot::Overview, .include = LakeInView()},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .offset = {0.0f, 60.0f},
	        .needs = {.energy = 1.0f, .exhaustion = 0.0f, .dehydration = 0.95f, .poo = 0.0f},
	        .desires = {{.desire = Desire::Water, .fraction = 1.0f}},
	    }},
	});

	all.push_back({
	    .id = "needs.hunger",
	    .name = "Hungry creature finds food",
	    .facet = Facet::Needs,
	    .description = "A hungry tiger with two pots of food on the land in front of it, each half a meal.",
	    .expected = "It walks up to the nearer food and eats it, then, if still hungry, the other; its energy fills up "
	                "and the hunger desire falls.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .needs = {.energy = 0.2f, .dehydration = 0.0f, .poo = 0.0f},
	        .desires = {{.desire = Desire::Hunger, .fraction = 0.9f}},
	    }},
	    .objects = {{.type = PotInfo::FoodPot, .offset = {0.0f, -50.0f}, .amount = 400},
	                {.type = PotInfo::FoodPot, .offset = {40.0f, -90.0f}, .amount = 400}},
	});

	all.push_back({
	    .id = "needs.starving",
	    .name = "Starving creature faints",
	    .facet = Facet::Needs,
	    .description = "A tiger with almost no energy left and no food anywhere, its body's time ten times as fast.",
	    .expected = "Its hunger desire grows and it shows it is hungry; once its energy runs out it faints where it "
	                "stands, lies out cold, and gets up again with a little energy back.",
	    .environment = {.bodyTimeScale = 10.0f},
	    .framing = {.shot = Shot::Follow},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .needs = {.energy = 0.02f, .exhaustion = 0.0f, .dehydration = 0.0f, .poo = 0.0f},
	        .desires = {{.desire = Desire::Hunger, .fraction = 1.0f}},
	    }},
	});

	all.push_back({
	    .id = "needs.sleep_day",
	    .name = "Tired creature sleeps by day",
	    .facet = Facet::Needs,
	    .description = "A worn out tiger at midday, its desire to sleep strong, its body's time ten times as fast.",
	    .expected = "It may yawn, then lies down and sleeps, eyes closed, until it is rested, and gets up again.",
	    .environment = {.bodyTimeScale = 10.0f},
	    .framing = {.shot = Shot::Follow},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .needs = {.energy = 1.0f, .exhaustion = 0.75f, .dehydration = 0.0f, .poo = 0.0f},
	        .desires = {{.desire = Desire::Tiredness, .fraction = 1.0f}},
	    }},
	});

	all.push_back({
	    .id = "needs.sleep_night",
	    .name = "Tired creature sleeps at night",
	    .facet = Facet::Needs,
	    .description = "Ten at night, the clock standing still: a tiger a little tired, which the night makes more so.",
	    .expected = "Under the night sky its desire to sleep grows with the dark; it lies down and sleeps, and sleeps "
	                "longer than by day.",
	    .environment = {.hour = 22.0f, .clockRuns = false},
	    .framing = {.shot = Shot::Follow},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .needs = {.energy = 1.0f, .exhaustion = 0.5f, .dehydration = 0.0f, .poo = 0.0f},
	        .desires = {{.desire = Desire::Tiredness, .fraction = 0.6f}},
	    }},
	});

	all.push_back({
	    .id = "needs.poo",
	    .name = "Needs to poo",
	    .facet = Facet::Needs,
	    .description = "A tiger full of poo with its desire to go as strong as it gets.",
	    .expected = "It may show it needs a poo first, then squats and goes; a poo drops behind it and the desire is "
	                "held back for a while.",
	    .framing = {.shot = Shot::Follow},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .needs = {.energy = 1.0f, .exhaustion = 0.0f, .dehydration = 0.0f, .poo = 0.95f},
	        .desires = {{.desire = Desire::Poo, .fraction = 1.0f}},
	    }},
	});

	all.push_back({
	    .id = "needs.puke",
	    .name = "Puke",
	    .facet = Facet::Needs,
	    .description = "A tiger is told to be sick every so often, as eating something bad would make it.",
	    .expected = "It retches and is sick, drops of it flying out in front of it and landing on the ground, where they "
	                "fade.",
	    .framing = {.shot = Shot::Follow},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 0.0f})},
	    .commands = {Act(Kind::Puke, 0, 2.0f), Act(Kind::Puke, 0, 10.0f, true)},
	    .repeatFrom = 1,
	});

	all.push_back({
	    .id = "needs.exhaustion",
	    .name = "Exhaustion and fainting from running",
	    .facet = Facet::Needs,
	    .description = "An already tired tiger is made to run back and forth across the testbed, its body's time twenty "
	                   "times as fast.",
	    .expected = "Running tires it; past 0.8 exhaustion it slows down, and at 1 it faints and lies out cold, then is "
	                "carried in a fizz to its pen, the middle of the testbed, where it rests and gets up again before "
	                "running on.",
	    .environment = {.bodyTimeScale = 20.0f},
	    .framing = {.shot = Shot::Overview},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .offset = {-150.0f, 0.0f},
	        .facingDegrees = 270.0f,
	        .needs = {.energy = 1.0f, .exhaustion = 0.6f, .dehydration = 0.0f, .poo = 0.0f},
	    }},
	    // Each run is long enough to cross; the commands don't wait for it to be free, as its idle mind sits it down
	    // whenever it stops
	    .commands = {Go(Kind::RunTo, 0, {150.0f, 0.0f}, 0.5f, false), Go(Kind::RunTo, 0, {-150.0f, 0.0f}, 16.0f, false),
	                 Go(Kind::RunTo, 0, {150.0f, 0.0f}, 16.0f, false)},
	    .repeatFrom = 1,

	});

	// The needs left to themselves: nothing is held and no desire is set, only the body's time runs faster, so the
	// needs build up as they would in play and the creature's mind sees to them when it chooses to
	all.push_back({
	    .id = "needs.left_alone",
	    .name = "Looks after itself",
	    .facet = Facet::Needs,
	    .description = "A tiger left to itself with three pots of food about it and the lake to the north, nothing held "
	                   "and no desire set, its body's time ten times as fast.",
	    .expected = "Over a few minutes its energy runs down, it gets hungry and goes to eat a pot of food, picking it up "
	                "first; the meal fills it with poo and it goes; it tires and sleeps, and it gets thirsty and walks to "
	                "the lake to drink. In between it sees to its other desires, such as calling for attention.",
	    .environment = {.bodyTimeScale = 10.0f},
	    .framing = {.shot = Shot::Follow, .distance = 1.5f},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger}},
	    .objects = {{.type = PotInfo::FoodPot, .offset = {40.0f, -40.0f}},
	                {.type = PotInfo::FoodPot, .offset = {-60.0f, -30.0f}},
	                {.type = PotInfo::FoodPot, .offset = {20.0f, -90.0f}}},
	});

	all.push_back({
	    .id = "needs.gets_hungry",
	    .name = "Gets hungry and eats",
	    .facet = Facet::Needs,
	    .description = "A tiger half full, two pots of food on the land in front of it, nothing held and no desire set, "
	                   "its body's time ten times as fast.",
	    .expected = "Its energy runs down and its hunger grows by itself until it is the strongest desire; it walks up "
	                "to the nearer pot, picks it up, looks at it and eats it, and its energy fills up.",
	    .environment = {.bodyTimeScale = 10.0f},
	    .framing = {.shot = Shot::Follow, .distance = 1.3f},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger, .needs = {.energy = 0.55f}}},
	    .objects = {{.type = PotInfo::FoodPot, .offset = {0.0f, -40.0f}},
	                {.type = PotInfo::FoodPot, .offset = {50.0f, -80.0f}}},
	});

	all.push_back({
	    .id = "needs.gets_thirsty",
	    .name = "Gets thirsty and drinks at the lake",
	    .facet = Facet::Needs,
	    .description = "A tiger south of the lake and already rather thirsty, nothing held and no desire set, its body's "
	                   "time ten times as fast.",
	    .expected = "Its thirst grows until its desire for water is the strongest; it walks to the near edge of the lake, "
	                "bends down and drinks, its thirst clears and it doesn't want water for a while.",
	    .environment = {.bodyTimeScale = 10.0f},
	    .framing = {.shot = Shot::Overview, .include = LakeInView()},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger, .offset = {0.0f, 80.0f}, .needs = {.dehydration = 0.5f}}},
	});

	all.push_back({
	    .id = "needs.gets_tired",
	    .name = "Gets tired at night and sleeps",
	    .facet = Facet::Needs,
	    .description = "Late evening with the clock running, a tiger a little worn out, nothing held and no desire set, "
	                   "its body's time ten times as fast.",
	    .expected = "As night falls its tiredness grows from the dark and its exhaustion; it may yawn, then lies down and "
	                "sleeps with its eyes closed, resting, and wakes once rested, looking sleepy.",
	    .environment = {.hour = 21.5f, .bodyTimeScale = 10.0f},
	    .framing = {.shot = Shot::Follow},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger, .needs = {.exhaustion = 0.5f}}},
	});

	all.push_back({
	    .id = "needs.needs_to_poo",
	    .name = "Needs to poo after a meal",
	    .facet = Facet::Needs,
	    .description = "A tiger that has just eaten, half full of poo, nothing held and no desire set, its body's time "
	                   "ten times as fast.",
	    .expected = "Its desire to poo grows from the poo inside it; it may show it needs to go, then squats and goes, a "
	                "lump drops behind it and the desire is held back for a while.",
	    .environment = {.bodyTimeScale = 10.0f},
	    .framing = {.shot = Shot::Follow},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger, .needs = {.poo = 0.6f}}},
	});

	all.push_back({
	    .id = "needs.cold",
	    .name = "Cold creature",
	    .facet = Facet::Needs,
	    .description = "A blizzard over the whole island, freezing cold, and a tiger out in it, its body's time ten "
	                   "times as fast. Its other needs are kept met, and its desires to be friends and for attention, "
	                   "which would show before the cold, are held at nothing.",
	    .expected = "Its warmth falls to -1 within seconds, its desire to get warmer grows to its most and it shows it "
	                "is cold with its emote; the snow starts to lie on the land round it.",
	    .environment = {.weather = Weather::Blizzard, .bodyTimeScale = 10.0f},
	    .framing = {.shot = Shot::Follow, .distance = 1.3f},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .needs = {.energy = 1.0f, .exhaustion = 0.0f, .dehydration = 0.0f, .poo = 0.0f, .life = 1.0f},
	        .desires = {{.desire = Desire::BeFriends, .fraction = 0.0f},
	                    {.desire = Desire::AttractAttention, .fraction = 0.0f}},
	        .hold = true,
	    }},
	});
}

void AddGrowth(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "growth.timelapse",
	    .name = "Growth time-lapse",
	    .facet = Facet::Growth,
	    .description = "A baby tiger, small and early in growing up, kept fed and still, its body's time a thousand "
	                   "times as fast.",
	    .expected = "It grows fast while young and more slowly as it ages, up to size 2, the most a creature grows by "
	                "itself; the readout shows its size.",
	    .environment = {.bodyTimeScale = 1000.0f},
	    .framing = {.shot = Shot::Overview, .include = {{-50.0f, -40.0f}, {50.0f, 60.0f}}},
	    .creatures = {CreatureSetup {
	        .label = "baby",
	        .species = CreatureType::Tiger,
	        .size = 0.3f,
	        .phase = 3,
	        .needs = Content(),
	        .hold = true,
	        .pauseMind = true,
	    }},
	});

	struct Shape
	{
		std::string_view label;
		float alignment;
		float fatness;
		float strength;
	};
	constexpr std::array<Shape, 7> k_Shapes {{
	    {"thin", 0.0f, 0.0f, 0.5f},
	    {"fat", 0.0f, 1.0f, 0.5f},
	    {"weak", 0.0f, 0.5f, 0.0f},
	    {"strong", 0.0f, 0.5f, 1.0f},
	    {"evil", -1.0f, 0.5f, 0.5f},
	    {"good", 1.0f, 0.5f, 0.5f},
	    {"neutral", 0.0f, 0.5f, 0.5f},
	}};
	std::vector<CreatureSetup> lineup;
	for (size_t i = 0; i < k_Shapes.size(); ++i)
	{
		const auto& shape = k_Shapes.at(i);
		auto creature = Content(CreatureType::Tiger, {RowX(i, k_Shapes.size()), 0.0f}, 0.0f, shape.label);
		creature.alignment = shape.alignment;
		creature.fatness = shape.fatness;
		creature.strength = shape.strength;
		creature.size = 1.0f;
		creature.pauseMind = true;
		lineup.push_back(creature);
	}
	all.push_back({
	    .id = "growth.morph_lineup",
	    .name = "Morph lineup",
	    .facet = Facet::Growth,
	    .description = "Seven tigers of the same size side by side, left to right: thin, fat, weak, strong, evil, good "
	                   "and neutral.",
	    .expected = "Each body is pulled towards its axis' mesh: a lean and a heavy belly, slight and bulging muscles, "
	                "the evil one hunched and dark and the good one upright and bright, the last as the base mesh.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = lineup,
	});

	std::vector<CreatureSetup> parade;
	constexpr size_t k_Species = static_cast<size_t>(CreatureType::_COUNT) - 1;
	constexpr size_t k_FrontRow = 9;
	std::vector<Command> parading;
	for (size_t i = 0; i < k_Species; ++i)
	{
		const auto row = i < k_FrontRow ? 0 : 1;
		const auto inRow = row == 0 ? i : i - k_FrontRow;
		const auto count = row == 0 ? k_FrontRow : k_Species - k_FrontRow;
		const glm::vec2 offset {RowX(inRow, count, 40.0f), 60.0f + (static_cast<float>(row) * 50.0f)};
		auto creature = Posed(static_cast<CreatureType>(i + 1), offset);
		parade.push_back(creature);
		parading.push_back(Go(Kind::WalkTo, i, offset - glm::vec2(0.0f, 60.0f), i == 0 ? 3.0f : 0.0f, false));
	}
	for (size_t i = 0; i < k_Species; ++i)
	{
		parading.push_back(Act(Kind::FaceCamera, i, i == 0 ? 8.0f : 0.0f));
	}
	all.push_back({
	    .id = "growth.species_parade",
	    .name = "All species parade",
	    .facet = Facet::Growth,
	    .description = "One creature of every species, all at size 1, in two rows: cow, tiger, leopard, wolf, lion, "
	                   "horse, tortoise, zebra and brown bear in front; polar bear, sheep, chimp, ogre, mandrill, rhino, "
	                   "gorilla and giant ape behind.",
	    .expected = "After a moment they all walk forward together, then turn to face the camera, each "
	                "with its own walk.",
	    .framing = {.shot = Shot::Overview, .include = {{0.0f, -10.0f}}},
	    .creatures = parade,
	    .commands = parading,
	});
}

void AddAppearance(std::vector<Scenario>& all)
{
	constexpr std::array<CreatureType, 3> k_Species {CreatureType::GiantApe, CreatureType::Mandrill, CreatureType::Horse};
	constexpr std::array<float, 3> k_Alignments {-1.0f, 0.0f, 1.0f};
	constexpr std::array<std::string_view, 9> k_Labels {"evil ape",      "neutral ape",      "good ape",
	                                                    "evil mandrill", "neutral mandrill", "good mandrill",
	                                                    "evil horse",    "neutral horse",    "good horse"};
	std::vector<CreatureSetup> skins;
	for (size_t row = 0; row < k_Species.size(); ++row)
	{
		for (size_t column = 0; column < k_Alignments.size(); ++column)
		{
			auto creature = Content(k_Species.at(row), {RowX(column, 3, 45.0f), static_cast<float>(row) * 45.0f}, 0.0f,
			                        k_Labels.at((row * 3) + column));
			creature.alignment = k_Alignments.at(column);
			creature.pauseMind = true;
			skins.push_back(creature);
		}
	}
	all.push_back({
	    .id = "appearance.alignment_skins",
	    .name = "Alignment skins and hair",
	    .facet = Facet::Appearance,
	    .description = "Giant apes, mandrills and horses, a row of each from front to back, evil, neutral and good from "
	                   "left to right.",
	    .expected = "Each skin blends towards its evil or good skin with the alignment, the bodies follow it, and the "
	                "hair of the hairy species is drawn over them (the spawner's Show hair turns it off).",
	    .framing = {.shot = Shot::Overview},
	    .creatures = skins,
	});

	auto tattooed = Posed(CreatureType::Tiger, {0.0f, 0.0f}, 30.0f, "tattooed");
	constexpr std::array<glm::u8vec3, 8> k_Colours {{
	    {220, 40, 40},
	    {40, 200, 60},
	    {50, 90, 230},
	    {240, 210, 40},
	    {200, 60, 220},
	    {40, 210, 220},
	    {250, 140, 30},
	    {245, 245, 245},
	}};
	for (size_t site = 0; site < creature_tattoo::k_SlotCount; ++site)
	{
		tattooed.tattoos.push_back({.design = static_cast<uint8_t>(((site * 5) + 1) % creature_tattoo::k_DesignCount),
		                            .site = static_cast<uint8_t>(site),
		                            .colour = k_Colours.at(site)});
	}
	tattooed.wounds = {
	    {.u = 60, .v = 90, .skin = 0, .type = 3, .column = 1},  {.u = 150, .v = 120, .skin = 0, .type = 4, .column = 3},
	    {.u = 200, .v = 70, .skin = 1, .type = 5, .column = 5}, {.u = 90, .v = 180, .skin = 1, .type = 1, .column = 2},
	    {.u = 120, .v = 40, .skin = 2, .type = 3, .column = 6},
	};
	for (uint8_t v = 100; v < 112; ++v)
	{
		tattooed.blood.push_back({.u = 128, .v = v, .skin = 0});
	}
	auto plain = Posed(CreatureType::Tiger, {25.0f, 0.0f}, 330.0f, "unmarked");
	all.push_back({
	    .id = "appearance.tattoos_wounds",
	    .name = "Tattoos and wounds showcase",
	    .facet = Facet::Appearance,
	    .description = "A tiger with a tattoo in every slot, each of a different design and colour, five wounds and "
	                   "burns and a trail of blood, beside an unmarked tiger to compare.",
	    .expected = "The designs sit on the species' tattoo sites in their colours; the wounds start fresh and red and "
	                "heal over time to old scars, the blood running down the skin and fading.",
	    .framing = {.shot = Shot::Follow, .distance = 0.9f},
	    .creatures = {tattooed, plain},
	});

	std::vector<Command> passing {Act(Kind::SitDown, 0, 0.5f)};
	const std::array<glm::vec2, 2> across {glm::vec2 {-45.0f, -30.0f}, glm::vec2 {45.0f, -30.0f}};
	WalkRound(passing, 1, across);
	// Sitting down again once it has got up by itself, so it stays to be looked at
	passing.push_back(Act(Kind::SitDown, 0, 0.0f));
	all.push_back({
	    .id = "appearance.eyes",
	    .name = "Eyes and blinking close-up",
	    .facet = Facet::Appearance,
	    .description = "A tiger, close up, sits facing the camera while a cow walks back and forth in front of it, "
	                   "just behind the camera.",
	    .expected = "It blinks every few seconds; its eyes and head turn to follow the cow from side to side, the eyes "
	                "leading the head.",
	    .framing = {.shot = Shot::Head, .distance = 1.5f},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 0.0f}, 0.0f, "watcher"),
	                  Posed(CreatureType::Cow, across.at(1), 90.0f, "walker")},
	    .commands = passing,
	    .repeatFrom = 1,
	});
}

void AddLight(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "light.shadows",
	    .name = "Shadows through the day",
	    .facet = Facet::Light,
	    .description = "A tiger, a cow and a giant ape stand still while the hour jumps from morning to midday, the "
	                   "afternoon and dusk, the clock otherwise standing still.",
	    .expected = "Their shadows lie long to one side in the morning, short under them at midday and long to the "
	                "other side in the afternoon, fading as the sun goes down.",
	    .environment = {.hour = 9.0f, .clockRuns = false},
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Posed(CreatureType::Tiger, {40.0f, 0.0f}), Posed(CreatureType::Cow, {0.0f, 0.0f}),
	                  Posed(CreatureType::GiantApe, {-45.0f, 0.0f})},
	    .commands = {AtHour(9.0f, 0.0f), AtHour(12.0f, 5.0f), AtHour(15.0f, 5.0f), AtHour(17.5f, 5.0f), AtHour(20.0f, 5.0f),
	                 AtHour(9.0f, 5.0f)},
	    .repeatFrom = 1,
	});

	all.push_back({
	    .id = "light.reflections",
	    .name = "Reflections in the lake",
	    .facet = Facet::Light,
	    .description = "A tiger, a horse and a giant ape stand in the far shallows of the testbed's lake, seen across "
	                   "the water from the south.",
	    .expected = "Each shows upside down in the water in front of it, moving as it moves.",
	    .framing = {.shot = Shot::Overview, .include = LakeInView()},
	    .creatures = {Posed(CreatureType::Tiger, {k_Lake.x + 30.0f, k_FarShallows}),
	                  Posed(CreatureType::Horse, {k_Lake.x, k_FarShallows}),
	                  Posed(CreatureType::GiantApe, {k_Lake.x - 30.0f, k_FarShallows})},
	});
}

void AddMovement(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "movement.course",
	    .name = "Walk, run, turn and step course",
	    .facet = Facet::Movement,
	    .description = "A tiger goes round a square course: walking one side, running the next, turning on the spot to "
	                   "face the middle, walking, running, then a short step off.",
	    .expected = "Its legs blend between standing, walking and running with its speed, it turns on the spot without "
	                "sliding its feet, and the short last leg is a step rather than a walk; the route shows over the "
	                "land in the spawner.",
	    .framing = {.shot = Shot::Overview, .include = {{70.0f, 70.0f}, {-70.0f, -70.0f}}},
	    .creatures = {Posed(CreatureType::Tiger, {60.0f, -60.0f}, 180.0f)},
	    .commands = {Go(Kind::WalkTo, 0, {60.0f, 60.0f}, 1.0f), Go(Kind::RunTo, 0, {-60.0f, 60.0f}, 0.5f),
	                 Go(Kind::TurnToFace, 0, {0.0f, 0.0f}, 0.5f), Go(Kind::WalkTo, 0, {-60.0f, -60.0f}, 1.0f),
	                 Go(Kind::RunTo, 0, {50.0f, -60.0f}, 0.5f), Go(Kind::WalkTo, 0, {60.0f, -60.0f}, 0.5f)},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "movement.lake",
	    .name = "Round the lake and through its shallows",
	    .facet = Facet::Movement,
	    .description = "A tiger on the west bank of the testbed's lake walks to the east bank, then down into the near "
	                   "shallows and back up onto the plain.",
	    .expected = "It never sets foot on the open water, which is too deep: its route bends round the lake on the bank "
	                "or wades its shallows; it walks into the shallows and stands with its feet at the water, then "
	                "climbs the bank.",
	    .framing = {.shot = Shot::Overview, .include = LakeInView()},
	    .creatures = {Posed(CreatureType::Tiger, {k_Lake.x - k_LakeHalf.x - 50.0f, k_Lake.y}, 90.0f)},
	    .commands = {Go(Kind::WalkTo, 0, {k_Lake.x + k_LakeHalf.x + 50.0f, k_Lake.y}, 1.0f),
	                 Go(Kind::WalkTo, 0, {k_Lake.x + 20.0f, k_NearShallows}, 1.0f),
	                 Go(Kind::WalkTo, 0, {k_Lake.x - k_LakeHalf.x - 50.0f, k_Lake.y}, 1.0f)},
	    .repeatFrom = 0,
	});

	std::vector<ObjectSetup> wall;
	for (int i = -3; i <= 3; ++i)
	{
		wall.push_back({.type = TreeInfo::Conifer, .offset = {static_cast<float>(i) * 12.0f, 0.0f}, .scale = 1.5f});
	}
	wall.push_back({.type = FeatureInfo::FatPilarChalk, .offset = {-60.0f, 0.0f}});
	all.push_back({
	    .id = "movement.route_obstacles",
	    .name = "Route around obstacles",
	    .facet = Facet::Movement,
	    .description = "A small tiger walks back and forth past a row of tall conifers and a pillar of rock, with a cow "
	                   "standing still beyond them in its way.",
	    .expected = "Its route bends round the end of the row of trees, which are taller than it, and round the standing "
	                "cow rather than through them; the route shows over the land in the spawner.",
	    .framing = {.shot = Shot::Overview, .include = {{0.0f, 120.0f}, {0.0f, -100.0f}}},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger,
	                                 .offset = {0.0f, -100.0f},
	                                 .facingDegrees = 180.0f,
	                                 .size = 0.6f,
	                                 .needs = Content(),
	                                 .hold = true,
	                                 .pauseMind = true},
	                  CreatureSetup {.label = "in the way",
	                                 .species = CreatureType::Cow,
	                                 .offset = {0.0f, 60.0f},
	                                 .needs = Content(),
	                                 .hold = true,
	                                 .pauseMind = true}},
	    .objects = wall,
	    .commands = {Go(Kind::WalkTo, 0, {0.0f, 120.0f}, 1.5f), Go(Kind::WalkTo, 0, {0.0f, -100.0f}, 1.5f)},
	    .repeatFrom = 0,
	});

	std::vector<ObjectSetup> copse;
	for (int x = -2; x <= 2; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const auto type = (x + y) % 2 == 0 ? TreeInfo::Bush : TreeInfo::Birch;
			copse.push_back({.type = type,
			                 .offset = {static_cast<float>(x) * 9.0f, static_cast<float>(y) * 9.0f},
			                 .yawDegrees = static_cast<float>((x * 40) + (y * 70))});
		}
	}
	all.push_back({
	    .id = "movement.trampling",
	    .name = "Walking through small trees",
	    .facet = Facet::Movement,
	    .description = "A full grown giant ape walks back and forth through a copse of bushes and birches much shorter "
	                   "than it.",
	    .expected = "It walks straight through the copse rather than round it, as big creatures walk through trees under "
	                "one and a half times their height; the trees stay standing.",
	    .framing = {.shot = Shot::Overview, .include = {{0.0f, 90.0f}, {0.0f, -90.0f}}},
	    .creatures = {CreatureSetup {.species = CreatureType::GiantApe,
	                                 .offset = {0.0f, -90.0f},
	                                 .facingDegrees = 180.0f,
	                                 .size = 2.0f,
	                                 .needs = Content(),
	                                 .hold = true,
	                                 .pauseMind = true}},
	    .objects = copse,
	    .commands = {Go(Kind::WalkTo, 0, {0.0f, 90.0f}, 1.0f), Go(Kind::WalkTo, 0, {0.0f, -90.0f}, 1.0f)},
	    .repeatFrom = 0,
	});

	const auto loop = Circle({0.0f, 0.0f}, 70.0f, 6);
	std::vector<Command> chase {{.kind = Kind::Follow, .creature = 1, .delaySeconds = 1.0f, .value = 0}};
	WalkRound(chase, 0, loop);
	chase.push_back(Go(Kind::FleeFrom, 2, {0.0f, 0.0f}, 0.0f, false));
	chase.push_back(Go(Kind::WalkTo, 2, {0.0f, 20.0f}, 3.0f, true));
	all.push_back({
	    .id = "movement.follow_flee",
	    .name = "Follow and flee",
	    .facet = Facet::Movement,
	    .description = "A cow walks round a circle with a tiger following it; a wolf in the middle keeps running away "
	                   "and coming back.",
	    .expected = "The tiger keeps within a short distance of the cow, catching up when it falls behind; the wolf runs "
	                "off from the middle, then walks back.",
	    .framing = {.shot = Shot::Overview, .include = {{80.0f, 80.0f}, {-80.0f, -80.0f}}},
	    .creatures = {Posed(CreatureType::Cow, loop.back(), 0.0f, "leader"),
	                  Posed(CreatureType::Tiger, loop.back() + glm::vec2(0.0f, -30.0f), 0.0f, "follower"),
	                  Posed(CreatureType::Wolf, {0.0f, 20.0f}, 0.0f, "flees")},
	    .commands = chase,
	    .repeatFrom = 1,
	});
}

void AddFootprints(std::vector<Scenario>& all)
{
	constexpr std::array<CreatureType, 3> k_Walkers {CreatureType::Tiger, CreatureType::Cow, CreatureType::GiantApe};
	std::vector<CreatureSetup> walkers;
	std::vector<Command> trails;
	for (size_t i = 0; i < k_Walkers.size(); ++i)
	{
		const auto y = 30.0f - (static_cast<float>(i) * 30.0f);
		walkers.push_back(Posed(k_Walkers.at(i), {110.0f, y}, 90.0f));
		trails.push_back(Go(Kind::WalkTo, i, {-110.0f, y}, i == 0 ? 1.0f : 0.0f, false));
	}
	for (size_t i = 0; i < k_Walkers.size(); ++i)
	{
		const auto y = 30.0f - (static_cast<float>(i) * 30.0f);
		trails.push_back(Go(Kind::WalkTo, i, {110.0f, y}, 0.0f, true));
	}
	all.push_back({
	    .id = "footprints.trail",
	    .name = "Footprints trail per species",
	    .facet = Facet::Footprints,
	    .description = "A tiger, a cow and a giant ape walk side by side across the testbed and back.",
	    .expected = "Each leaves its own prints where its feet fall: paws for the tiger, hooves for the cow and broad "
	                "feet for the ape, fading after a while.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = walkers,
	    .commands = trails,
	    .repeatFrom = 0,
	});

	std::vector<Command> circling;
	WalkRound(circling, 0, Circle({0.0f, 0.0f}, 50.0f, 8));
	all.push_back({
	    .id = "footprints.april_fools",
	    .name = "1 April smiley footprints",
	    .facet = Facet::Footprints,
	    .description = "As on the first of April, whatever the date: a tiger walks round in a circle.",
	    .expected = "Its footprints are smiley faces.",
	    .environment = {.aprilFools = true},
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Posed(CreatureType::Tiger, {50.0f, 0.0f}, 180.0f)},
	    .commands = circling,
	    .repeatFrom = 0,
	});
}

void AddAudio(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "audio.surfaces",
	    .name = "Footsteps on grass and in the shallows",
	    .facet = Facet::Audio,
	    .description = "A tiger walks from the grass down the bank into the shallows of the testbed's lake and back. "
	                   "Select it in the spawner to see its sound log.",
	    .expected = "Its footsteps sound of grass on the land and of splashing in the water; the log's keys show the "
	                "surface changing.",
	    .framing = {.shot = Shot::Follow, .distance = 1.4f},
	    .creatures = {Posed(CreatureType::Tiger, {k_Lake.x, 80.0f}, 180.0f)},
	    .commands = {Go(Kind::WalkTo, 0, {k_Lake.x, k_NearShallows}, 1.0f), Go(Kind::WalkTo, 0, {k_Lake.x, 80.0f}, 1.0f)},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "audio.snow",
	    .name = "Footsteps in snow",
	    .facet = Facet::Audio,
	    .description = "Snow falls over the whole island while a tiger walks back and forth. Select it in the spawner to "
	                   "see its sound log.",
	    .expected = "Once the snow lies deep enough, its footsteps change from grass to snow.",
	    .environment = {.weather = Weather::Snow},
	    .framing = {.shot = Shot::Follow, .distance = 1.4f},
	    .creatures = {Posed(CreatureType::Tiger, {-50.0f, 0.0f}, 270.0f)},
	    .commands = {Go(Kind::WalkTo, 0, {50.0f, 0.0f}, 1.0f), Go(Kind::WalkTo, 0, {-50.0f, 0.0f}, 1.0f)},
	    .repeatFrom = 0,
	});

	constexpr std::array<size_t, 5> k_Voiced {animations::k_Angry, animations::k_Happy, animations::k_Hungry,
	                                          animations::k_Taunt, animations::k_Frightened};
	std::vector<Command> voices;
	for (const auto action : k_Voiced)
	{
		for (size_t creature = 0; creature < 3; ++creature)
		{
			voices.push_back(Play(Kind::PlayAction, creature, action, 1.0f, true));
		}
	}
	auto small = Content(CreatureType::Tiger, {45.0f, 0.0f}, 0.0f, "small tiger");
	small.size = 0.5f;
	small.pauseMind = true;
	auto big = Content(CreatureType::Tiger, {0.0f, 0.0f}, 0.0f, "big tiger");
	big.size = 2.0f;
	big.pauseMind = true;
	auto cow = Content(CreatureType::Cow, {-50.0f, 0.0f}, 0.0f, "cow");
	cow.pauseMind = true;
	all.push_back({
	    .id = "audio.voices",
	    .name = "Voices by species and size",
	    .facet = Facet::Audio,
	    .description = "A small tiger, a big tiger and a cow take turns to be angry, happy, hungry, taunting and "
	                   "frightened. Select one in the spawner to see its sound log.",
	    .expected = "Each plays its voice for the action from its own bank, the small tiger higher than the big one and "
	                "the cow its own; the log's size key differs between the tigers.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {small, big, cow},
	    .commands = voices,
	    .repeatFrom = 0,
	});
}

void AddObjects(std::vector<Scenario>& all)
{
	// Balls all round a tiger facing the camera, south: it reaches for each in turn, looks it over and puts it down,
	// then goes back to the middle and faces south again
	constexpr float k_Reach = 10.0f;
	constexpr float k_Diagonal = 7.0f;
	const std::array<glm::vec2, 6> k_Round {glm::vec2 {0.0f, -k_Reach},          glm::vec2 {-k_Diagonal, -k_Diagonal},
	                                        glm::vec2 {k_Diagonal, -k_Diagonal}, glm::vec2 {-k_Reach, 0.0f},
	                                        glm::vec2 {k_Reach, 0.0f},           glm::vec2 {0.0f, k_Reach}};
	std::vector<ObjectSetup> balls;
	std::vector<Command> reaching;
	for (size_t i = 0; i < k_Round.size(); ++i)
	{
		balls.push_back({.type = MobileObjectInfo::Ball, .offset = k_Round.at(i)});
		reaching.push_back(OnObject(Kind::PickUp, 0, i, 1.0f));
		reaching.push_back(Play(Kind::Examine, 0, creature_object_actions::k_KeepAnimationCount - 1, 0.5f, true));
		reaching.push_back(Act(Kind::PutDown, 0, 0.5f, true));
		reaching.push_back(Go(Kind::WalkTo, 0, {0.0f, 0.0f}, 0.5f));
		reaching.push_back(Go(Kind::TurnToFace, 0, {0.0f, -60.0f}, 0.0f));
	}
	all.push_back({
	    .id = "objects.reach",
	    .name = "Picking up, looking over, putting down",
	    .facet = Facet::Objects,
	    .description = "A tiger facing the camera with balls in front of it, to its front left and right, either side "
	                   "and behind it. It picks each up in turn, examines it and puts it down, then goes back to the "
	                   "middle.",
	    .expected = "It reaches for each ball the way it lies, in front, to the side or behind, picking it up with its "
	                "hand; holds it up to look at it; and puts it down gently.",
	    .framing = {.shot = Shot::Follow, .distance = 1.3f},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, 0.0f})},
	    .objects = balls,
	    .commands = reaching,
	    .repeatFrom = 0,
	});

	std::vector<Command> examining {OnObject(Kind::PickUp, 0, 0, 1.0f)};
	for (size_t way = 0; way < creature_object_actions::k_KeepAnimationCount; ++way)
	{
		examining.push_back(Play(Kind::Examine, 0, way, 1.0f, true));
	}
	examining.push_back(Act(Kind::PutDown, 0, 1.0f, true));
	all.push_back({
	    .id = "objects.examine",
	    .name = "Looking a thing over four ways",
	    .facet = Facet::Objects,
	    .description = "A tiger picks up a pot in front of it and strokes, shakes, smells and examines it in turn, then "
	                   "puts it down.",
	    .expected = "Each way of looking the pot over plays its own animation with the pot held in the hand.",
	    .framing = {.shot = Shot::Follow, .distance = 1.2f},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, 0.0f})},
	    .objects = {{.type = MobileObjectInfo::EgyptPotA, .offset = {0.0f, -k_Reach}}},
	    .commands = examining,
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "objects.throw_lob",
	    .name = "Throwing at a target, lobbing and tossing away",
	    .facet = Facet::Objects,
	    .description = "A tiger picks up a ball and throws it at a pillar of rock 60 units east, fetches it and lobs it, "
	                   "fetches it again and tosses it away.",
	    .expected = "The throw flies flat and hard towards the pillar; the lob goes up high and comes down near; the "
	                "toss drops it aside. Each comes to rest where it lands.",
	    .framing = {.shot = Shot::Overview, .include = {{60.0f, 30.0f}, {-30.0f, -30.0f}}},
	    .creatures = {Posed(CreatureType::Tiger, {-20.0f, 0.0f}, 270.0f)},
	    .objects = {{.type = MobileObjectInfo::Ball, .offset = {-20.0f, -k_Reach}},
	                {.type = FeatureInfo::FatPilarChalk, .offset = {60.0f, 0.0f}}},
	    .commands = {OnObject(Kind::PickUp, 0, 0, 1.0f), Go(Kind::ThrowAt, 0, {60.0f, 0.0f}, 0.5f),
	                 OnObject(Kind::PickUp, 0, 0, 1.0f), Act(Kind::Lob, 0, 0.5f, true), OnObject(Kind::PickUp, 0, 0, 1.0f),
	                 Act(Kind::Discard, 0, 0.5f, true), Go(Kind::WalkTo, 0, {-20.0f, 0.0f}, 1.0f)},
	    .repeatFrom = 0,
	});

	auto hungry = Posed(CreatureType::Tiger, {0.0f, 0.0f});
	hungry.needs = {.energy = 0.3f};
	hungry.hold = false;
	all.push_back({
	    .id = "objects.eat_held",
	    .name = "Eating by picking food up",
	    .facet = Facet::Objects,
	    .description = "A hungry tiger with three piles of food about it picks each up and eats it from its hand.",
	    .expected = "It walks up to each, picks it up, brings it to its mouth and eats it; the food is gone and its "
	                "energy rises in the spawner.",
	    .framing = {.shot = Shot::Follow, .distance = 1.3f},
	    .creatures = {hungry},
	    .objects = {{.type = MobileObjectInfo::MagicFood, .offset = {0.0f, -k_Reach}},
	                {.type = MobileObjectInfo::MagicFood, .offset = {-15.0f, -5.0f}},
	                {.type = MobileObjectInfo::MagicFood, .offset = {15.0f, -5.0f}}},
	    .commands = {OnObject(Kind::PickUp, 0, 0, 1.0f), Act(Kind::EatHeld, 0, 0.5f, true), OnObject(Kind::PickUp, 0, 1, 1.0f),
	                 Act(Kind::EatHeld, 0, 0.5f, true), OnObject(Kind::PickUp, 0, 2, 1.0f), Act(Kind::EatHeld, 0, 0.5f, true)},
	});

	all.push_back({
	    .id = "objects.knock_down_trees",
	    .name = "Knocking down trees",
	    .facet = Facet::Objects,
	    .description = "A tiger walks up to each of three trees in a row and knocks it down.",
	    .expected = "It strikes each tree, which falls; its town's view of it shows in the spawner.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, -20.0f})},
	    .objects = {{.type = TreeInfo::Oak, .offset = {-40.0f, 10.0f}},
	                {.type = TreeInfo::Beech, .offset = {0.0f, 10.0f}},
	                {.type = TreeInfo::Conifer, .offset = {40.0f, 10.0f}}},
	    .commands = {OnObject(Kind::KnockDown, 0, 0, 1.0f), OnObject(Kind::KnockDown, 0, 1, 1.0f),
	                 OnObject(Kind::KnockDown, 0, 2, 1.0f)},
	});

	all.push_back({
	    .id = "objects.point",
	    .name = "Pointing",
	    .facet = Facet::Objects,
	    .description = "A tiger facing the camera points in front of it, to either side, far off and behind it.",
	    .expected = "It turns as needed and points its arm at each point in turn.",
	    .framing = {.shot = Shot::Follow, .distance = 1.4f},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, 0.0f})},
	    .commands = {Go(Kind::PointAt, 0, {0.0f, -40.0f}, 1.0f), Go(Kind::PointAt, 0, {-40.0f, 0.0f}, 1.0f),
	                 Go(Kind::PointAt, 0, {40.0f, 0.0f}, 1.0f), Go(Kind::PointAt, 0, {0.0f, 200.0f}, 1.0f),
	                 Go(Kind::TurnToFace, 0, {0.0f, -60.0f}, 1.0f)},
	    .repeatFrom = 0,
	});

	// Left to the mind: one desire as strong as it gets, with things about to act on
	struct Mood
	{
		std::string_view id;
		std::string_view name;
		Desire desire;
		std::string_view description;
		std::string_view expected;
	};
	const std::array<Mood, 3> k_Moods {{
	    {"objects.curious", "Curious about things", Desire::Curiosity,
	     "A tiger as curious as it gets, wanting nothing else, among a ball, a pot and a barrel.",
	     "It walks up to something nearby, picks it up and looks it over, then mostly puts it down gently and sometimes "
	     "tosses it away, and goes on to the next."},
	    {"objects.playful", "Playing with things", Desire::Play,
	     "A tiger as playful as it gets, wanting nothing else, among a ball, a pot and a barrel.",
	     "It picks something up and throws it about, a good way off in a random direction, and goes after the next."},
	    {"objects.angry", "Angry with things", Desire::Anger,
	     "A tiger as angry as it gets, wanting nothing else, among a ball, a pot and a barrel, with trees nearby.",
	     "It picks something up, now and then shows its anger first, and hurls it at the nearest tree."},
	}};
	for (const auto& mood : k_Moods)
	{
		auto moody = Content(CreatureType::Tiger, {0.0f, 0.0f});
		moody.desires = OnlyDesire(mood.desire);
		all.push_back({
		    .id = mood.id,
		    .name = mood.name,
		    .facet = Facet::Objects,
		    .description = mood.description,
		    .expected = mood.expected,
		    .framing = {.shot = Shot::Overview, .distance = 1.2f},
		    .creatures = {moody},
		    .objects = {{.type = MobileObjectInfo::Ball, .offset = {-15.0f, -10.0f}},
		                {.type = MobileObjectInfo::EgyptPotA, .offset = {15.0f, -10.0f}},
		                {.type = MobileObjectInfo::EgyptBarrel, .offset = {0.0f, 15.0f}},
		                {.type = TreeInfo::Oak, .offset = {-50.0f, 30.0f}},
		                {.type = TreeInfo::Birch, .offset = {50.0f, 30.0f}}},
		});
	}
}

void AddHand(std::vector<Scenario>& all)
{
	using creature_feedback::BodyPart;
	// Every part of the body, the head first
	std::vector<Command> strokes;
	for (size_t part = 0; part < creature_feedback::k_BodyPartCount; ++part)
	{
		strokes.push_back(Stroke(0, static_cast<BodyPart>(part), 1.0f));
	}
	strokes.push_back(Act(Kind::HandLetGo, 0, 1.0f, true));
	all.push_back({
	    .id = "hand.stroke",
	    .name = "Stroking each part of the body",
	    .facet = Facet::Hand,
	    .description = "The hand rests on a tiger facing the camera and strokes its head, armpits, belly, groin, feet and "
	                   "hands in turn, then lets go.",
	    .expected = "Each stroke plays the pleased animation for its part, mirrored for the left side, and its face; the "
	                "panel's reward climbs a tenth a stroke to Good Boy! 90%, and letting go warms it to the player.",
	    .framing = {.shot = Shot::Follow, .distance = 1.2f},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, 0.0f})},
	    .commands = strokes,
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "hand.slap",
	    .name = "Slapping gently and hard, high and low",
	    .facet = Facet::Hand,
	    .description = "The hand slaps a tiger facing the camera on the head, the waist and the feet, gently and then "
	                   "hard, from either side, then lets go.",
	    .expected = "Each slap plays its reeling animation for its height, the gentle ones softer, the sweeps to the "
	                "right mirrored; the panel's reward falls to Bad Boy! 100%, and letting go cools it to the player.",
	    .framing = {.shot = Shot::Follow, .distance = 1.2f},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, 0.0f})},
	    .commands = {Slap(0, 0.85f, true, false, 1.0f), Slap(0, 0.55f, true, true, 1.0f), Slap(0, 0.2f, true, false, 1.0f),
	                 Slap(0, 0.85f, false, true, 1.0f), Slap(0, 0.55f, false, false, 1.0f), Slap(0, 0.2f, false, true, 1.0f),
	                 Act(Kind::HandLetGo, 0, 1.0f, true)},
	    .repeatFrom = 0,
	});

	auto worn = Posed(CreatureType::Tiger, {0.0f, 0.0f});
	worn.needs = {.energy = 0.595f, .exhaustion = 0.325f, .life = 0.745f};
	all.push_back({
	    .id = "hand.status_panel",
	    .name = "The creature's status panel",
	    .facet = Facet::Hand,
	    .description = "The hand on a tiger whose needs are held at 25% damage, 40% hunger and 32% tiredness strokes it "
	                   "three times, slaps it once gently and once hard, and lets go.",
	    .expected = "While the hand is on it the panel at the left shows Damage 25%, Hunger 40% and Tiredness 32% in "
	                "yellow bars, and the reward going from No Reward 0% to Good Boy! 30% in green, then down past the "
	                "middle to Bad Boy! in red. Hovering the hand over it shows the panel with the last reward.",
	    .framing = {.shot = Shot::Follow, .distance = 1.0f},
	    .creatures = {worn},
	    .commands = {Stroke(0, BodyPart::Head, 1.0f), Stroke(0, BodyPart::Belly, 1.0f), Stroke(0, BodyPart::RightHand, 1.0f),
	                 Slap(0, 0.85f, true, false, 2.0f), Slap(0, 0.55f, false, true, 2.0f), Act(Kind::HandLetGo, 0, 3.0f, true)},
	    .repeatFrom = 0,
	});
}

void AddLeash(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "leash.types",
	    .name = "Led on each leash",
	    .facet = Facet::Leash,
	    .description = "A tiger is put on the learning leash, then the aggression leash, then the compassion leash, a "
	                   "while each, and the leash is taken off. Move the hand about to lead it.",
	    .expected = "A rope runs from the hand to its collar, coloured for each leash; on the aggression leash it wants "
	                "to be angry, on the compassion leash to be kind, and on the learning leash it watches the player.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 40.0f})},
	    .commands = {Leash(0, LeashType::Rope, 1.0f), Leash(0, LeashType::Evil, 10.0f), Leash(0, LeashType::Good, 10.0f),
	                 Act(Kind::TakeOffLeash, 0, 10.0f), Act(Kind::Stop, 0, 3.0f)},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "leash.pull_to_hand",
	    .name = "Pulled to the hand",
	    .facet = Facet::Leash,
	    .description = "A tiger far off to the north is put on the learning leash. Put the hand on the land near the "
	                   "camera, well away from it.",
	    .expected = "Once the rope is pulled taut it stops what it is doing and walks to the hand, then its mind takes "
	                "over again near the hand.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 150.0f})},
	    .commands = {Leash(0, LeashType::Rope, 1.0f)},
	});

	all.push_back({
	    .id = "leash.tied",
	    .name = "Tied to a tree and a rock",
	    .facet = Facet::Leash,
	    .description = "A tiger on the learning leash is tied to a tree and told to walk far off, untied back to the "
	                   "hand, then tied to a pillar of rock and told to walk far off again.",
	    .expected = "The rope runs from what it is tied to, and it is kept within the rope's length of it however far "
	                "it is told to walk; untied, the rope goes back to the hand.",
	    .framing = {.shot = Shot::Overview, .include = {{-120.0f, 60.0f}, {120.0f, 60.0f}}},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, 0.0f})},
	    .objects = {{.type = TreeInfo::Oak, .offset = {-40.0f, 10.0f}},
	                {.type = FeatureInfo::FatPilarChalk, .offset = {40.0f, 10.0f}}},
	    .commands = {Leash(0, LeashType::Rope, 1.0f), OnObject(Kind::TieLeash, 0, 0, 1.0f, false),
	                 Go(Kind::WalkTo, 0, {-120.0f, 60.0f}, 1.0f, false), Act(Kind::UntieLeash, 0, 8.0f),
	                 OnObject(Kind::TieLeash, 0, 1, 2.0f, false), Go(Kind::WalkTo, 0, {120.0f, 60.0f}, 1.0f, false),
	                 Act(Kind::TakeOffLeash, 0, 8.0f)},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "leash.hand",
	    .name = "Leash your creature with the hand",
	    .facet = Facet::Leash,
	    .description = "Your tiger stands to the north. The hand clicks it with the right button, putting the learning "
	                   "leash on. Move the hand about to lead it; press L or shake the hand to take the leash off "
	                   "again, or right click it yourself to put it back on.",
	    .expected = "A rope runs from the hand to its collar; pulled taut, the tiger walks to the hand. The readout says "
	                "\"on\".",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 60.0f})},
	    .commands = {{.kind = Kind::HandTapLeash, .creature = 0, .delaySeconds = 1.0f}},
	});

	all.push_back({
	    .id = "leash.someone_elses",
	    .name = "Another player's creature refuses the leash",
	    .facet = Facet::Leash,
	    .description = "Your tiger on the left and player two's lion on the right. Your hand taps the lion, then your "
	                   "tiger.",
	    .expected = "The lion is refused, as it belongs to another player, and the log says so; then your tiger is "
	                "leashed. Right clicking the lion yourself is refused the same way.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {-30.0f, 60.0f}),
	                  [] {
		                  auto lion = Content(CreatureType::Lion, {30.0f, 60.0f});
		                  lion.owner = PlayerNames::PLAYER_TWO;
		                  return lion;
	                  }()},
	    .commands = {{.kind = Kind::HandTapLeash, .creature = 1, .delaySeconds = 1.0f},
	                 {.kind = Kind::HandTapLeash, .creature = 0, .delaySeconds = 2.0f}},
	});

	all.push_back({
	    .id = "leash.only_leashable",
	    .name = "Only the leashable one of two",
	    .facet = Facet::Leash,
	    .description = "You have two tigers: the left one is the one you can lead, the right one isn't. The hand taps "
	                   "the right one, then the left one.",
	    .expected = "The right tiger is refused as it isn't the one you can lead; the left one is leashed.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {-30.0f, 60.0f}), Content(CreatureType::Tiger, {30.0f, 60.0f})},
	    .commands = {{.kind = Kind::HandTapLeash, .creature = 1, .delaySeconds = 1.0f},
	                 {.kind = Kind::HandTapLeash, .creature = 0, .delaySeconds = 2.0f}},
	});

	all.push_back({
	    .id = "leash.switch",
	    .name = "Switch the leashable creature",
	    .facet = Facet::Leash,
	    .description = "Your left tiger is on the leash. The right tiger is made the one you can lead, then the hand "
	                   "taps the left one and the right one.",
	    .expected = "Made leashable, the right tiger takes over: the left one's leash comes off and tapping it is "
	                "refused, and the right one is leashed.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {-30.0f, 60.0f}), Content(CreatureType::Tiger, {30.0f, 60.0f})},
	    .commands = {{.kind = Kind::HandTapLeash, .creature = 0, .delaySeconds = 1.0f},
	                 {.kind = Kind::MakeLeashable, .creature = 1, .delaySeconds = 4.0f},
	                 {.kind = Kind::HandTapLeash, .creature = 0, .delaySeconds = 2.0f},
	                 {.kind = Kind::HandTapLeash, .creature = 1, .delaySeconds = 2.0f}},
	});

	all.push_back({
	    .id = "leash.keys",
	    .name = "The leash shortcuts",
	    .facet = Facet::Leash,
	    .description = "Your tiger knows all three leashes. The keys are pressed in turn: L puts the learning leash "
	                   "on, B steps down to the aggression leash, V steps back up to the learning leash and on up to "
	                   "the compassion leash, and L takes it off, however many leashes it knows.",
	    .expected = "The rope goes on, changes colour three times, and comes off; the readout names each leash. "
	                "Pressing L, V and B yourself does the same.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 60.0f})},
	    .commands = {{.kind = Kind::LeashKey, .delaySeconds = 1.0f, .value = 0},
	                 {.kind = Kind::LeashKey, .delaySeconds = 3.0f, .value = 2},
	                 {.kind = Kind::LeashKey, .delaySeconds = 3.0f, .value = 1},
	                 {.kind = Kind::LeashKey, .delaySeconds = 3.0f, .value = 1},
	                 {.kind = Kind::LeashKey, .delaySeconds = 3.0f, .value = 0}},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "leash.click_or_hold",
	    .name = "Hold to stroke, right click to leash",
	    .facet = Facet::Leash,
	    .description = "The hand holds on to your tiger with the right button, stroking its head and belly, and lets go; "
	                   "then it clicks the tiger, which puts the leash on. Try it yourself: holding the right button on "
	                   "it for a second strokes it, sweeping the held hand across it slaps it, and a quick right click "
	                   "leashes it.",
	    .expected = "The hold strokes it without leashing it; the click then puts the rope on without stroking it.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 50.0f})},
	    .commands = {Stroke(0, creature_feedback::BodyPart::Head, 1.0f),
	                 Stroke(0, creature_feedback::BodyPart::Belly, 2.5f),
	                 Act(Kind::HandLetGo, 0, 2.0f, true),
	                 {.kind = Kind::HandTapLeash, .creature = 0, .delaySeconds = 2.0f}},
	});

	all.push_back({
	    .id = "leash.shake",
	    .name = "Shake the leash off",
	    .facet = Facet::Leash,
	    .description = "Your tiger is clicked to put the leash on, and the hand is then shaken back and forth. Shake the "
	                   "hand yourself (move the mouse quickly side to side with no button held) to try it.",
	    .expected = "The rope goes on, and comes off once the hand is shaken; the readout says \"off\".",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 50.0f})},
	    .commands = {{.kind = Kind::HandTapLeash, .creature = 0, .delaySeconds = 1.0f},
	                 {.kind = Kind::LeashShake, .creature = 0, .delaySeconds = 4.0f}},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "leash.key_off",
	    .name = "L takes the leash off",
	    .facet = Facet::Leash,
	    .description = "Your tiger is clicked to put the leash on, then L is pressed.",
	    .expected = "The rope goes on, then comes off with L.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {0.0f, 50.0f})},
	    .commands = {{.kind = Kind::HandTapLeash, .creature = 0, .delaySeconds = 1.0f},
	                 {.kind = Kind::LeashKey, .delaySeconds = 4.0f, .value = 0}},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "leash.other_gods_creatures",
	    .name = "Slapping other gods' creatures",
	    .facet = Facet::Leash,
	    .description = "Your tiger on the left, a tortoise of player three (as Khazar's is) in the middle and a wolf of "
	                   "player four (as Lethys's is) on the right. The hand slaps the tortoise and the wolf, as it may "
	                   "any god's creature, then tries to leash the tortoise.",
	    .expected = "The tortoise and the wolf are slapped and reel. Leashing the tortoise is refused, as it isn't "
	                "yours, and the log says so.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Content(CreatureType::Tiger, {-40.0f, 60.0f}),
	                  [] {
		                  auto tortoise = Content(CreatureType::Tortoise, {0.0f, 60.0f});
		                  tortoise.owner = PlayerNames::PLAYER_THREE;
		                  return tortoise;
	                  }(),
	                  [] {
		                  auto wolf = Content(CreatureType::Wolf, {40.0f, 60.0f});
		                  wolf.owner = PlayerNames::PLAYER_FOUR;
		                  return wolf;
	                  }()},
	    .commands = {Slap(1, 0.85f, false, true, 1.0f),
	                 Act(Kind::HandLetGo, 1, 2.0f, true),
	                 Slap(2, 0.85f, false, true, 1.0f),
	                 Act(Kind::HandLetGo, 2, 2.0f, true),
	                 {.kind = Kind::HandTapLeash, .creature = 1, .delaySeconds = 1.0f}},
	});

	all.push_back({
	    .id = "leash.home",
	    .name = "Kept at home",
	    .facet = Facet::Leash,
	    .description = "A tiger is kept within 40 units of where it stands, as a young creature is kept near its home, "
	                   "and told to walk 120 units away.",
	    .expected = "Once it strays past the radius it turns back and walks home again.",
	    .framing = {.shot = Shot::Overview, .include = {{120.0f, 0.0f}, {-40.0f, 0.0f}}},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, 0.0f}, 90.0f)},
	    .commands = {{.kind = Kind::ConfineToHome, .delaySeconds = 0.5f, .radius = 40.0f},
	                 Go(Kind::WalkTo, 0, {120.0f, 0.0f}, 1.0f),
	                 Go(Kind::WalkTo, 0, {0.0f, 0.0f}, 6.0f)},
	    .repeatFrom = 1,
	});
}

Command Fight(Kind kind, size_t creature, size_t value, float delay, float chargeMs = 0.0f)
{
	return {.kind = kind, .creature = creature, .delaySeconds = delay, .value = value, .chargeMs = chargeMs};
}

/// A content tiger for a fight, its owner, strength and alignment given
CreatureSetup Fighter(glm::vec2 offset, float facing, std::string_view label, PlayerNames owner, float strength,
                      float alignment)
{
	auto creature = Content(CreatureType::Tiger, offset, facing, label);
	creature.owner = owner;
	creature.strength = strength;
	creature.alignment = alignment;
	return creature;
}

void AddCombat(std::vector<Scenario>& all)
{
	constexpr size_t k_High = 0;
	constexpr size_t k_Mid = 1;
	constexpr size_t k_Low = 2;
	constexpr size_t k_Forward = 0;
	constexpr size_t k_Back = 1;
	const glm::vec2 left {-25.0f, 40.0f};
	const glm::vec2 right {25.0f, 40.0f};
	const auto red = [&](PlayerNames owner, float strength, float alignment) {
		return Fighter(left, 90.0f, "red", owner, strength, alignment);
	};
	const auto blue = [&](PlayerNames owner, float strength, float alignment) {
		return Fighter(right, 270.0f, "blue", owner, strength, alignment);
	};
	const Framing ringside {.shot = Shot::Overview, .include = {{-45.0f, 40.0f}, {45.0f, 40.0f}}, .distance = 0.8f};

	all.push_back({
	    .id = "combat.ai_duel",
	    .name = "Two tigers fight by themselves",
	    .facet = Facet::Combat,
	    .description = "Two somewhat evil tigers, so aggressive fighters, the red one stronger, are told to fight, and both "
	                   "fight by themselves.",
	    .expected = "They walk to their places either side of the arena, face each other and maybe taunt, then duel: "
	                "blows high, in the middle and low with their hit wobbles and reeling, blocks and steps. The panel at "
	                "the top left shows both fight healths falling. The loser faints, and the winner finishes and shows "
	                "off.",
	    .framing = ringside,
	    .creatures = {red(PlayerNames::PLAYER_ONE, 0.9f, -0.45f), blue(PlayerNames::PLAYER_TWO, 0.3f, -0.45f)},
	    .commands = {Fight(Kind::StartFight, 0, 1, 1.0f), Fight(Kind::FightAuto, 0, 1, 0.2f)},
	});

	all.push_back({
	    .id = "combat.player",
	    .name = "Directing the player's creature",
	    .facet = Facet::Combat,
	    .description = "The player's red tiger fights a blue tiger that fights by itself. The red one is given blows "
	                   "high, in the middle and low, a step forward, a block and a fully charged high blow in turn, as "
	                   "clicking the blue tiger's head, middle and legs, the ground ahead, and the red tiger itself would. "
	                   "Click them yourself to direct it too.",
	    .expected = "Each order plays at once: the blow chosen lands at the height asked for, stepping in first when "
	                "out of reach; the block holds until the next order. Left alone for 15 seconds, it fights by "
	                "itself.",
	    .framing = ringside,
	    .creatures = {red(PlayerNames::PLAYER_ONE, 0.5f, 0.0f), blue(PlayerNames::PLAYER_TWO, 0.5f, 0.3f)},
	    .commands = {Fight(Kind::StartFight, 0, 1, 1.0f), Fight(Kind::FightBlow, 0, k_High, 8.0f, 400.0f),
	                 Fight(Kind::FightBlow, 0, k_Mid, 2.5f, 400.0f), Fight(Kind::FightBlow, 0, k_Low, 2.5f, 400.0f),
	                 Fight(Kind::FightStep, 0, k_Forward, 2.5f), Fight(Kind::FightBlock, 0, 0, 2.0f),
	                 Fight(Kind::FightBlow, 0, k_High, 3.0f, creature_fight::k_MaxChargeMs)},
	    .repeatFrom = 1,
	});

	all.push_back({
	    .id = "combat.blocking",
	    .name = "Blocking an aggressive opponent",
	    .facet = Facet::Combat,
	    .description = "The player's red tiger blocks while an evil blue tiger, as aggressive as fighters get, attacks "
	                   "it; every twelve seconds the red one steps back out of its block and blocks again.",
	    .expected = "The red tiger raises its guard and holds it. Blows on it make it reel back in its block and take "
	                "a tenth of the damage, leaving no wounds; the blue tiger attacks again and again.",
	    .framing = ringside,
	    .creatures = {red(PlayerNames::PLAYER_ONE, 0.5f, 0.0f), blue(PlayerNames::PLAYER_TWO, 0.8f, -0.9f)},
	    .commands = {Fight(Kind::StartFight, 0, 1, 1.0f), Fight(Kind::FightBlock, 0, 0, 7.0f),
	                 Fight(Kind::FightStep, 0, k_Back, 12.0f), Fight(Kind::FightBlock, 0, 0, 1.5f)},
	    .repeatFrom = 2,
	});

	all.push_back({
	    .id = "combat.charged",
	    .name = "Charged and quick blows",
	    .facet = Facet::Combat,
	    .description = "The player's red tiger strikes a good, defensive blue tiger with blows in the middle, in turn "
	                   "let go at once and held the full 1.2 seconds.",
	    .expected = "A quick click's blow plays at half speed and does little; a fully charged blow plays half as fast "
	                "again as normal and takes three times as much off the blue tiger's fight health.",
	    .framing = ringside,
	    .creatures = {red(PlayerNames::PLAYER_ONE, 0.5f, 0.0f), blue(PlayerNames::PLAYER_TWO, 0.5f, 0.9f)},
	    .commands = {Fight(Kind::StartFight, 0, 1, 1.0f), Fight(Kind::FightBlow, 0, k_Mid, 8.0f, 0.0f),
	                 Fight(Kind::FightBlow, 0, k_Mid, 4.0f, creature_fight::k_MaxChargeMs)},
	    .repeatFrom = 1,
	});

	auto worn = Fighter({0.0f, 0.0f}, 0.0f, "worn", PlayerNames::PLAYER_ONE, 0.5f, 0.0f);
	worn.hold = false;
	worn.pauseMind = true;
	worn.needs.life = 0.6f;
	worn.needs.exhaustion = 0.7f;
	all.push_back({
	    .id = "combat.faint_recovery",
	    .name = "Knocked out, taken home and back up",
	    .facet = Facet::Combat,
	    .description = "A worn tiger, life 0.6 and exhaustion 0.7, is given a home where it stands, walks 90 units away "
	                   "and is knocked out there as a fight's loser is.",
	    .expected = "It faints and lies out cold with its eyes closed for 12 seconds, is taken home, lies there a few "
	                "seconds more, then rests until it is no more exhausted than 0.3, and gets up.",
	    .framing = {.shot = Shot::Overview, .include = {{90.0f, 30.0f}, {-20.0f, 0.0f}}, .distance = 0.6f},
	    .creatures = {worn},
	    .commands = {{.kind = Kind::ConfineToHome, .delaySeconds = 0.5f, .radius = 400.0f},
	                 Go(Kind::WalkTo, 0, {90.0f, 30.0f}, 0.5f, false),
	                 Act(Kind::KnockOut, 0, 1.0f, true)},
	});

	all.push_back({
	    .id = "combat.leash",
	    .name = "Leashed onto another creature",
	    .facet = Facet::Combat,
	    .description = "The player's red tiger is put on the aggression leash, which is tied to the blue tiger.",
	    .expected = "Tied to another creature, it fights it: both walk to the arena and duel, the red one fighting by "
	                "itself once left alone for three seconds.",
	    .framing = ringside,
	    .creatures = {red(PlayerNames::PLAYER_ONE, 0.6f, 0.0f), blue(PlayerNames::PLAYER_TWO, 0.5f, 0.0f)},
	    .commands = {Leash(0, LeashType::Evil, 1.0f), Fight(Kind::TieLeashToCreature, 0, 1, 2.0f)},
	});

	auto angry = red(PlayerNames::PLAYER_ONE, 0.6f, -0.3f);
	angry.desires = OnlyDesire(Desire::Anger);
	all.push_back({
	    .id = "combat.anger",
	    .name = "An angry creature picks a fight",
	    .facet = Facet::Combat,
	    .description = "A red tiger as angry as it gets, wanting nothing else, stands near a blue tiger, with angry "
	                   "creatures picking fights.",
	    .expected = "It picks a fight with the blue tiger by itself, and they duel. Once it is over, it waits two "
	                "minutes before picking another.",
	    .environment = {.angerStartsFights = true},
	    .framing = ringside,
	    .creatures = {angry, blue(PlayerNames::PLAYER_TWO, 0.5f, 0.0f)},
	});

	auto lion = Fighter(left, 90.0f, "evil lion", PlayerNames::PLAYER_TWO, 1.0f, -0.9f);
	lion.species = CreatureType::Lion;
	lion.size = 1.3f;
	auto small = Fighter(right, 270.0f, "small tiger", PlayerNames::PLAYER_ONE, 0.1f, 0.8f);
	small.size = 0.8f;
	small.needs.life = 0.3f;
	// Left to heal as it rests after being knocked out
	small.hold = false;
	all.push_back({
	    .id = "combat.evil_winner",
	    .name = "An evil winner has a poo on the loser",
	    .facet = Facet::Combat,
	    .description = "A big, strong, evil lion fights a small, weak, good tiger with little life left, so starting "
	                   "with little fight health, both by themselves.",
	    .expected = "The tiger soon faints. The lion finishes, walks up to it and has a poo on it, rather than showing "
	                "off as a good winner does.",
	    .framing = ringside,
	    .creatures = {lion, small},
	    .commands = {Fight(Kind::StartFight, 0, 1, 1.0f), Fight(Kind::FightAuto, 1, 1, 0.2f)},
	});
}

void AddMind(std::vector<Scenario>& all)
{
	// The game's belief type for villagers, the deed of playing with a toy, and the first skill of the game's table
	constexpr size_t k_VillagerBelief = 6;
	constexpr size_t k_PlayWithToy = 41;
	constexpr size_t k_FirstSkill = 0;
	const auto setDesire = [](Desire desire, float amount, float delay) {
		return Command {.kind = Kind::SetDesire,
		                .creature = 0,
		                .delaySeconds = delay,
		                .value = static_cast<size_t>(desire),
		                .amount = amount};
	};

	all.push_back({
	    .id = "mind.reward_villagers",
	    .name = "Learning: reward eating villagers, punish eating food",
	    .facet = Facet::Mind,
	    .description = "A hungry tiger with a villager right by it, lots of magic food close by and more villagers further "
	                   "off. Every 18 seconds its "
	                   "hunger is set to 40% of its most, enough for one meal. Whatever it eats, as soon as it has eaten, "
	                   "it is stroked if it was a villager and slapped if it was anything else.",
	    .expected = "First it eats the villager beside it and is stroked: with one example, its hunger tree finds "
	                "everything good. Then it eats the nearest food and is slapped: now the tree tells them apart. From then "
	                "on it walks past the food to eat villagers, the "
	                "planner's goal usefulness for villagers near 0.8 and for food near 0. The spawner's Mind panel shows "
	                "the tree and the thoughts.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .needs = {.energy = 0.3f, .exhaustion = 0.0f, .dehydration = 0.0f, .poo = 0.0f},
	        .desires = OnlyDesire(Desire::Hunger),
	    }},
	    .objects = {{.type = VillagerInfo::CelticLeaderMale, .offset = {12.0f, -12.0f}},
	                {.type = MobileObjectInfo::MagicFood, .offset = {-25.0f, -25.0f}},
	                {.type = MobileObjectInfo::MagicFood, .offset = {25.0f, -30.0f}},
	                {.type = MobileObjectInfo::MagicFood, .offset = {0.0f, -40.0f}},
	                {.type = MobileObjectInfo::MagicFood, .offset = {-35.0f, 15.0f}},
	                {.type = MobileObjectInfo::MagicFood, .offset = {35.0f, 20.0f}},
	                {.type = VillagerInfo::CelticFarmerMale, .offset = {70.0f, -60.0f}},
	                {.type = VillagerInfo::CelticForesterMale, .offset = {-70.0f, -60.0f}},
	                {.type = VillagerInfo::CelticHousewifeFemale, .offset = {80.0f, 30.0f}},
	                {.type = VillagerInfo::CelticFishermanMale, .offset = {-80.0f, 40.0f}},
	                {.type = VillagerInfo::CelticShepherdMale, .offset = {0.0f, 85.0f}},
	                {.type = VillagerInfo::CelticTraderMale, .offset = {60.0f, 75.0f}}},
	    .commands = {{.kind = Kind::RewardIf, .creature = 0, .delaySeconds = 1.0f, .value = k_VillagerBelief},
	                 setDesire(Desire::Hunger, 0.4f, 1.0f),
	                 Act(Kind::Stop, 0, 17.0f)},
	    .repeatFrom = 1,
	});

	all.push_back({
	    .id = "mind.community",
	    .name = "Load a chosen creature mind",
	    .facet = Facet::Mind,
	    .description = "A wolf that takes up the mind file last opened with the spawner's Open mind file button (a "
	                   "community-made creature, say), or else the game's own Khazar creature mind, with villagers, "
	                   "food, trees and a ball about it.",
	    .expected = "Its name, desires, opinions and the examples of its decision trees come from the file (the spawner's "
	                "Mind panel lists them, and its thoughts). The planner weighs what it has learnt: it goes for the "
	                "things its trees rate well. If neither file can be read the wolf keeps a fresh mind and the log "
	                "says why.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {CreatureSetup {.species = CreatureType::Wolf, .mindFile = "chosen:KhazarCreature"}},
	    .objects = {{.type = MobileObjectInfo::MagicFood, .offset = {-30.0f, -30.0f}},
	                {.type = MobileObjectInfo::Ball, .offset = {30.0f, -20.0f}},
	                {.type = TreeInfo::Beech, .offset = {-50.0f, 40.0f}},
	                {.type = TreeInfo::Beech, .offset = {55.0f, 45.0f}},
	                {.type = VillagerInfo::CelticFarmerMale, .offset = {60.0f, -50.0f}},
	                {.type = VillagerInfo::CelticHousewifeFemale, .offset = {-60.0f, -55.0f}}},
	});

	std::vector<Command> phases;
	for (size_t phase = 1; phase <= k_LastPhase; ++phase)
	{
		phases.push_back({.kind = Kind::SetPhase, .creature = 0, .delaySeconds = 8.0f, .value = phase});
	}
	all.push_back({
	    .id = "mind.phases",
	    .name = "Stages of growing up unlock desires",
	    .facet = Facet::Mind,
	    .description = "A cow that starts at the first stage of growing up and moves on a stage every eight seconds, "
	                   "through all fourteen.",
	    .expected = "At first only fear, tiredness, attention, showing how it is and the like are active; hunger, "
	                "curiosity and play come at the second stage, water at the fourth, anger at the eleventh, compassion "
	                "at the twelfth. The Mind panel's desires list grows, and the planner starts weighing what each new "
	                "desire can do.",
	    .framing = {.shot = Shot::Follow},
	    .creatures = {CreatureSetup {.species = CreatureType::Cow, .phase = 0}},
	    .objects = {{.type = MobileObjectInfo::MagicFood, .offset = {-30.0f, -30.0f}},
	                {.type = MobileObjectInfo::Ball, .offset = {30.0f, -20.0f}}},
	    .commands = phases,
	});

	all.push_back({
	    .id = "mind.mimic",
	    .name = "Mimic the player",
	    .facet = Facet::Mind,
	    .description = "A grown-up tiger with balls about it. Every 20 seconds the player plays with a toy near it, the "
	                   "one deed creatures copy without the learning leash.",
	    .expected = "Nine times in ten it notices: it turns to look where the player played; a few seconds later it "
	                "picks up a ball and throws it about, copying the player; then for a while it wants to play. The "
	                "Mind panel shows the stage of copying.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger, .needs = Content()}},
	    .objects = {{.type = MobileObjectInfo::Ball, .offset = {25.0f, -25.0f}},
	                {.type = MobileObjectInfo::Ball, .offset = {-30.0f, -20.0f}}},
	    .commands =
	        {{.kind = Kind::PlayerDid, .creature = 0, .delaySeconds = 3.0f, .point = {30.0f, -30.0f}, .value = k_PlayWithToy},
	         Act(Kind::Stop, 0, 20.0f)},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "mind.watch_skill",
	    .name = "Learn a skill by watching",
	    .facet = Facet::Mind,
	    .description = "A grown-up lion watches villagers build: it sees the skill now, again 4 seconds later and again "
	                   "4 seconds after that.",
	    .expected = "Once it has watched for longer than the skill takes to learn (6 seconds by the game's table) it "
	                "knows it: its thoughts say it has learnt to build, and the Mind panel marks the skill known.",
	    .framing = {.shot = Shot::Follow},
	    .creatures = {CreatureSetup {.species = CreatureType::Lion, .needs = Content()}},
	    .commands = {{.kind = Kind::SeeSkill, .creature = 0, .delaySeconds = 1.0f, .value = k_FirstSkill},
	                 {.kind = Kind::SeeSkill, .creature = 0, .delaySeconds = 4.0f, .value = k_FirstSkill},
	                 {.kind = Kind::SeeSkill, .creature = 0, .delaySeconds = 4.0f, .value = k_FirstSkill}},
	});
}

void AddParticles(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "particles.smoke_and_fire",
	    .name = "Smoke, steam and a bonfire",
	    .facet = Facet::Particles,
	    .description = "The spot visuals' smoke, steam, evil smoke and a bonfire, side by side on the plain.",
	    .expected = "Grey smoke and white steam puffs rise, spin and fade; the evil smoke is dark; the bonfire's flames "
	                "flicker in its additive sprites with smoke rising above them.",
	    .framing = {.shot = Shot::Overview, .include = {{-40.0f, 0.0f}, {40.0f, 0.0f}}, .distance = 0.3f},
	    .particles = {{.type = ParticleType::Smoke, .offset = {-30.0f, 0.0f}, .magnitude = 8.0f},
	                  {.type = ParticleType::Steam, .offset = {-10.0f, 0.0f}, .magnitude = 8.0f},
	                  {.type = ParticleType::EvilSmoke, .offset = {10.0f, 0.0f}, .magnitude = 8.0f},
	                  {.type = ParticleType::Bonfire, .offset = {30.0f, 0.0f}, .magnitude = 5.0f}},
	});

	all.push_back({
	    .id = "particles.sparkles",
	    .name = "Sparkles and magic",
	    .facet = Facet::Particles,
	    .description = "Short effects started again every few seconds: the failed cast, the hand gripping the land, a "
	                   "magic object made, an object appearing in the second player's colour, and a forest made.",
	    .expected = "Each plays out in sprites from the sprite sheets and starts again: the cross of the failed cast, "
	                "a ring of sparks, a spinning cloud of sparks, sparks fountaining up in green, and falling leaves.",
	    .framing = {.shot = Shot::Overview, .include = {{-30.0f, 0.0f}, {30.0f, 0.0f}}, .distance = 0.45f},
	    .particles =
	        {{.type = ParticleType::SpellFail, .offset = {-30.0f, 0.0f}, .height = 5.0f, .restartSeconds = 4.0f},
	         {.type = ParticleType::GripLandscape, .offset = {-15.0f, 0.0f}, .restartSeconds = 3.0f},
	         {.type = ParticleType::MagicObjectCreated,
	          .offset = {0.0f, 0.0f},
	          .height = 3.0f,
	          .magnitude = 3.0f,
	          .restartSeconds = 5.0f},
	         {.type = ParticleType::ObjectAppear,
	          .offset = {15.0f, 0.0f},
	          .magnitude = 3.0f,
	          .player = 1,
	          .restartSeconds = 4.0f},
	         {.type = ParticleType::ForestCreated, .offset = {30.0f, 0.0f}, .magnitude = 1.0f, .restartSeconds = 6.0f}},
	});

	all.push_back({
	    .id = "particles.models",
	    .name = "Models among the sparks",
	    .facet = Facet::Particles,
	    .description = "Effects whose particles are models: the vortex rising where a magic object is made, the glowing "
	                   "cone of the beam marking a place, the paper streamers of the ticker tape and the fireball's flash.",
	    .expected = "A spinning additive vortex model rises and fades; a glowing cone turns to face the camera wherever it "
	                "looks from; coloured streamers fall from a disk; the flash's model plays through its texture's frames.",
	    .framing = {.shot = Shot::Overview, .include = {{-30.0f, 0.0f}, {30.0f, 0.0f}}, .distance = 0.4f},
	    .particles = {{.file = "SF_MagicObjectCreated2", .offset = {-30.0f, 0.0f}, .magnitude = 2.0f, .restartSeconds = 5.0f},
	                  {.file = "SF_SeeThisBeam", .offset = {-10.0f, 0.0f}, .magnitude = 1.0f, .restartSeconds = 6.0f},
	                  {.file = "SF_TickerTape", .offset = {10.0f, 0.0f}, .height = 15.0f, .restartSeconds = 6.0f},
	                  {.file = "SF_Flash", .offset = {30.0f, 0.0f}, .height = 3.0f, .magnitude = 2.0f, .restartSeconds = 3.0f}},
	});

	all.push_back({
	    .id = "particles.light_maps",
	    .name = "Light on the land",
	    .facet = Facet::Particles,
	    .description = "Effects that light the ground under them, as fire and lightning do, at night so the light shows: "
	                   "the ground effect's ring and the light a landscape vortex leaves, each many cells across.",
	    .expected = "A ring of light spreads over the ground and fades, again and again; a steady pool of coloured light "
	                "lies on the ground beside it, lighting what stands there too.",
	    .environment = {.hour = 23.0f},
	    .framing = {.shot = Shot::Overview, .include = {{-120.0f, 0.0f}, {120.0f, 0.0f}}, .distance = 0.4f},
	    .particles = {{.file = "SF_GroundEffect", .offset = {-70.0f, 0.0f}, .magnitude = 2.0f, .restartSeconds = 4.0f},
	                  {.file = "SF_LandscapeVortexLightMap", .offset = {70.0f, 0.0f}}},
	});

	all.push_back({
	    .id = "particles.mist",
	    .name = "Mist over the holders",
	    .facet = Facet::Particles,
	    .description = "The water and storm miracles' mists, as they sit over a holder, and the water miracle in the hand.",
	    .expected = "Puffs of mist, shaped wider than tall, turn to face the camera, shrink seen edge on and grow seen from "
	                "above, and play through their frames of the smoke texture.",
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 0.0f}, {20.0f, 0.0f}}, .distance = 0.3f},
	    .particles = {{.file = "SF_WaterOnHolder", .offset = {-15.0f, 0.0f}, .height = 4.0f, .magnitude = 3.0f},
	                  {.file = "SF_LightningStormOnHolder", .offset = {0.0f, 0.0f}, .height = 4.0f, .magnitude = 3.0f},
	                  {.file = "SF_WaterInHand", .offset = {15.0f, 0.0f}, .height = 4.0f, .magnitude = 3.0f}},
	});

	all.push_back({
	    .id = "particles.symbols_and_heal",
	    .name = "Player symbols and the heal chakra",
	    .facet = Facet::Particles,
	    .description = "The player icon fountain throwing up the casting player's symbol in each player's colour, and the "
	                   "heal miracle's chakra over two creatures.",
	    .expected = "Symbols between two glows of the player's colour, one turning, arc up out of each fountain and fade; "
	                "a chakra appears over each creature and follows it, a burst of sparks under it rising and fading.",
	    .framing = {.shot = Shot::Overview, .include = {{-30.0f, 0.0f}, {30.0f, 0.0f}}, .distance = 0.45f},
	    .creatures = {{.offset = {15.0f, 0.0f}}, {.offset = {30.0f, 5.0f}}},
	    .particles = {{.file = "SF_PlayerIconFountain", .offset = {-30.0f, 0.0f}, .magnitude = 2.0f, .player = 0},
	                  {.file = "SF_PlayerIconFountain", .offset = {-15.0f, 0.0f}, .magnitude = 2.0f, .player = 1},
	                  {.file = "SF_HealChakra",
	                   .targetsCreatures = true,
	                   .offset = {22.0f, 0.0f},
	                   .magnitude = 2.0f,
	                   .restartSeconds = 6.0f}},
	});

	all.push_back({
	    .id = "particles.draw_paths",
	    .name = "Sorted and queued sprites",
	    .facet = Facet::Particles,
	    .description = "The same smoke drawn three ways side by side: each puff in its own place among what blends, the "
	                   "whole effect at its origin as some spot visuals are, and as the miracle in the hand is.",
	    .expected = "The sorted smoke's puffs blend far to near from any side; the queued smoke draws newest first as one, "
	                "so seen from above older puffs cover newer ones; the third matches the queued without a hand.",
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 0.0f}, {20.0f, 0.0f}}, .distance = 0.3f},
	    .particles = {{.type = ParticleType::Smoke, .offset = {-20.0f, 0.0f}, .magnitude = 8.0f},
	                  {.type = ParticleType::Smoke,
	                   .path = particles::draw::DrawPath::Queued,
	                   .offset = {0.0f, 0.0f},
	                   .magnitude = 8.0f},
	                  {.type = ParticleType::Smoke,
	                   .path = particles::draw::DrawPath::Immediate,
	                   .offset = {20.0f, 0.0f},
	                   .magnitude = 8.0f}},
	});
}

void AddEditor(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "editor.one_of_each",
	    .name = "One of each kind to pick",
	    .facet = Facet::Editor,
	    .description = "A tiger, a villager, an oak, a pillar of rock and a ball, spread out in view, for trying the "
	                   "editor (F2) on: the outliner lists each under its kind, a click on the land or in the list "
	                   "picks one, and the inspector shows it.",
	    .expected = "Each picks with a box drawn round it. The Move (E) and Rotate (R) tools drag the picked thing over "
	                "the land and turn it; Ctrl+D copies it beside itself and Delete removes it. The tiger's inspector "
	                "has every creature section.",
	    .environment = {.clockRuns = false},
	    .framing = {.shot = Shot::Overview, .include = {{-40.0f, 30.0f}, {40.0f, -30.0f}}},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, 0.0f})},
	    .objects = {{.type = VillagerInfo::CelticFarmerMale, .offset = {-25.0f, -10.0f}},
	                {.type = TreeInfo::Oak, .offset = {-40.0f, 20.0f}},
	                {.type = FeatureInfo::FatPilarChalk, .offset = {40.0f, 20.0f}},
	                {.type = MobileObjectInfo::Ball, .offset = {20.0f, -15.0f}}},
	});

	all.push_back({
	    .id = "editor.orbit_walker",
	    .name = "A walking creature to orbit and follow",
	    .facet = Facet::Editor,
	    .description = "A tiger walks round a square 80 units across, again and again, for trying the editor's cameras "
	                   "on it: pick it, then Orbit (O) or Follow (Shift+O).",
	    .expected = "Orbiting, the camera keeps round the tiger as it walks, turned by dragging with the right button "
	                "and drawn in and out by the wheel. Following, it eases along behind the tiger as it turns each "
	                "corner. Free, or Escape, hands the camera back where it is.",
	    .framing = {.shot = Shot::Overview, .include = {{-40.0f, -40.0f}, {40.0f, 40.0f}}},
	    .creatures = {Posed(CreatureType::Tiger, {-40.0f, -40.0f})},
	    .commands = {Go(Kind::WalkTo, 0, {40.0f, -40.0f}, 0.5f), Go(Kind::WalkTo, 0, {40.0f, 40.0f}, 0.5f),
	                 Go(Kind::WalkTo, 0, {-40.0f, 40.0f}, 0.5f), Go(Kind::WalkTo, 0, {-40.0f, -40.0f}, 0.5f)},
	    .repeatFrom = 0,
	});
}

/// A benchmark of a crowd of the size, to compare runs of several sizes and see which costs grow faster than the crowd
/// does
void AddCrowd(std::vector<Scenario>& all, Crowd::Kind kind, size_t count, std::string_view id, std::string_view name)
{
	const bool creatures = kind == Crowd::Kind::Creatures;
	all.push_back({
	    .id = id,
	    .name = name,
	    .facet = Facet::Benchmark,
	    .description = creatures ? "Creatures of every species and of four players, spread evenly over the land round the "
	                               "middle and clear of the lake, the same way every run. Nothing holds them: their minds, "
	                               "needs, walking and route finding all run as in the game."
	                             : "Villagers of every tribe and role in towns of fifty, each a ring of huts about a storage "
	                               "pit, spread evenly over the land round the middle and clear of the lake, the same way "
	                               "every run, and left to go about their lives.",
	    .expected = "They spawn a batch a frame, then the frames are measured once they have settled: the mean, 95th "
	                "percentile and slowest frame and the costliest profiler stages show as it runs, and Save results "
	                "writes them out to compare with the other sizes.",
	    // The crowd is spread over the middle, where the miracle dispensers would stand
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Testbed},
	    .crowd = Crowd {.kind = kind, .count = count, .seed = k_BenchmarkSeed, .perFrame = creatures ? 100u : 250u},
	});
}

void AddBenchmark(std::vector<Scenario>& all)
{
	// Steps close enough together that a cost growing faster than the crowd shows between them
	AddCrowd(all, Crowd::Kind::Creatures, 10, "benchmark.creatures_10", "10 active creatures");
	AddCrowd(all, Crowd::Kind::Creatures, 25, "benchmark.creatures_25", "25 active creatures");
	AddCrowd(all, Crowd::Kind::Creatures, 50, "benchmark.creatures_50", "50 active creatures");
	AddCrowd(all, Crowd::Kind::Creatures, 100, "benchmark.creatures_100", "100 active creatures");
	AddCrowd(all, Crowd::Kind::Villagers, 100, "benchmark.villagers_100", "100 active villagers");
	AddCrowd(all, Crowd::Kind::Villagers, 250, "benchmark.villagers_250", "250 active villagers");
	AddCrowd(all, Crowd::Kind::Villagers, 500, "benchmark.villagers_500", "500 active villagers");
	AddCrowd(all, Crowd::Kind::Villagers, 1'000, "benchmark.villagers_1000", "1,000 active villagers");
}

std::vector<Scenario> Build()
{
	std::vector<Scenario> all;
	AddIdle(all);
	AddExpressions(all);
	AddSenses(all);
	AddNeeds(all);
	AddGrowth(all);
	AddAppearance(all);
	AddLight(all);
	AddMovement(all);
	AddFootprints(all);
	AddAudio(all);
	AddObjects(all);
	AddHand(all);
	AddLeash(all);
	AddCombat(all);
	AddMind(all);
	AddParticles(all);
	AddEditor(all);
	AddMiracleScenarios(all);
	AddBenchmark(all);
	AddCreatureModeScenarios(all);
	return all;
}

bool InRange(float value, float low, float high)
{
	return value >= low && value <= high;
}

void CheckNeeds(const NeedOverrides& needs, std::vector<std::string>& problems, std::string_view who)
{
	const std::array<std::pair<std::string_view, std::optional<float>>, 5> k_UnitNeeds {{
	    {"exhaustion", needs.exhaustion},
	    {"dehydration", needs.dehydration},
	    {"poo", needs.poo},
	    {"life", needs.life},
	    {"energy", needs.energy},
	}};
	for (const auto& [name, value] : k_UnitNeeds)
	{
		if (value.has_value() && !InRange(*value, 0.0f, creature_physiology::k_MaxGrownSize))
		{
			problems.push_back(fmt::format("{}: {} {} out of range", who, name, *value));
		}
	}
	if (needs.warmth.has_value() && !InRange(*needs.warmth, -1.0f, 1.0f))
	{
		problems.push_back(fmt::format("{}: warmth {} out of range", who, *needs.warmth));
	}
}

bool ValidAnimation(Kind kind, size_t animation)
{
	switch (kind)
	{
	case Kind::PlayAction:
		return animation >= animations::k_FirstAction && animation < animations::k_FirstAction + animations::k_ActionCount;
	case Kind::PlayGesture:
		return animation >= animations::k_FirstGesture && animation < animations::k_FirstGesture + animations::k_GestureCount;
	case Kind::PullFace:
		return animation >= animations::k_FirstFace && animation < animations::k_FirstFace + animations::k_FaceCount;
	default:
		return true;
	}
}

bool ValidOffset(glm::vec2 offset)
{
	return std::abs(offset.x) <= k_MaxOffset && std::abs(offset.y) <= k_MaxOffset;
}

/// What is wrong with what a command acts on, if anything: the object it picks up must be a thing, what it knocks down
/// a thing or a tree, the part it strokes, the way it looks something over and the leash put on real ones
std::string_view CommandProblem(const Command& command, std::span<const ObjectSetup> objects)
{
	const auto* object = command.object < objects.size() ? &objects[command.object] : nullptr;
	switch (command.kind)
	{
	case Kind::PickUp:
		return object != nullptr && std::holds_alternative<MobileObjectInfo>(object->type) ? "" : "picks up no thing";
	case Kind::KnockDown:
		return object != nullptr && !std::holds_alternative<FeatureInfo>(object->type) ? "" : "knocks down no thing or tree";
	case Kind::TieLeash:
		return object != nullptr ? "" : "ties the leash to nothing";
	case Kind::Examine:
		return command.value < creature_object_actions::k_KeepAnimationCount ? "" : "no such way of looking it over";
	case Kind::HandStroke:
		return command.bodyPart < creature_feedback::k_BodyPartCount ? "" : "no such part of the body";
	case Kind::HandSlap:
		return command.slapHeight > 0.0f && command.slapHeight < creature_feedback::k_SlapAbove ? "" : "slaps above it";
	case Kind::PutOnLeash:
		return command.leash == LeashType::Rope || command.leash == LeashType::Evil || command.leash == LeashType::Good
		           ? ""
		           : "no such leash";
	case Kind::ConfineToHome:
		return command.radius > 0.0f ? "" : "keeps it nowhere";
	case Kind::LeashKey:
		return command.value < 3 ? "" : "no such leash key";
	case Kind::HandTapLeash:
		return command.player < PlayerNames::_COUNT ? "" : "no such player";
	case Kind::FightBlow:
		return command.value < 3 && command.chargeMs >= 0.0f && command.chargeMs <= creature_fight::k_MaxChargeMs
		           ? ""
		           : "no such blow or charge";
	case Kind::FightStep:
		return command.value < 4 ? "" : "no such step";
	default:
		return "";
	}
}
} // namespace

std::string_view testbed_scenarios::Name(Facet facet)
{
	constexpr std::array<std::string_view, k_FacetCount> k_Names {
	    "Idle",     "Expressions", "Senses", "Needs",    "Growth",    "Appearance",    "Light",
	    "Movement", "Footprints",  "Audio",  "Objects",  "Hand",      "Leash",         "Combat",
	    "Mind",     "Particles",   "Editor", "Miracles", "Benchmark", "Creature Mode",
	};
	return k_Names.at(static_cast<size_t>(facet));
}

std::string_view testbed_scenarios::Name(Weather weather)
{
	constexpr std::array<std::string_view, 5> k_Names {"clear", "rain", "thunderstorm", "snow", "blizzard"};
	return k_Names.at(static_cast<size_t>(weather));
}

std::string_view testbed_scenarios::Name(Shot shot)
{
	constexpr std::array<std::string_view, 4> k_Names {"testbed", "overview", "follow", "head"};
	return k_Names.at(static_cast<size_t>(shot));
}

std::string_view testbed_scenarios::Name(Command::Kind kind)
{
	constexpr std::array<std::string_view, 67> k_Names {
	    "walk to",
	    "run to",
	    "follow",
	    "flee from",
	    "turn to face",
	    "face the camera",
	    "stop",
	    "play action",
	    "gesture",
	    "pull face",
	    "show feeling",
	    "sit down",
	    "stand up",
	    "sleep",
	    "wake",
	    "eat",
	    "drink",
	    "poo",
	    "puke",
	    "faint",
	    "stroke",
	    "slap",
	    "set hour",
	    "pick up",
	    "put down",
	    "toss away",
	    "lob",
	    "eat it",
	    "look it over",
	    "throw at",
	    "knock down",
	    "point at",
	    "hand strokes",
	    "hand slaps",
	    "hand lets go",
	    "put on leash",
	    "tie leash to",
	    "untie leash",
	    "take off leash",
	    "keep at home",
	    "make leashable",
	    "hand taps to leash",
	    "leash key",
	    "shake the hand",
	    "start fight",
	    "fight blow",
	    "fight block",
	    "fight step",
	    "fight special",
	    "fight by itself",
	    "knock out",
	    "bring round",
	    "tie leash to creature",
	    "set desire",
	    "set stage",
	    "reward if",
	    "see skill",
	    "see miracle",
	    "player did",
	    "press C",
	    "double click",
	    "camera keys",
	    "clear the view",
	    "give the camera back",
	    "press F5",
	    "tattoo",
	    "take tattoo off",
	};
	return k_Names.at(static_cast<size_t>(kind));
}

bool NeedOverrides::Empty() const
{
	return !energy && !exhaustion && !dehydration && !poo && !life && !warmth && !age;
}

std::span<const Scenario> testbed_scenarios::All()
{
	static const std::vector<Scenario> k_All = Build();
	return k_All;
}

const Scenario* testbed_scenarios::Find(std::string_view id)
{
	const auto all = All();
	const auto found = std::ranges::find(all, id, &Scenario::id);
	return found != all.end() ? &*found : nullptr;
}

std::vector<std::string> testbed_scenarios::Problems(const Scenario& scenario)
{
	std::vector<std::string> problems;
	if (scenario.id.empty() || scenario.name.empty() || scenario.description.empty() || scenario.expected.empty())
	{
		problems.emplace_back("missing its id, name, description or what to expect");
	}
	if (static_cast<size_t>(scenario.facet) >= k_FacetCount)
	{
		problems.emplace_back("no such facet");
	}
	const auto& environment = scenario.environment;
	if (!InRange(environment.hour, 0.0f, k_HoursPerDay) || environment.bodyTimeScale < 0.0f)
	{
		problems.emplace_back("hour or body time out of range");
	}
	if (scenario.creatures.empty() && scenario.particles.empty() && scenario.miracles.empty() && scenario.dispensers.empty() &&
	    !environment.dispenserGrid && !scenario.crowd.has_value())
	{
		problems.emplace_back("no creatures, particles, miracles, dispensers or crowd");
	}
	if (scenario.crowd.has_value() && (scenario.crowd->count == 0 || scenario.crowd->perFrame == 0))
	{
		problems.emplace_back("a crowd of no one, or spawned none a frame");
	}
	for (const auto& miracle : scenario.miracles)
	{
		const auto type = static_cast<size_t>(miracle.type);
		if (type == 0 || type >= k_Miracles)
		{
			problems.emplace_back("a miracle of no such magic type");
		}
		if (miracle.target == MiracleCast::Target::Creature && miracle.creature >= scenario.creatures.size())
		{
			problems.emplace_back("a miracle cast on a creature that isn't there");
		}
		if (!ValidOffset(miracle.point) || !ValidOffset(miracle.handOffset) || miracle.delaySeconds < 0.0f ||
		    miracle.holdSeconds.value_or(0.0f) < 0.0f || miracle.repeatSeconds.value_or(1.0f) <= 0.0f)
		{
			problems.emplace_back("a miracle's points or times out of range");
		}
	}
	for (const auto& dispenser : scenario.dispensers)
	{
		const auto type = static_cast<size_t>(dispenser.type);
		if (type == 0 || type >= k_Miracles || !ValidOffset(dispenser.offset))
		{
			problems.emplace_back("a dispenser of no such magic type, or off the map");
		}
	}
	for (const auto& particle : scenario.particles)
	{
		if (particle.file.empty() && particles::ParticleTypeFile(particle.type).empty())
		{
			problems.emplace_back("a particle type that has no file");
		}
		if (particle.targetsCreatures && scenario.creatures.empty())
		{
			problems.emplace_back("a particle effect given creatures to act on, without creatures");
		}
		if (particle.magnitude < 0.0f || particle.restartSeconds < 0.0f)
		{
			problems.emplace_back("a particle effect's magnitude or restart out of range");
		}
	}
	const auto creatures = scenario.creatures.size();
	const auto& framing = scenario.framing;
	if ((framing.shot == Shot::Follow || framing.shot == Shot::Head) && framing.creature >= creatures)
	{
		problems.emplace_back("the camera frames a creature that isn't there");
	}
	if (framing.distance <= 0.0f || !std::ranges::all_of(framing.include, ValidOffset))
	{
		problems.emplace_back("the framing's distance or points are out of range");
	}
	for (size_t i = 0; i < creatures; ++i)
	{
		const auto& creature = scenario.creatures.at(i);
		const auto who = fmt::format("creature {}", i);
		const auto species = static_cast<int32_t>(creature.species);
		if (species < 1 || species >= static_cast<int32_t>(CreatureType::_COUNT))
		{
			problems.push_back(fmt::format("{}: no such species", who));
		}
		if (static_cast<size_t>(creature.owner) >= static_cast<size_t>(PlayerNames::_COUNT))
		{
			problems.push_back(fmt::format("{}: no such owner", who));
		}
		if (!ValidOffset(creature.offset))
		{
			problems.push_back(fmt::format("{}: off the map", who));
		}
		if ((creature.alignment && !InRange(*creature.alignment, -1.0f, 1.0f)) ||
		    (creature.fatness && !InRange(*creature.fatness, 0.0f, 1.0f)) ||
		    (creature.strength && !InRange(*creature.strength, 0.0f, 1.0f)) ||
		    (creature.size && !InRange(*creature.size, 0.05f, 4.0f)))
		{
			problems.push_back(fmt::format("{}: body out of range", who));
		}
		if (!creature.mindFile.empty() && !creature.mindFile.starts_with("chosen:") && !creature.mindFile.starts_with("game:"))
		{
			problems.push_back(fmt::format("{}: a mind file is chosen: or game:", who));
		}
		if (creature.phase && *creature.phase > k_LastPhase)
		{
			problems.push_back(fmt::format("{}: no such stage of growing up", who));
		}
		CheckNeeds(creature.needs, problems, who);
		for (const auto& desire : creature.desires)
		{
			if (static_cast<size_t>(desire.desire) >= creature_desires::k_DesireCount || !InRange(desire.fraction, 0.0f, 1.0f))
			{
				problems.push_back(fmt::format("{}: desire out of range", who));
			}
		}
		for (const auto& tattoo : creature.tattoos)
		{
			if (tattoo.design >= creature_tattoo::k_DesignCount || tattoo.Empty())
			{
				problems.push_back(fmt::format("{}: no such tattoo design or site", who));
			}
		}
		if (creature.tattoos.size() > creature_tattoo::k_SlotCount)
		{
			problems.push_back(fmt::format("{}: more tattoos than slots", who));
		}
		for (const auto& mark : creature.wounds)
		{
			if (mark.type >= creature_marks::k_WoundLifetimes.size() || mark.column >= 8 || mark.skin >= 4)
			{
				problems.push_back(fmt::format("{}: no such wound", who));
			}
		}
	}
	for (const auto& object : scenario.objects)
	{
		const bool valid = std::visit(
		    []<typename T>(T type) {
			    const auto index = static_cast<int32_t>(type);
			    return index >= 0 && index < static_cast<int32_t>(T::_COUNT);
		    },
		    object.type);
		if (!valid || object.scale <= 0.0f || !ValidOffset(object.offset))
		{
			problems.emplace_back("an object of no such kind, size or place");
		}
	}
	for (size_t i = 0; i < scenario.commands.size(); ++i)
	{
		const auto& command = scenario.commands.at(i);
		const auto what = fmt::format("command {} ({})", i, Name(command.kind));
		if (command.kind != Kind::SetHour && command.creature >= creatures)
		{
			problems.push_back(fmt::format("{}: no such creature", what));
		}
		if ((command.kind == Kind::Follow || command.kind == Kind::StartFight || command.kind == Kind::TieLeashToCreature) &&
		    (command.value >= creatures || command.value == command.creature))
		{
			problems.push_back(fmt::format("{}: acts on no other creature", what));
		}
		if (!ValidAnimation(command.kind, command.value))
		{
			problems.push_back(fmt::format("{}: animation {} isn't of its kind", what, command.value));
		}
		if (const auto problem = CommandProblem(command, scenario.objects); !problem.empty())
		{
			problems.push_back(fmt::format("{}: {}", what, problem));
		}
		if (!ValidOffset(command.point) || command.delaySeconds < 0.0f || !InRange(command.hour, 0.0f, k_HoursPerDay))
		{
			problems.push_back(fmt::format("{}: point, delay or hour out of range", what));
		}
		if ((command.kind == Kind::SetDesire &&
		     (command.value >= creature_desires::k_DesireCount || !InRange(command.amount, 0.0f, 1.0f))) ||
		    (command.kind == Kind::SetPhase && command.value > k_LastPhase) ||
		    (command.kind == Kind::ShowFeeling && command.value >= creature_face::k_CueCount) ||
		    (command.kind == Kind::SeeSkill && command.value >= k_Skills) ||
		    (command.kind == Kind::SeeMiracle && command.value >= k_Miracles) ||
		    (command.kind == Kind::PlayerDid && command.value >= k_Deeds) ||
		    (command.kind == Kind::CameraKeys && (command.value >= 4 || command.amount <= 0.0f)) ||
		    (command.kind == Kind::OpenCreatureCave && command.value >= creature_cave::k_PageCount) ||
		    ((command.kind == Kind::ApplyTattoo || command.kind == Kind::RemoveTattoo) &&
		     (command.value >= creature_tattoo::k_DesignCount || command.bodyPart >= creature_tattoo::k_SlotCount)))
		{
			problems.push_back(fmt::format("{}: value {} out of range", what, command.value));
		}
	}
	if (scenario.repeatFrom.has_value() && *scenario.repeatFrom >= scenario.commands.size())
	{
		problems.emplace_back("repeats from a command that isn't there");
	}
	return problems;
}

glm::vec2 testbed_scenarios::MapPoint(glm::vec2 middle, glm::vec2 offset)
{
	return middle + offset;
}

float testbed_scenarios::CreatureHeight(float size)
{
	// A creature of size 1 stands about 15 units tall, whatever its species' mesh
	constexpr float k_HeightPerSize = 15.0f;
	return k_HeightPerSize * std::max(size, 0.05f);
}

CameraPlacement testbed_scenarios::Overview(glm::vec3 centre, glm::vec2 halfSize, glm::vec2 fieldOfView, float distanceFactor)
{
	// Looking down from the south at this angle, far enough back for the box's width to fit across the view and its
	// depth, foreshortened, up and down it, with a margin round it for the creatures' heights
	constexpr float k_Pitch = 0.65f;
	constexpr float k_MinDistance = 40.0f;
	constexpr float k_Margin = 25.0f;
	const auto halfAngles = glm::clamp(fieldOfView * 0.5f, 0.1f, 1.5f);
	// The near side of the box is closer to the camera than its middle, so its width takes a wider angle
	const auto across = ((halfSize.x + k_Margin) / std::tan(halfAngles.x)) + ((halfSize.y + k_Margin) * std::cos(k_Pitch));
	const auto upAndDown = (halfSize.y + k_Margin) * std::sin(k_Pitch) / std::tan(halfAngles.y);
	const auto distance = distanceFactor * std::max({across, upAndDown, k_MinDistance});
	return {
	    .origin = centre + glm::vec3(0.0f, distance * std::sin(k_Pitch), -distance * std::cos(k_Pitch)),
	    .focus = centre,
	};
}

CameraPlacement testbed_scenarios::Follow(glm::vec3 position, float height, float distanceFactor)
{
	constexpr float k_MinDistance = 25.0f;
	const glm::vec3 focus = position + glm::vec3(0.0f, height * 0.5f, 0.0f);
	const auto distance = distanceFactor * std::max(height * 3.5f, k_MinDistance);
	return {.origin = focus + glm::vec3(0.0f, distance * 0.45f, -distance * 0.9f), .focus = focus};
}

CameraPlacement testbed_scenarios::Head(glm::vec3 position, glm::vec2 ahead, float height, float distanceFactor)
{
	const auto length = glm::length(ahead);
	const auto forward = length > 0.0f ? ahead / length : glm::vec2(0.0f, -1.0f);
	const glm::vec3 forward3 {forward.x, 0.0f, forward.y};
	// The head is high on the body and a little forward of its middle
	const glm::vec3 focus = position + glm::vec3(0.0f, height * 0.75f, 0.0f) + (forward3 * (height * 0.3f));
	const auto distance = distanceFactor * height;
	return {.origin = focus + (forward3 * distance) + glm::vec3(0.0f, height * 0.15f, 0.0f), .focus = focus};
}

Bounds testbed_scenarios::BoundsOf(std::span<const glm::vec2> points, glm::vec2 minimumHalfSize)
{
	if (points.empty())
	{
		return {.centre = glm::vec2(0.0f), .halfSize = minimumHalfSize};
	}
	auto low = points.front();
	auto high = points.front();
	for (const auto& point : points)
	{
		low = glm::min(low, point);
		high = glm::max(high, point);
	}
	return {.centre = (low + high) * 0.5f, .halfSize = glm::max((high - low) * 0.5f, minimumHalfSize)};
}

void testbed_scenarios::Apply(const NeedOverrides& overrides, creature_physiology::Needs& needs, float size)
{
	const auto set = [](float& value, const std::optional<float>& wanted, float low, float high) {
		if (wanted.has_value())
		{
			value = std::clamp(*wanted, low, high);
		}
	};
	set(needs.energy, overrides.energy, 0.0f, std::max(1.0f, size));
	set(needs.exhaustion, overrides.exhaustion, 0.0f, 1.0f);
	set(needs.dehydration, overrides.dehydration, 0.0f, 1.0f);
	set(needs.poo, overrides.poo, 0.0f, 1.0f);
	set(needs.life, overrides.life, 0.0f, 1.0f);
	set(needs.warmth, overrides.warmth, -1.0f, 1.0f);
	if (overrides.age.has_value())
	{
		needs.age = *overrides.age;
	}
}

void testbed_scenarios::Apply(std::span<const DesireOverride> overrides, creature_desires::Desires& desires)
{
	for (const auto& override : overrides)
	{
		auto& state = desires[override.desire];
		state.activated = true;
		state.value = std::clamp(override.fraction, 0.0f, 1.0f) * std::max(state.max, 0.0f);
	}
}

std::vector<size_t> testbed_scenarios::Advance(Timeline& timeline, std::span<const Command> commands,
                                               std::optional<size_t> repeatFrom, float seconds,
                                               const std::function<bool(size_t creature)>& isFree)
{
	std::vector<size_t> due;
	if (commands.empty() || timeline.next >= commands.size())
	{
		timeline.done = true;
	}
	if (timeline.done)
	{
		return due;
	}
	timeline.seconds += seconds;
	timeline.sinceGiven += seconds;
	// At most a round of the commands at once, so commands that go round again without waiting can't run away
	while (!timeline.done && due.size() < commands.size())
	{
		const auto& command = commands[timeline.next];
		if (command.waitUntilFree && !timeline.freed)
		{
			if (timeline.sinceGiven < k_SettleSeconds || !isFree(command.creature))
			{
				// Its delay counts from when the creature is free
				timeline.seconds = 0.0f;
				break;
			}
			timeline.freed = true;
		}
		if (timeline.seconds < command.delaySeconds)
		{
			break;
		}
		due.push_back(timeline.next);
		timeline.seconds = 0.0f;
		timeline.sinceGiven = 0.0f;
		timeline.freed = false;
		if (++timeline.next >= commands.size())
		{
			if (repeatFrom.has_value() && *repeatFrom < commands.size())
			{
				timeline.next = *repeatFrom;
			}
			else
			{
				timeline.done = true;
			}
		}
	}
	return due;
}

bool testbed_scenarios::KeepsDispenserGrid(const Scenario& scenario)
{
	if (!scenario.environment.dispenserGrid)
	{
		return false;
	}
	const auto onGrid = [](const auto& setup) { return testbed_dispensers::InGridArea(setup.offset); };
	return std::ranges::none_of(scenario.creatures, onGrid) && std::ranges::none_of(scenario.objects, onGrid);
}
