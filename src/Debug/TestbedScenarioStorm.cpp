/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the storm miracles: the plain storm raining on a village, its fields and a fire, and the
// powered up storm's lightning over a wood, both cast by hand at a circle drawn on the land and thrown north; the clouds
// seen from under them, the drift logged over the lake's banks, and the flashes of a powered up seed held in the hand

#include <vector>

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
/// The storms are cast here, north of the middle, at a circle this wide, and thrown north from a hand to the south
constexpr glm::vec2 k_StormCentre {0.0f, 60.0f};
constexpr float k_StormRadius = 60.0f;
constexpr glm::vec2 k_StormHand {0.0f, 30.0f};
constexpr glm::vec3 k_ThrowNorth {0.0f, 0.0f, 20.0f};

/// A village of three huts with two fields beside them and a few of its people about
std::vector<ObjectSetup> Village()
{
	return {
	    {.type = AbodeInfo::CelticHut, .offset = {-25.0f, 55.0f}, .yawDegrees = 20.0f},
	    {.type = AbodeInfo::CelticHut, .offset = {-5.0f, 70.0f}, .yawDegrees = 160.0f},
	    {.type = AbodeInfo::CelticShackX, .offset = {15.0f, 52.0f}, .yawDegrees = 90.0f},
	    {.type = FieldTypeInfo::Wheat, .offset = {-30.0f, 85.0f}},
	    {.type = FieldTypeInfo::Corn, .offset = {10.0f, 90.0f}},
	    {.type = VillagerInfo::CelticFarmerMale, .offset = {-12.0f, 60.0f}},
	    {.type = VillagerInfo::CelticHousewifeFemale, .offset = {0.0f, 58.0f}},
	    {.type = VillagerInfo::CelticForesterMale, .offset = {8.0f, 65.0f}},
	    // The trees a fireball sets alight before the storm comes
	    {.type = TreeInfo::Oak, .offset = {30.0f, 75.0f}},
	    {.type = TreeInfo::Beech, .offset = {36.0f, 80.0f}},
	};
}
} // namespace

