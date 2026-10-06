/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of Creature Mode, the camera locked onto a creature with its status panel, and of the
// Creature Cave

#include "Creature/CreatureCave.h"
#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Kind = Command::Kind;

/// The cursor keys, by the camera keys command's value
constexpr size_t k_Left = 0;
constexpr size_t k_Up = 2;
constexpr size_t k_Down = 3;

CreatureSetup Player(glm::vec2 offset, std::string_view label)
{
	return {.label = label,
	        .species = CreatureType::Tiger,
	        .offset = offset,
	        .owner = PlayerNames::PLAYER_ONE,
	        .needs = {.energy = 0.8f, .exhaustion = 0.3f, .dehydration = 0.0f, .poo = 0.0f, .life = 0.9f}};
}

Command Walk(glm::vec2 point, float delay)
{
	return {.kind = Kind::WalkTo, .creature = 0, .delaySeconds = delay, .point = point};
}

Command Key(Kind kind, float delay, size_t creature = 0)
{
	return {.kind = kind, .creature = creature, .delaySeconds = delay};
}

Command CameraKeys(size_t direction, float seconds, bool ctrl, float delay)
{
	return {
	    .kind = Kind::CameraKeys, .creature = 0, .delaySeconds = delay, .value = direction, .amount = seconds, .ctrl = ctrl};
}

void AddCreatureMode(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "creature_mode.enter",
	    .name = "Creature Mode with C",
	    .facet = Facet::CreatureMode,
	    .description = "The player's tiger walks round a square; C is pressed after a second, and again after twenty.",
	    .expected = "The camera flies to the tiger over two seconds, keeping the way it was turned, about eight of the "
	                "tiger's heights away and looking at its middle, then follows it round the square, easing after it "
	                "a second behind. Near the top left of the screen the panel shows its damage (10%), hunger (20%) and "
	                "tiredness (30%), without the reward. The second C gives the camera back where it is.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Player({0.0f, 40.0f}, "yours")},
	    .commands = {Key(Kind::CreatureKey, 1.0f), Walk({40.0f, 40.0f}, 1.0f), Walk({40.0f, 100.0f}, 5.0f),
	                 Walk({0.0f, 100.0f}, 5.0f), Walk({0.0f, 40.0f}, 5.0f), Key(Kind::CreatureKey, 5.0f)},
	});

	all.push_back({
	    .id = "creature_mode.other_god",
	    .name = "Double click another god's creature",
	    .facet = Facet::CreatureMode,
	    .description = "The player's tiger and another god's lion. The lion is double clicked; later C is pressed.",
	    .expected = "The double click locks the camera onto the other god's lion, any god's creature can be followed, "
	                "and the panel shows the lion's status. C then moves the camera over to the player's own tiger, as "
	                "C only ever locks onto your creature; C again gives the camera back.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Player({-30.0f, 50.0f}, "yours"),
	                  CreatureSetup {.label = "another god's",
	                                 .species = CreatureType::Lion,
	                                 .offset = {30.0f, 50.0f},
	                                 .owner = PlayerNames::PLAYER_TWO,
	                                 .needs = {.energy = 0.5f, .exhaustion = 0.5f, .life = 0.5f}}},
	    .commands = {Key(Kind::DoubleClick, 1.0f, 1), Key(Kind::CreatureKey, 8.0f), Key(Kind::CreatureKey, 8.0f)},
	});

	all.push_back({
	    .id = "creature_mode.orbit",
	    .name = "Orbit with Shift and the cursor keys",
	    .facet = Facet::CreatureMode,
	    .description = "Locked onto the player's tiger, Shift and left are held for four seconds, Shift and up for one, "
	                   "Shift and down for one, then Ctrl and up for one and Ctrl and down for two.",
	    .expected = "Shift and left swing the camera round the tiger, half a turn in about four seconds on a screen 2560 "
	                "wide (400 pixels a second, a screen's width a half turn); Shift and up tilt it to look down more "
	                "steeply, and down back again, no lower than about 14 degrees; Ctrl and up draw it in, Ctrl and down "
	                "out. The cursor keys alone would give the camera back.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Player({0.0f, 40.0f}, "yours")},
	    .commands = {Key(Kind::CreatureKey, 1.0f), CameraKeys(k_Left, 4.0f, false, 3.0f), CameraKeys(k_Up, 1.0f, false, 5.0f),
	                 CameraKeys(k_Down, 1.0f, false, 2.0f), CameraKeys(k_Up, 1.0f, true, 2.0f),
	                 CameraKeys(k_Down, 2.0f, true, 2.0f)},
	});

	// A wall of trees between the camera and the tiger
	std::vector<ObjectSetup> trees;
	for (int i = -3; i <= 3; ++i)
	{
		trees.push_back({.type = TreeInfo::Conifer, .offset = {static_cast<float>(i) * 8.0f, 25.0f}, .scale = 1.5f});
	}
	all.push_back({
	    .id = "creature_mode.obstruction",
	    .name = "Ctrl and Shift clear the view",
	    .facet = Facet::CreatureMode,
	    .description = "Locked onto the player's tiger behind a wall of conifers, Ctrl and Shift are pressed together.",
	    .expected = "The camera swings round to the heading, of 32 round the tiger, along which the land falls away "
	                "furthest below its middle, favouring the heading it has, and tilts to a little over 22 degrees plus "
	                "a fifth of its pitch, between 22.5 and 60. As in the game it reads only the land's heights: on the "
	                "testbed's level plane it keeps its heading and only tilts, and the trees aren't looked at.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Player({0.0f, 40.0f}, "yours")},
	    .objects = trees,
	    .commands = {Key(Kind::CreatureKey, 1.0f), Key(Kind::ClearCameraView, 4.0f)},
	});

	all.push_back({
	    .id = "creature_mode.pass_out",
	    .name = "Passes out at 100% and comes round in its pen",
	    .facet = Facet::CreatureMode,
	    .description = "The player's tiger, its tiredness at 100%, away from the middle of the testbed with the camera "
	                   "locked onto it.",
	    .expected = "With the panel's tiredness at 100% the tiger passes out where it stands: it lies out cold for "
	                "four times its size and eight seconds, fizzes out of sight over two seconds and back in over two at "
	                "its pen, which on the testbed, with no temple, is the middle of the land; the camera follows it "
	                "there. It lies a few seconds, rests until its life is 40% and its tiredness 30%, and gets up.",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {CreatureSetup {.label = "yours",
	                                 .species = CreatureType::Tiger,
	                                 .offset = {-60.0f, 60.0f},
	                                 .facingDegrees = 270.0f,
	                                 .owner = PlayerNames::PLAYER_ONE,
	                                 .needs = {.energy = 1.0f, .exhaustion = 1.0f, .dehydration = 0.0f, .poo = 0.0f}}},
	    .commands = {Key(Kind::CreatureKey, 0.5f)},
	});
}

