/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the player's hand finding its way about: hovering over the land, dragging it, turning and
// zooming the camera, and clicking. The mouse is driven through the same events the real one sends.

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Kind = Command::Kind;

constexpr size_t k_Left = 1;
constexpr size_t k_Middle = 2;
constexpr size_t k_Right = 3;

Command PointerTo(glm::vec2 point, float delay)
{
	return {.kind = Kind::PointerTo, .delaySeconds = delay, .point = point};
}

Command Press(size_t button, float delay)
{
	return {.kind = Kind::PointerPress, .delaySeconds = delay, .value = button};
}

Command Release(size_t button, float delay)
{
	return {.kind = Kind::PointerRelease, .delaySeconds = delay, .value = button};
}

/// The mouse moved by a share of the screen over some seconds
Command Sweep(glm::vec2 by, float seconds, float delay)
{
	return {.kind = Kind::PointerSweep, .delaySeconds = delay, .point = by, .amount = seconds};
}

Command Wheel(size_t notches, bool towards, float delay)
{
	return {.kind = Kind::WheelTurn, .delaySeconds = delay, .value = notches, .ctrl = towards};
}
} // namespace

void testbed_scenarios::AddHandNavigationScenarios(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "hand.rotate_release",
	    .name = "Turn the camera with the middle button and let go",
	    .facet = Facet::Hand,
	    .description = "The pointer rests low on the right of the screen, on bare land. The middle button is held while "
	                   "the mouse moves a third of the screen to the left over a second and a half, then let go; a "
	                   "second later the mouse moves a little up and right.",
	    .expected = "The camera turns about the land under the hand while the mouse moves. The hand and the cursor stay "
	                "where they were on the screen the whole time, the hand easing to the land that comes under it. "
	                "Letting go of the button leaves the hand where it is: it doesn't fly anywhere. The last small move "
	                "moves the hand on from there.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.75f, 0.85f}, 1.0f), Press(k_Middle, 1.0f), Sweep({-0.33f, 0.0f}, 1.5f, 0.2f),
	                 Release(k_Middle, 1.6f), Sweep({0.03f, -0.03f}, 0.3f, 1.0f)},
	});

	all.push_back({
	    .id = "hand.two_buttons",
	    .name = "Turn and zoom the camera with both buttons",
	    .facet = Facet::Hand,
	    .description = "The pointer rests low on the right, on bare land. Both buttons are held while the mouse moves down "
	                   "over a second and a half, then jumps right by a twentieth of the screen in a frame and moves on "
	                   "right slowly for a second; then they are let go.",
	    .expected = "Moving down zooms the camera and, once the mouse has moved a fortieth of the screen's width across in "
	                "a frame, moving across turns it. The hand doesn't grip the land: it keeps its idle hover and stays with "
	                "the cursor where it was on "
	                "the screen, and stays there once the buttons are let go.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.7f, 0.85f}, 1.0f), Press(k_Left, 1.0f), Press(k_Right, 0.0f),
	                 Sweep({0.0f, 0.15f}, 1.5f, 0.2f), Sweep({0.05f, 0.0f}, 0.001f, 1.6f), Sweep({0.1f, 0.0f}, 1.0f, 0.1f),
	                 Release(k_Right, 1.2f), Release(k_Left, 0.0f)},
	});

	all.push_back({
	    .id = "hand.drag",
	    .name = "Drag the land",
	    .facet = Facet::Hand,
	    .description = "The pointer rests low on the right of the screen. The left button is held while the mouse moves "
	                   "up and left over a second, then let go.",
	    .expected = "Pressing grips the land under the hand at once: the hand fades onto the land over 0.13 seconds "
	                "as it changes to its grip. The land then follows the hand, which stays on the spot it "
	                "gripped. Letting go, the hand fades back to hovering at the cursor as quickly, carrying on from how "
	                "far the gripped land was from the camera.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.85f, 0.85f}, 1.0f), Press(k_Left, 1.0f), Sweep({-0.15f, -0.15f}, 1.0f, 0.2f),
	                 Release(k_Left, 1.2f)},
	});

	all.push_back({
	    .id = "hand.edge_hover",
	    .name = "Hover at the edges of the screen",
	    .facet = Facet::Hand,
	    .description = "The pointer rests in the middle, then at the right edge, the bottom edge, the very bottom, and "
	                   "the top of the screen, a second at each.",
	    .expected = "In the middle the hand hovers in its ordinary pose. Near the sides (beyond 45% of the half width), "
	                "near the bottom (below 43%) and at the top (above 49%, or 40% over no land) it shows the turning "
	                "pose, standing up towards where the camera looks; the very bottom (below 49%) offers tilting too, "
	                "but turning shows.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.5f, 0.6f}, 1.0f), PointerTo({0.98f, 0.6f}, 1.0f), PointerTo({0.5f, 0.95f}, 1.0f),
	                 PointerTo({0.5f, 0.995f}, 1.0f), PointerTo({0.5f, 0.004f}, 1.0f), PointerTo({0.5f, 0.6f}, 1.0f)},
	});

	all.push_back({
	    .id = "hand.edge_rotate",
	    .name = "Drag round the edge to turn the camera",
	    .facet = Facet::Hand,
	    .description = "The left button is pressed at the right edge of the screen, and the mouse moves down slowly for a "
	                   "second and a half, then is let go.",
	    .expected = "Once the mouse has moved a fiftieth of the screen the drag turns the camera: the cursor and the hand "
	                "are held on a ring nine tenths of the way out from the middle, and the camera turns by the angle "
	                "they sweep round the middle. The hand shows the turning pose, a third of its height higher.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.98f, 0.55f}, 1.0f), Press(k_Left, 1.0f), Sweep({0.0f, 0.3f}, 1.5f, 0.2f),
	                 Release(k_Left, 1.6f)},
	});

	all.push_back({
	    .id = "hand.edge_pan",
	    .name = "A quick drag in from the edge pans",
	    .facet = Facet::Hand,
	    .description = "The left button is pressed at the right edge of the screen, and the mouse moves quickly in towards "
	                   "the middle, then is let go.",
	    .expected = "Pressed at the edge the hand offers turning, but moving quickly (within 0.3 s) towards the middle "
	                "the drag pans: the hand grips the land and the land follows it.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.98f, 0.6f}, 1.0f), Press(k_Left, 1.0f), Sweep({-0.2f, 0.0f}, 0.2f, 0.0f),
	                 Sweep({-0.2f, 0.0f}, 0.8f, 0.25f), Release(k_Left, 1.0f)},
	});

	all.push_back({
	    .id = "hand.top_pitch",
	    .name = "Drag up and down at the top to tilt",
	    .facet = Facet::Hand,
	    .description = "The left button is pressed at the top of the screen and the mouse moves down slowly, then up, "
	                   "then is let go.",
	    .expected = "The drag tilts the camera, by seven thirds of the field of view across for a screen's height of "
	                "movement, down and then back up; the hand shows the tilting pose and keeps to the cursor.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.5f, 0.004f}, 1.0f), Press(k_Left, 1.0f), Sweep({0.0f, 0.15f}, 1.0f, 0.2f),
	                 Sweep({0.0f, -0.1f}, 1.0f, 1.1f), Release(k_Left, 1.1f)},
	});

	all.push_back({
	    .id = "hand.fast_pan",
	    .name = "Drag the land quickly",
	    .facet = Facet::Hand,
	    .description = "The left button is pressed low in the middle of the screen, the mouse moves a quarter of the "
	                   "screen up and left in a fifth of a second, rests a second, and the button is let go.",
	    .expected = "The land gripped follows the cursor: the camera eases after it over about 0.3 seconds, as the game's "
	                "camera does, so the hand trails the cursor while it moves and settles under it once it stops. "
	                "Letting go, nothing jumps: the hand is already under the cursor.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.5f, 0.8f}, 1.0f), Press(k_Left, 1.0f), Sweep({-0.25f, -0.25f}, 0.2f, 0.2f),
	                 Release(k_Left, 1.2f)},
	});

	all.push_back({
	    .id = "hand.zoom",
	    .name = "Zoom with the wheel",
	    .facet = Facet::Hand,
	    .description = "The pointer rests low on the right of the screen. The wheel turns three notches away, then three back.",
	    .expected = "The camera zooms in towards the land and out again. The hand stays at the cursor on the screen, "
	                "easing to the land as it comes nearer and goes away.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.8f, 0.85f}, 1.0f), Wheel(3, false, 1.0f), Wheel(3, true, 2.0f)},
	});

	all.push_back({
	    .id = "hand.click",
	    .name = "Click the land",
	    .facet = Facet::Hand,
	    .description = "The pointer moves across the screen, then the left button is clicked.",
	    .expected = "The hand follows the cursor, easing in and out to its height above the land. The click grips the "
	                "land for a moment, the hand settling onto it, and lets it go without moving the camera.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.6f, 0.75f}, 1.0f), Sweep({0.3f, 0.1f}, 1.0f, 0.5f), Press(k_Left, 1.5f),
	                 Release(k_Left, 0.1f)},
	});
}
