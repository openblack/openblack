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
	    .description = "The pointer rests low on the right, on bare land. Both buttons are held while the mouse moves right "
	                   "and down over a second and a half; then they are let go.",
	    .expected = "Moving down zooms the camera and, once the mouse has moved sideways far enough, moving across turns "
	                "it. The hand doesn't grip the land: it keeps its idle hover and stays with the cursor where it was on "
	                "the screen, and stays there once the buttons are let go.",
	    .framing = {.shot = Shot::Testbed},
	    .commands = {PointerTo({0.7f, 0.85f}, 1.0f), Press(k_Left, 1.0f), Press(k_Right, 0.0f),
	                 Sweep({0.2f, 0.15f}, 1.5f, 0.2f), Release(k_Right, 1.6f), Release(k_Left, 0.0f)},
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
