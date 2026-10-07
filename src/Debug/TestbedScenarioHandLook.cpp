/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of how the hand looks for its player's alignment: evil, neutral and good, and turning from
// one to the other

#include <vector>

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Kind = Command::Kind;

/// The hand hovers over the land in the middle of the window
constexpr glm::vec2 k_Cursor {0.5f, 0.5f};

Environment Looking(float alignment)
{
	return {.dispenserGrid = false, .playerAlignment = alignment, .cursor = k_Cursor};
}

/// Looking down at the middle of the map from close by, where the hand hovers
Framing Close()
{
	return {.shot = Shot::Overview, .include = {{0.0f, 0.0f}}, .distance = 0.2f};
}

Command Align(float alignment, float delay)
{
	return {.kind = Kind::SetAlignment, .delaySeconds = delay, .alignment = alignment};
}
} // namespace

void testbed_scenarios::AddHandLookScenarios(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "hand.look_evil",
	    .name = "The hand of an evil player",
	    .facet = Facet::Hand,
	    .description = "The player's alignment is set to -1, fully evil, with the hand over the land in the middle of the "
	                   "window.",
	    .expected = "The hand is dark red and veined, its fingers and knuckles ridged and spiked: wholly the evil hand's shape "
	                "and skin.",
	    .environment = Looking(-1.0f),
	    .framing = Close(),
	});

	all.push_back({
	    .id = "hand.look_neutral",
	    .name = "The hand of a neutral player",
	    .facet = Facet::Hand,
	    .description = "The player's alignment is set to 0, with the hand over the land in the middle of the window.",
	    .expected = "The hand is the plain orange-brown hand with smooth fingers: the base hand untouched.",
	    .environment = Looking(0.0f),
	    .framing = Close(),
	});

	all.push_back({
	    .id = "hand.look_good",
	    .name = "The hand of a good player",
	    .facet = Facet::Hand,
	    .description = "The player's alignment is set to 1, fully good, with the hand over the land in the middle of the "
	                   "window.",
	    .expected = "The hand is pale gold, the same smooth shape as the neutral hand: the good hand only changes its skin.",
	    .environment = Looking(1.0f),
	    .framing = Close(),
	});

	// From neutral, jumps to either end and back, then a slow sweep from evil to good and back again
	std::vector<Command> turning {Align(-1.0f, 3.0f), Align(0.0f, 3.0f),  Align(1.0f, 3.0f),
	                              Align(0.0f, 3.0f),  Align(-0.5f, 3.0f), Align(0.5f, 3.0f)};
	constexpr int k_SweepSteps = 200;
	constexpr float k_SweepDelay = 0.05f;
	for (int i = 0; i <= k_SweepSteps; ++i)
	{
		turning.push_back(Align(-1.0f + (2.0f * static_cast<float>(i) / k_SweepSteps), i == 0 ? 3.0f : k_SweepDelay));
	}
	for (int i = k_SweepSteps - 1; i >= 0; --i)
	{
		turning.push_back(Align(-1.0f + (2.0f * static_cast<float>(i) / k_SweepSteps), k_SweepDelay));
	}
	turning.push_back(Align(0.0f, 3.0f));
	all.push_back({
	    .id = "hand.look_turning",
	    .name = "The hand turning evil and good",
	    .facet = Facet::Hand,
	    .description = "Every three seconds the player's alignment jumps: to -1, 0, 1, 0, -0.5 and 0.5. Then it sweeps "
	                   "from -1 to 1 and back by a hundredth every twentieth of a second, and returns to 0.",
	    .expected = "Each jump changes the hand at once, with no fading: red and ridged at -1, plain at 0, pale gold at 1, "
	                "half way at -0.5 and 0.5. In the sweep the hand changes in steps of three or four hundredths, its "
	                "ridges smoothing away as it nears neutral and its skin turning from red through orange-brown to gold.",
	    .environment = Looking(0.0f),
	    .framing = Close(),
	    .commands = turning,
	    .repeatFrom = 0,
	});
}
