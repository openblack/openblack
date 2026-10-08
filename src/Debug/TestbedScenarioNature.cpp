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

#include <array>
#include <string_view>
#include <utility>
#include <vector>

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Kind = Command::Kind;

/// The buttons: the left taps a one-shot globe into the hand; the right, the action, pulls up trees and casts the
/// miracle held
constexpr size_t k_Tap = 1;
constexpr size_t k_Action = 3;

/// The tree pulled up stands here, from the middle of the map
constexpr glm::vec2 k_Tree {0.0f, 30.0f};
/// The village the fireflies come to
constexpr glm::vec2 k_Village {0.0f, 60.0f};
/// The evening, once the sky has darkened past dusk, and the morning, once it has brightened past dawn
constexpr float k_Evening = 18.0f;
constexpr float k_Morning = 10.0f;

/// Three huts with trees round them, near and far
std::vector<ObjectSetup> Village()
{
	std::vector<ObjectSetup> things {
	    {.type = AbodeInfo::NorseHut, .offset = k_Village + glm::vec2(-12.0f, 0.0f), .yawDegrees = 160.0f},
	    {.type = AbodeInfo::NorseHut, .offset = k_Village + glm::vec2(10.0f, 6.0f), .yawDegrees = 200.0f},
	    {.type = AbodeInfo::NorseHut, .offset = k_Village + glm::vec2(2.0f, -14.0f), .yawDegrees = 20.0f},
	};
	constexpr std::array k_Kinds {TreeInfo::Oak, TreeInfo::Beech, TreeInfo::Birch, TreeInfo::Pine};
	constexpr std::array<glm::vec2, 12> k_Trees {{{-20.0f, 6.0f},
	                                              {-18.0f, -8.0f},
	                                              {18.0f, 12.0f},
	                                              {16.0f, -2.0f},
	                                              {-4.0f, -22.0f},
	                                              {8.0f, -20.0f},
	                                              {-30.0f, 25.0f},
	                                              {32.0f, 30.0f},
	                                              {0.0f, 35.0f},
	                                              {-40.0f, -30.0f},
	                                              {45.0f, -25.0f},
	                                              {0.0f, 12.0f}}};
	for (size_t i = 0; i < k_Trees.size(); ++i)
	{
		things.push_back({.type = k_Kinds.at(i % k_Kinds.size()), .offset = k_Village + k_Trees.at(i)});
	}
	return things;
}

