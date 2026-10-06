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

#include "Creature/CreatureLayers.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

// How to add a scenario: append one to the list Build() returns, in the section of its facet. Give it a new id
// ("facet.what", never reused or renamed, as tests and the command line pick scenarios by it), a name, its facet, a
// description of what it sets up and what to look for, and then its data: the environment (land, hour, weather, body
// time), the framing, the creatures with the bodies, needs and desires they start with, the objects on the land, and
// the commands, which play in turn. Offsets are from the middle of the map, x east and y north; the testbed's camera
// looks north from 120 units south of the middle, and the pool lies between them (x -160 to 160, y -120 to -20). A
// new facet goes into the Facet enum and its Name. test_testbed_scenarios checks the data of every scenario; anything a
// scenario needs that the runner can't yet do goes into these types and TestbedScenarioRunner together.

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
	    .expected = "Smile, grimace, growl, scared, sad, amazed, puzzled, laugh, ooh and aah, each easing out of the last, "
	                "then a nod, a shake of the head, a yawn, thirst, squirting water and talking.",
	    .framing = {.shot = Shot::Head, .distance = 1.6f},
	    .creatures = {CreatureSetup {.species = CreatureType::Tiger, .needs = Content(), .hold = true, .pauseMind = true}},
	    .commands = faces,
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
	    .description = "On the testbed with the pool, a parched tiger starts north of the middle with its desire for "
	                   "water as strong as it gets.",
	    .expected = "It walks to the nearest edge of the pool, bends down and drinks; its thirst clears and the desire "
	                "for water is held back for a while.",
	    .environment = {.land = Land::Pool},
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .offset = {0.0f, 90.0f},
	        .needs = {.energy = 1.0f, .exhaustion = 0.0f, .dehydration = 0.95f, .poo = 0.0f},
	        .desires = {{.desire = Desire::Water, .fraction = 1.0f}},
	    }},
	});

	all.push_back({
	    .id = "needs.hunger",
	    .name = "Hungry creature finds food",
	    .facet = Facet::Needs,
	    .description = "A hungry tiger with two lots of magic food on the land in front of it.",
	    .expected = "It walks up to the nearer food and eats it, then, if still hungry, the other; its energy fills up "
	                "and the hunger desire falls.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {CreatureSetup {
	        .species = CreatureType::Tiger,
	        .needs = {.energy = 0.2f, .dehydration = 0.0f, .poo = 0.0f},
	        .desires = {{.desire = Desire::Hunger, .fraction = 0.9f}},
	    }},
	    .objects = {{.type = MobileObjectInfo::MagicFood, .offset = {0.0f, -50.0f}},
	                {.type = MobileObjectInfo::MagicFood, .offset = {40.0f, -90.0f}}},
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
	    .expected = "Running tires it; past 0.8 exhaustion it slows down, and at 1 it faints, lies out cold and gets up "
	                "again before running on.",
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
	    .name = "Reflections in the pool",
	    .facet = Facet::Light,
	    .description = "On the testbed with the pool, a tiger, a horse and a giant ape stand on its far edge, seen "
	                   "across the water from the testbed's camera.",
	    .expected = "Each shows upside down in the water in front of it, moving as it moves.",
	    .environment = {.land = Land::Pool},
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Posed(CreatureType::Tiger, {40.0f, -5.0f}), Posed(CreatureType::Horse, {0.0f, -5.0f}),
	                  Posed(CreatureType::GiantApe, {-45.0f, -5.0f})},
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
	    .description = "On the testbed with the pool, a tiger walks from the grass into the edge of the pool and back. "
	                   "Select it in the spawner to see its sound log.",
	    .expected = "Its footsteps sound of grass on the land and of splashing in the water; the log's keys show the "
	                "surface changing.",
	    .environment = {.land = Land::Pool},
	    .framing = {.shot = Shot::Follow, .distance = 1.4f},
	    .creatures = {Posed(CreatureType::Tiger, {0.0f, 40.0f})},
	    .commands = {Go(Kind::WalkTo, 0, {0.0f, -25.0f}, 1.0f), Go(Kind::WalkTo, 0, {0.0f, 40.0f}, 1.0f)},
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
} // namespace

std::string_view testbed_scenarios::Name(Facet facet)
{
	constexpr std::array<std::string_view, k_FacetCount> k_Names {
	    "Idle", "Expressions", "Senses", "Needs", "Growth", "Appearance", "Light", "Movement", "Footprints", "Audio",
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
	constexpr std::array<std::string_view, 22> k_Names {
	    "walk to", "run to",      "follow",  "flee from", "turn to face", "face the camera",
	    "stop",    "play action", "gesture", "pull face", "sit down",     "stand up",
	    "sleep",   "wake",        "eat",     "drink",     "poo",          "puke",
	    "faint",   "stroke",      "slap",    "set hour",
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
	if (scenario.creatures.empty())
	{
		problems.emplace_back("no creatures");
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
		if (creature.phase && *creature.phase > 13)
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
		if (command.kind == Kind::Follow && (command.value >= creatures || command.value == command.creature))
		{
			problems.push_back(fmt::format("{}: follows no other creature", what));
		}
		if (!ValidAnimation(command.kind, command.value))
		{
			problems.push_back(fmt::format("{}: animation {} isn't of its kind", what, command.value));
		}
		if (!ValidOffset(command.point) || command.delaySeconds < 0.0f || !InRange(command.hour, 0.0f, k_HoursPerDay))
		{
			problems.push_back(fmt::format("{}: point, delay or hour out of range", what));
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