void testbed_scenarios::AddStormScenarios(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.storm_village",
	    .name = "Storm over a village, its fields and a fire",
	    .facet = Facet::Miracles,
	    .description = "A fireball sets two trees by a village alight; then a storm seed is let go by hand after a circle "
	                   "of 60 is drawn over the village, the hand moving north as it lets go.",
	    .expected = "A swirl of dark sprites tightens at the circle's middle and drifts off north, spreading and fading "
	                "after a few seconds. Five gatherings of grey clouds form 90 above the village, starting out twice as "
	                "far and closing in over ten seconds, each cloud thickening and thinning as it circles, their shadows "
	                "darkening the land. The land under them darkens and rain falls in streaks over the village, coming in "
	                "over four seconds, with the rain and wind of the weather heard; the fields grow at their wet rate and "
	                "the burning trees go out sooner. After five seconds the storm drifts north the way it was thrown, the "
	                "rain with it, no slower over slopes than over flat land. There is no lightning. When its 40 seconds are "
	                "up the rain stops at once and the clouds "
	                "fade over three seconds.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-60.0f, 20.0f}, {60.0f, 140.0f}}, .distance = 1.8f},
	    .objects = Village(),
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = {33.0f, 78.0f},
	                  .handOffset = {33.0f, 28.0f},
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 4.0f, 40.0f}},
	                 {.type = MagicType::StormWindRain,
	                  .point = k_StormCentre,
	                  .handOffset = k_StormHand,
	                  .handHeight = 20.0f,
	                  .throwVelocity = k_ThrowNorth,
	                  .delaySeconds = 4.0f,
	                  .byHand = true,
	                  .circleRadius = k_StormRadius}},
	});

	all.push_back({
	    .id = "miracles.storm_script",
	    .name = "Storm cast by a script stays put",
	    .facet = Facet::Miracles,
	    .description = "A script casts the storm over the village, as a challenge's script would, from a point to its "
	                   "south, so that it is cast heading north.",
	    .expected = "The same swirl, clouds and rain as the storm cast by hand, but the storm never drifts: a storm a "
	                "script casts feels no wind, its own or the island's, and stays over the village until it ends.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-60.0f, 20.0f}, {60.0f, 140.0f}}, .distance = 1.8f},
	    .objects = Village(),
	    .miracles = {{.type = MagicType::StormWindRain,
	                  .point = k_StormCentre,
	                  .handOffset = k_StormHand,
	                  .handHeight = 20.0f,
	                  .throwVelocity = k_ThrowNorth,
	                  .delaySeconds = 2.0f,
	                  .player = PlayerNames::NEUTRAL}},
	});

	all.push_back({
	    .id = "miracles.storm_powerup",
	    .name = "Powered up storm's lightning over a wood",
	    .facet = Facet::Miracles,
	    .description = "A powered up storm seed (storm with lightning) let go by hand after a circle of 60 is drawn over a "
	                   "wood and a hut, the hand moving north as it lets go.",
	    .expected = "The same clouds, rain and drift as the plain storm. From about ten seconds on, every one to three "
	                "seconds a cloud of each gathering flashes white blue and lets fly a forked bolt at the trees and the "
	                "hut within its radius, or at the ground, with a flash of light on the land; the cloud's flash lasts half "
	                "a second. The thunder comes from the land under that cloud, later the further that is, small, medium "
	                "or large. The trees struck catch light, the rain "
	                "cooling them. Each bolt costs the miracle two of its prayer power, and its upkeep each turn is its "
	                "plain 25 times the square of 60 over 40, about 56.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-60.0f, 20.0f}, {60.0f, 140.0f}}, .distance = 1.8f},
	    .objects = {{.type = TreeInfo::Oak, .offset = {-20.0f, 50.0f}},
	                {.type = TreeInfo::Beech, .offset = {-10.0f, 65.0f}},
	                {.type = TreeInfo::Conifer, .offset = {0.0f, 55.0f}},
	                {.type = TreeInfo::Oak, .offset = {12.0f, 70.0f}},
	                {.type = TreeInfo::Beech, .offset = {22.0f, 58.0f}},
	                {.type = TreeInfo::Conifer, .offset = {-25.0f, 75.0f}},
	                {.type = AbodeInfo::CelticHut, .offset = {5.0f, 85.0f}, .yawDegrees = 200.0f}},
	    .miracles = {{.type = MagicType::StormWindRainLightning,
	                  .point = k_StormCentre,
	                  .handOffset = k_StormHand,
	                  .handHeight = 20.0f,
	                  .throwVelocity = k_ThrowNorth,
	                  .byHand = true,
	                  .circleRadius = k_StormRadius}},
	});

	all.push_back({
	    .id = "miracles.storm_clouds_below",
	    .name = "Powered up storm's clouds seen from under them",
	    .facet = Facet::Miracles,
	    .description = "The powered up storm cast by hand over a wood, as in the lightning scenario, with the camera low on "
	                   "the land south of the circle looking up at where its clouds gather, 90 above the land.",
	    .expected = "The five gatherings of grey clouds fill the sky ahead, each cloud a soft billboard facing the camera, "
	                "closing in over ten seconds and thickening and thinning as it circles. From about ten seconds on a "
	                "cloud flashes white blue for half a second as it lets fly a bolt down to the trees or the ground.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Placed, .eye = {0.0f, 6.0f, -10.0f}, .look = {0.0f, 90.0f, 60.0f}},
	    .objects = {{.type = TreeInfo::Oak, .offset = {-20.0f, 50.0f}},
	                {.type = TreeInfo::Beech, .offset = {-10.0f, 65.0f}},
	                {.type = TreeInfo::Conifer, .offset = {0.0f, 55.0f}},
	                {.type = TreeInfo::Oak, .offset = {12.0f, 70.0f}}},
	    .miracles = {{.type = MagicType::StormWindRainLightning,
	                  .point = k_StormCentre,
	                  .handOffset = k_StormHand,
	                  .handHeight = 20.0f,
	                  .throwVelocity = k_ThrowNorth,
	                  .byHand = true,
	                  .circleRadius = k_StormRadius}},
	});

	all.push_back({
	    .id = "miracles.storm_drift_slope",
	    .name = "Storm drifting over the lake's banks, logged",
	    .facet = Facet::Miracles,
	    .description = "The plain storm cast by hand over the village and thrown north, across the dip of the lake and up "
	                   "its banks; the storm's position and the land's height under it are logged every second.",
	    .expected = "After five seconds the storm drifts north. In the log, each second's step follows only the wind: it "
	                "eases towards the wind's pace and keeps it whether the land under it falls into the lake's dip, rises "
	                "up its banks or lies flat. It is not slowed on the slopes.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-60.0f, 20.0f}, {60.0f, 260.0f}}, .distance = 1.8f},
	    .objects = Village(),
	    .miracles = {{.type = MagicType::StormWindRain,
	                  .point = k_StormCentre,
	                  .handOffset = k_StormHand,
	                  .handHeight = 20.0f,
	                  .throwVelocity = k_ThrowNorth,
	                  .byHand = true,
	                  .circleRadius = k_StormRadius}},
	    .logMiraclesEvery = 1.0f,
	});

	all.push_back({
	    .id = "miracles.storm_inhand_flash",
	    .name = "Powered up storm seed held in the hand",
	    .facet = Facet::Miracles,
	    .description = "A powered up storm seed is held in the hand for eight seconds before it is cast, the camera close "
	                   "to the hand.",
	    .expected = "The little storm cloud in the hand flashes white blue now and then, each flash fading out over half "
	                "a second, the flashes a quarter to a whole of their gap apart rather than evenly spaced.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Placed, .eye = {0.0f, 24.0f, 10.0f}, .look = {0.0f, 20.0f, 30.0f}},
	    .miracles = {{.type = MagicType::StormWindRainLightning,
	                  .point = k_StormCentre,
	                  .handOffset = k_StormHand,
	                  .handHeight = 20.0f,
	                  .throwVelocity = k_ThrowNorth,
	                  .byHand = true,
	                  .circleRadius = k_StormRadius,
	                  .holdBeforePress = 8.0f}},
	});
}