/// What a caught firefly gives on Land 1, as its script sets it
std::vector<std::pair<std::string_view, float>> Land1Rewards()
{
	return {{"HEAL", 20.0f}, {"FIRE", 1.0f}, {"LIGHTNING_BOLT", 1.0f}, {"NATURE", 1.0f},
	        {"FOOD", 1.0f},  {"WOOD", 1.0f}, {"WATER", 1.0f}};
}
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

	all.push_back({
	    .id = "nature.fireflies_nightfall",
	    .name = "Fireflies come out at nightfall",
	    .facet = Facet::Nature,
	    .description = "Three huts with a dozen trees round them, the clock held in the evening once the sky has darkened "
	                   "past dusk.",
	    .expected = "At the first turn the land is topped up: 50 coin tosses, each tails hiding a firefly in a random tree "
	                "and heads looking for a rock, of which there are none, so about 25 are made. Then one a turn comes out of "
	                "the tree it hides in, unless another shares its tree, "
	                "in which case it is gone: so one firefly a tree comes out. Each flies, easing in and out, to roughly "
	                "the nearest hut and hovers 2 m above its roof, drifting on a slow loop 8 m across and a quick one "
	                "1 m across, its drift growing over the first fifth of its flight. Each is a small white glow 0.6 m "
	                "across, at three quarters brightness within 100 m and fading out by 300 m.",
	    .environment = {.hour = k_Evening, .clockRuns = false, .dispenserGrid = false},
	    .framing = {.shot = Shot::Placed, .eye = {0.0f, 14.0f, 25.0f}, .look = {k_Village.x, 4.0f, k_Village.y}},
	    .objects = Village(),
	    .fireflyRewards = Land1Rewards(),
	});

	all.push_back({
	    .id = "nature.fireflies_dawn",
	    .name = "Fireflies go home at dawn",
	    .facet = Facet::Nature,
	    .description = "The village of the nightfall scenario: the fireflies come out in the evening, and after 20 "
	                   "seconds the clock jumps to the morning, once the sky has brightened past dawn.",
	    .expected = "In the morning one firefly a turn leaves its hut for the tree or rock roughly nearest it, not "
	                "necessarily the one it came from, its drift dying away over the last fifth of its flight; once there "
	                "it rests, hidden and not drawn. By day none is seen.",
	    .environment = {.hour = k_Evening, .clockRuns = false, .dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {k_Village - glm::vec2(45.0f), k_Village + glm::vec2(45.0f)}},
	    .objects = Village(),
	    .commands = {{.kind = Kind::SetHour, .delaySeconds = 20.0f, .hour = k_Morning}},
	    .fireflyRewards = Land1Rewards(),
	});

	all.push_back({
	    .id = "nature.fireflies_catch",
	    .name = "A firefly caught in its tree",
	    .facet = Facet::Nature,
	    .description = "A hut with one oak beside it, and the heal miracle as the only reward. Evening for 8 seconds, so "
	                   "the fireflies come out to the hut, then morning, so they hide again in the oak; at 16 seconds the "
	                   "hand pulls the oak out of the ground.",
	    .expected = "The evening makes about half of 50 fireflies in the only tree (the coin's other side looks for a "
	                "rock, and there is none); all but one are gone, a turn each, sharing their spot; that one hovers over the "
	                "hut and in the morning goes back into the oak. Pulled up, the oak "
	                "leaves its heap of roots and, where it stood, a one-shot heal globe: the firefly is caught.",
	    .environment = {.hour = k_Evening, .clockRuns = false, .dispenserGrid = false},
	    .framing = {.shot = Shot::Placed, .eye = {0.0f, 9.0f, 12.0f}, .look = {k_Tree.x, 3.0f, k_Tree.y}},
	    .objects = {{.type = TreeInfo::Oak, .offset = k_Tree},
	                {.type = AbodeInfo::NorseHut, .offset = k_Tree + glm::vec2(14.0f, 6.0f), .yawDegrees = 200.0f}},
	    .commands = {{.kind = Kind::SetHour, .delaySeconds = 8.0f, .hour = k_Morning},
	                 {.kind = Kind::PointerTo, .delaySeconds = 8.0f, .point = {0.5f, 0.5f}},
	                 {.kind = Kind::PointerPress, .delaySeconds = 0.5f, .value = k_Action},
	                 {.kind = Kind::PointerSweep, .delaySeconds = 0.3f, .point = {0.0f, -0.35f}, .amount = 1.0f}},
	    .fireflyRewards = {{"HEAL", 1.0f}},
	});

	all.push_back({
	    .id = "nature.firefly_seed_by_day",
	    .name = "A firefly's one-shot miracle, by day",
	    .facet = Facet::Nature,
	    .description = "Noon: a firefly hides in an oak by a hut, as a land's script places one, and heal is the only "
	                   "reward. The hand pulls the oak out of the ground and puts it down to the left, taps the globe "
	                   "left where the oak stood, then casts the miracle on a hurt villager in front.",
	    .expected = "Pulled up, the oak leaves its heap of roots and, at the very spot it stood, a one-shot heal globe on "
	                "the ground: the firefly is caught, with no sound or effect of its own. Tapped, the globe pops into the "
	                "hand as a fully charged heal, which is then cast on the villager, healing him. One firefly gives one "
	                "globe; none comes out at night, as none is left.",
	    .environment = {.hour = 12.0f, .clockRuns = false, .dispenserGrid = false},
	    .framing = {.shot = Shot::Placed, .eye = {0.0f, 9.0f, 12.0f}, .look = {k_Tree.x, 3.0f, k_Tree.y}},
	    .objects = {{.type = TreeInfo::Oak, .offset = k_Tree, .firefly = true},
	                {.type = AbodeInfo::NorseHut, .offset = k_Tree + glm::vec2(14.0f, 6.0f), .yawDegrees = 200.0f},
	                {.type = VillagerInfo::NorseFarmerMale, .offset = k_Tree + glm::vec2(4.0f, -7.0f), .life = 0.3f}},
	    .commands = {{.kind = Kind::PointerTo, .delaySeconds = 1.0f, .point = {0.5f, 0.5f}},
	                 {.kind = Kind::PointerPress, .delaySeconds = 0.5f, .value = k_Action},
	                 {.kind = Kind::PointerSweep, .delaySeconds = 0.3f, .point = {0.0f, -0.35f}, .amount = 1.0f},
	                 {.kind = Kind::PointerRelease, .delaySeconds = 1.3f, .value = k_Action},
	                 {.kind = Kind::PointerTo, .delaySeconds = 1.5f, .point = {0.15f, 0.8f}},
	                 {.kind = Kind::PointerPress, .delaySeconds = 1.0f, .value = k_Action},
	                 {.kind = Kind::PointerRelease, .delaySeconds = 0.15f, .value = k_Action},
	                 {.kind = Kind::PointerTo, .delaySeconds = 2.0f, .point = {0.5f, 0.54f}},
	                 {.kind = Kind::PointerPress, .delaySeconds = 1.0f, .value = k_Tap},
	                 {.kind = Kind::PointerRelease, .delaySeconds = 0.15f, .value = k_Tap},
	                 {.kind = Kind::PointerTo, .delaySeconds = 2.0f, .point = {0.75f, 0.85f}},
	                 {.kind = Kind::PointerPress, .delaySeconds = 1.5f, .value = k_Action},
	                 {.kind = Kind::PointerRelease, .delaySeconds = 1.0f, .value = k_Action}},
	    .fireflyRewards = {{"HEAL", 1.0f}},
	});
}