void AddCave(std::vector<Scenario>& all)
{
	auto tattoo = [](Kind kind, size_t design, size_t site, float delay) {
		return Command {.kind = kind, .creature = 0, .delaySeconds = delay, .value = design, .bodyPart = site};
	};
	all.push_back({
	    .id = "creature_mode.cave_tattoo",
	    .name = "The Creature Cave and its tattoos",
	    .facet = Facet::CreatureMode,
	    .description = "F5 is pressed for the player's tiger, on the tattoo page; a symbol is tattooed on its head, "
	                   "another on its chest, and the head's taken off again.",
	    .expected = "The temple opens on the creature's room, with its four scrolls telling of the tiger: its attributes, "
	                "the actions it has learnt, its mind and its miracles. The cave's screen stands at the right with "
	                "the same pages, what it has learnt to do and not to do, and its tattoos: the symbols appear on the "
	                "tiger's head and chest, and the head's goes again (seen on the tiger once out of the temple).",
	    .framing = {.shot = Shot::Overview},
	    .creatures = {Player({0.0f, 40.0f}, "yours")},
	    .commands = {{.kind = Kind::OpenCreatureCave,
	                  .creature = 0,
	                  .delaySeconds = 1.0f,
	                  .value = static_cast<size_t>(creature_cave::Page::Tattoos)},
	                 tattoo(Kind::ApplyTattoo, 4, 1, 3.0f),
	                 tattoo(Kind::ApplyTattoo, 9, 2, 2.0f),
	                 tattoo(Kind::RemoveTattoo, 0, 1, 4.0f)},
	});
}
} // namespace

void testbed_scenarios::AddCreatureModeScenarios(std::vector<Scenario>& all)
{
	AddCreatureMode(all);
	AddCave(all);
}
