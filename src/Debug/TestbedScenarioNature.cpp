/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the land's nature: a tree pulled up by the hand leaving its roots behind, and the fireflies
// coming out of the trees and rocks at nightfall

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Kind = Command::Kind;

constexpr size_t k_Action = 3;

/// The tree pulled up stands here, from the middle of the map
constexpr glm::vec2 k_Tree {0.0f, 30.0f};
} // namespace

void testbed_scenarios::AddNatureScenarios(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "nature.roots_hole",
	    .name = "A tree pulled up leaves its roots",
	    .facet = Facet::Nature,
	    .description =
	        "An oak stands in the middle of the screen. The pointer is put on its trunk, the action (right) button held, "
	        "and the mouse pulled up a third of the screen over a second and kept there until the tree comes "
	        "free into the hand, which carries it away up the screen.",
	    .expected = "The oak leans and stretches towards the hand, then comes out of the ground. Where it stood lies a "
	                "heap of roots as wide as a third of the tree's width and depth together, turned as the tree was, over "
	                "the land's shape, with one white puff of smoke. The heap lies 15 seconds of game time and fades out "
	                "over its last second; nothing else is left, and a tree let go gently on the spot takes root again.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Placed, .eye = {0.0f, 9.0f, 12.0f}, .look = {k_Tree.x, 3.0f, k_Tree.y}},
	    .objects = {{.type = TreeInfo::Oak, .offset = k_Tree}},
	    .commands = {{.kind = Kind::PointerTo, .delaySeconds = 1.0f, .point = {0.5f, 0.5f}},
	                 {.kind = Kind::PointerPress, .delaySeconds = 0.5f, .value = k_Action},
	                 {.kind = Kind::PointerSweep, .delaySeconds = 0.3f, .point = {0.0f, -0.35f}, .amount = 1.0f}},
	});
}
