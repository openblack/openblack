/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the gestures drawn with the hand: each gesture is drawn as a path through the same
// recogniser the cursor goes through, so the debug window's gesture overlay shows it being drawn and matched

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Kind = Command::Kind;

/// A gesture takes a second or two to draw; the next command waits for it and the hand's rest after it
constexpr float k_DrawSeconds = 3.0f;

Command HoldSeed(SpellSeedType seed, float delaySeconds)
{
	return {.kind = Kind::HoldSeed, .delaySeconds = delaySeconds, .value = static_cast<size_t>(seed)};
}

/// A command to the hand that waits until the hand has finished drawing, its delay counting from then
Command AfterDrawing(Command command)
{
	command.waitUntilFree = true;
	return command;
}

Command Draw(GestureType gesture, float delaySeconds = k_DrawSeconds)
{
	return {.kind = Kind::DrawGesture, .delaySeconds = delaySeconds, .value = static_cast<size_t>(gesture)};
}

CreatureSetup Tiger()
{
	return {.label = "your tiger",
	        .species = CreatureType::Tiger,
	        .offset = {0.0f, 50.0f},
	        .facingDegrees = 0.0f,
	        .needs = {.energy = 1.0f, .exhaustion = 0.0f, .dehydration = 0.0f, .poo = 0.0f, .life = 1.0f},
	        .hold = true};
}
} // namespace

void testbed_scenarios::AddGestureScenarios(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.gesture_circle_shield",
	    .name = "Size a shield with a circle",
	    .facet = Facet::Miracles,
	    .description = "A shield seed is put in the hand and a circle is drawn round the middle of the screen with the "
	                   "Action button held, which readies the shield. Open the "
	                   "Gestures debug window to see the path and the template it matches. Hold a shield or storm seed "
	                   "and draw a circle with the right button held to try it.",
	    .expected = "The circle is recognised: the log gives its middle on the land and its radius across the land, "
	                "and the miracles take it for the shield, which is cast there at that size on letting go. It is "
	                "remembered for five seconds. The hand draws the circle: while it holds the shield seed a faint "
	                "ribbon in the player's colour trails behind it, each part fading and narrowing away over two "
	                "seconds. On recognition the gesture's sound plays and its trail appears on the land: 234 small "
	                "glows in the player's colour grow from both ends of the drawn path over 0.6 seconds while flowing "
	                "onto a true circle laid on the land, raised a little towards the camera, dim and wiggling; a sheet "
	                "of stars in the player's colour rises along the circle in a wave, tallest two seconds in and gone "
	                "by four; at 2.4 seconds the glows flash to full brightness and the hand glows the player's colour "
	                "for half a second, the flash dying away by 4.5 seconds; from three seconds the wiggle shrinks away "
	                "over two seconds and the trail goes at seven.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Testbed},
	    .commands = {HoldSeed(SpellSeedType::Shield, 1.0f), Draw(GestureType::Circle, 1.0f)},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "miracles.gesture_power_up_storm",
	    .name = "Power up a storm",
	    .facet = Facet::Miracles,
	    .description = "A storm seed is summoned from the player's worship into the hand (only such a seed can be powered "
	                   "up) and its first power-up gesture, the inverse spiral, is drawn, then its second, the spiral.",
	    .expected =
	        "Each spiral is recognised as a power-up, level 0 then level 1, for the miracles to raise the seed "
	        "to (the band and the announcer's voice). A circle drawn with the Action button held would size the storm instead.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Testbed},
	    .commands = {{.kind = Kind::SummonSeed,
	                  .delaySeconds = k_DrawSeconds,
	                  .value = static_cast<size_t>(SpellSeedType::Storm)},
	                 Draw(GestureType::InverseSpiral, 1.0f),
	                 Draw(GestureType::Spiral)},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "miracles.gesture_scribble_cancel",
	    .name = "Cancel a held miracle with a scribble",
	    .facet = Facet::Miracles,
	    .description = "A fireball seed is put in the hand and a scribble is drawn: quick strokes from side to side.",
	    .expected = "The scribble is recognised; the miracles drop the seed back where it came from, refunding its "
	                "prayer power (or call off a power-up asked for first).",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Testbed},
	    // The next seed waits until the scribble has been drawn to its end
	    .commands = {AfterDrawing(HoldSeed(SpellSeedType::Fire, 1.0f)), Draw(GestureType::Scribble, 1.0f)},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "leash.gesture_picker",
	    .name = "The leash gesture and the leash picker",
	    .facet = Facet::Leash,
	    .description = "Your tiger is taught all three leashes, its leash is taken off, and the leash gesture (a square "
	                   "spiral) is drawn. With the picker open the learning leash's gesture (an E) is drawn; then the "
	                   "leash gesture again and the aggression leash's (a vertical scribble); then a scribble with the "
	                   "empty hand.",
	    .expected = "The square spiral puts the picked leash on and opens the picker, as the tiger knows more than one "
	                "leash. The E changes it to the learning rope and closes the picker. The square spiral opens the "
	                "picker again, the vertical scribble changes to the aggression leash, and the scribble shakes it "
	                "off. The picker closes by itself after 25 seconds.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Tiger()},
	    .commands = {{.kind = Kind::PutOnLeash, .creature = 0, .delaySeconds = 1.0f, .leash = LeashType::Evil},
	                 {.kind = Kind::PutOnLeash, .creature = 0, .delaySeconds = 1.0f, .leash = LeashType::Good},
	                 {.kind = Kind::TakeOffLeash, .creature = 0, .delaySeconds = 1.0f},
	                 Draw(GestureType::SquareSpirial, 1.0f),
	                 Draw(GestureType::EShape),
	                 Draw(GestureType::SquareSpirial),
	                 Draw(GestureType::VerticalScribble),
	                 Draw(GestureType::Scribble)},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "leash.gesture_scribbles",
	    .name = "Scribbles with the leash picker open",
	    .facet = Facet::Leash,
	    .description = "Your tiger, which knows every leash, has its rope taken off. The leash gesture (a square spiral) "
	                   "is drawn, then a scribble with the empty hand, then another.",
	    .expected = "The square spiral puts the rope on and opens the picker. The first scribble only closes the picker, "
	                "as in the game, and the rope stays on; the second shakes it off.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Tiger()},
	    .commands = {{.kind = Kind::PutOnLeash, .creature = 0, .delaySeconds = 1.0f, .leash = LeashType::Rope},
	                 {.kind = Kind::TakeOffLeash, .creature = 0, .delaySeconds = 1.0f},
	                 Draw(GestureType::SquareSpirial, 1.0f),
	                 Draw(GestureType::Scribble),
	                 Draw(GestureType::Scribble)},
	    .repeatFrom = 0,
	});
}
