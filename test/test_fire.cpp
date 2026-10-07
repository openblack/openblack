/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Fire/FireGraphic.h"
#include "Fire/FireModel.h"
#include "Fire/ViaPoint.h"
#include "Magic/MiracleDeeds.h"
#include "Magic/WaterRules.h"

using namespace openblack;
using namespace openblack::fire;

namespace
{
constexpr float k_Epsilon = 1e-4f;

/// A villager's table row: combustion 120, heat capacity 82.5, burn defence 0.5
Material Villager()
{
	return {.combustion = 120.0f,
	        .capacity = 82.5f,
	        .defence = 0.5f,
	        .burningPriority = 1.0f,
	        .radius = 0.5f,
	        .height = 1.8f,
	        .fireRadius = 0.5f};
}

/// A hut: combustion 150, heat capacity 2000, burn defence 0.01
Material Hut()
{
	return {.combustion = 150.0f,
	        .capacity = 2000.0f,
	        .defence = 0.01f,
	        .burningPriority = 0.5f,
	        .radius = 4.0f,
	        .height = 5.0f,
	        .fireRadius = 4.0f};
}

/// A tree: combustion 110, heat capacity 1000, burn defence 0.01
Material Tree()
{
	return {.combustion = 110.0f,
	        .capacity = 1000.0f,
	        .defence = 0.01f,
	        .burningPriority = 0.5f,
	        .radius = 2.0f,
	        .height = 8.0f,
	        .fireRadius = 2.0f};
}
} // namespace

TEST(FireModel, NothingCatchesBelowFortyDegreesAndHoldsAtLeastOneUnitOfHeat)
{
	Material cold {.combustion = 10.0f, .capacity = 0.0f};
	EXPECT_FLOAT_EQ(CombustionTemperature(cold), 40.0f);
	EXPECT_FLOAT_EQ(MaxTemperature(cold), 80.0f);
	// A creature's table gives a capacity that is next to nothing as a float
	cold.capacity = 2.1e-43f;
	EXPECT_FLOAT_EQ(Capacity(cold), 1.0f);
}

TEST(FireModel, BurningAtTwiceItsCombustionHurtsByItsDefence)
{
	// A villager loses a twentieth of its life a turn, a hut a thousandth
	EXPECT_NEAR(BurnDamage(240.0f, Villager()), 0.05f, k_Epsilon);
	EXPECT_NEAR(BurnDamage(300.0f, Hut()), 0.001f, k_Epsilon);
	EXPECT_FLOAT_EQ(BurnDamage(120.0f, Villager()), 0.0f);
}

TEST(FireModel, TheFireIsFiercestAtTwiceItsCombustionAndWeakAsItsObjectBurnsAway)
{
	const auto hut = Hut();
	EXPECT_FLOAT_EQ(FireFraction(120.0f, hut, 1.0f), 0.0f);
	EXPECT_FLOAT_EQ(FireFraction(300.0f, hut, 1.0f), 1.0f);
	EXPECT_NEAR(FireFraction(210.0f, hut, 1.0f), (210.0f - 120.0f) / 180.0f, k_Epsilon);
	// No more than twice the life left
	EXPECT_NEAR(FireFraction(300.0f, hut, 0.1f), 0.2f, k_Epsilon);
	EXPECT_FLOAT_EQ(FireRadius(hut, 1.0f), 5.0f);
	EXPECT_FLOAT_EQ(MaxFireRadius(hut), 5.0f);
	EXPECT_FLOAT_EQ(SafeFireRadius(2.0f, 5.0f), 3.0f);
	EXPECT_NEAR(FlameHeight(300.0f, hut), 5.0f * 1.25f, k_Epsilon);
	EXPECT_FLOAT_EQ(FlameHeight(k_AmbientTemperature, hut), 0.0f);
}

TEST(FireModel, ABurnPullsTheTemperatureTowardsTheAirsPlusTheBurn)
{
	// Water's -4000 cools something light at once, a hut by about 21 degrees a drop
	Material light {.combustion = 100.0f, .capacity = 10.0f};
	EXPECT_NEAR(ApplyBurn(300.0f, light, -4000.0f), k_AmbientTemperature - 4000.0f, 0.01f);
	EXPECT_NEAR(ApplyBurn(300.0f, Hut(), -4000.0f), 300.0f + 10.0f * (k_AmbientTemperature - 4300.0f) / 2000.0f, 0.01f);
	// A villager's beat cools a burning hut by about 1.4 degrees
	EXPECT_NEAR(300.0f - ApplyBurn(300.0f, Hut(), -8.0f), 1.4165f, 0.001f);
	// Each turn of a blast's 200 heats a villager by ten times the gap over its capacity
	EXPECT_NEAR(ApplyBurn(k_AmbientTemperature, Villager(), 200.0f), k_AmbientTemperature + 2000.0f / 82.5f, 0.01f);
	EXPECT_FLOAT_EQ(SetOnFireTemperature(Hut(), 0.5f), 300.0f * 0.5f + 150.0f);
}

TEST(FireModel, AFireHeatsItsNeighbourByTheDifferenceUpToHalfItsHeat)
{
	const auto tree = Tree();
	const auto hut = Hut();
	// Ten for each degree of difference, which changes the hut by no more than the difference
	const auto transfer = HeatTransfer(220.0f, tree, k_AmbientTemperature, hut);
	EXPECT_NEAR(transfer.targetTemperature, k_AmbientTemperature + 10.0f * (220.0f - k_AmbientTemperature) / 2000.0f, 0.01f);
	EXPECT_FLOAT_EQ(transfer.sourceTemperature, 220.0f);
	// Something hot but not burning, as a fireball below its combustion, loses what it gives
	Material ball {.combustion = 2000.0f, .capacity = 75.0f};
	const auto cooling = HeatTransfer(1500.0f, ball, k_AmbientTemperature, Villager());
	const float heat = 10.0f * (1500.0f - k_AmbientTemperature);
	EXPECT_NEAR(cooling.sourceTemperature, 1500.0f - heat / 75.0f, 0.01f);
	EXPECT_NEAR(cooling.targetTemperature, k_AmbientTemperature + heat / 82.5f, 0.01f);
	// Nothing passes to something hotter
	EXPECT_FLOAT_EQ(HeatTransfer(100.0f, tree, 200.0f, hut).targetTemperature, 200.0f);
}

TEST(FireModel, ABurningThingHeatsItselfSlowlyAndHurts)
{
	const auto hut = Hut();
	State state {.temperature = 150.0f, .previous = 140.0f};
	const auto outcome = Step(state, hut, {.life = 1.0f});
	EXPECT_FALSE(outcome.gone);
	EXPECT_NE(state.flags & k_JustIgnited, 0);
	EXPECT_NEAR(state.temperature, 150.0f + 0.1f * 150.0f / 300.0f, k_Epsilon);
	EXPECT_NEAR(outcome.damage, BurnDamage(state.temperature, hut), k_Epsilon);
	// It heats itself no hotter than twice its combustion
	state = {.temperature = 300.0f, .previous = 300.0f};
	(void)Step(state, hut, {.life = 1.0f});
	EXPECT_FLOAT_EQ(state.temperature, 300.0f);
	EXPECT_EQ(state.flags & k_VeryHot, 0);
}

TEST(FireModel, SomethingNotBurningCoolsBySurfaceOverCapacityFasterInTheWetAndRain)
{
	const auto tree = Tree();
	const float t = 100.0f;
	const float dry = t - (t + 10.0f - k_AmbientTemperature) * (4.0f * 8.0f * 2.0f) * 0.1f / 1000.0f;
	State state {.temperature = t, .previous = t};
	(void)Step(state, tree, {.life = 1.0f});
	EXPECT_NEAR(state.temperature, dry, k_Epsilon);
	state = {.temperature = t, .previous = t};
	(void)Step(state, tree, {.inWater = true, .life = 1.0f});
	EXPECT_NEAR(state.temperature, t - (t - dry) * 50.0f, k_Epsilon);
	state = {.temperature = t, .previous = t};
	const auto outcome = Step(state, tree, {.rain = 127.0f, .life = 1.0f});
	EXPECT_TRUE(outcome.rainedOn);
	EXPECT_NEAR(state.temperature, t - (t - dry) * 2.27f, k_Epsilon);
	// Heated this turn, it doesn't cool
	state = {.temperature = t, .previous = t - 1.0f};
	(void)Step(state, tree, {.life = 1.0f});
	EXPECT_FLOAT_EQ(state.temperature, t);
	// A burning thing cooled below its combustion has just gone out
	state = {.temperature = 111.0f, .previous = 111.0f};
	(void)Step(state, tree, {.inWater = true, .life = 1.0f});
	EXPECT_NE(state.flags & k_JustExtinguished, 0);
}

TEST(FireModel, ItCharsAsItsLifeRunsLowAndGoesOnceColdAndClean)
{
	const auto hut = Hut();
	State state {.temperature = 300.0f, .previous = 300.0f};
	(void)Step(state, hut, {.life = 0.3f});
	EXPECT_NEAR(state.charring, 0.04f, k_Epsilon);
	// No more charred than its lost life allows
	state.charring = 0.6f;
	(void)Step(state, hut, {.life = 0.3f});
	EXPECT_NEAR(state.charring, 0.5f, k_Epsilon);
	// Out, the charring fades
	state = {.temperature = 30.0f, .previous = 30.0f, .charring = 0.3f};
	(void)Step(state, hut, {.life = 0.3f});
	EXPECT_NEAR(state.charring, 0.28f, k_Epsilon);
	state = {.temperature = k_AmbientTemperature + 0.05f, .previous = k_AmbientTemperature, .charring = 0.0f};
	EXPECT_TRUE(Step(state, hut, {.life = 1.0f}).gone);
}

TEST(FireGraphic, FlamesBySizeAndCellsByAge)
{
	EXPECT_EQ(graphic::MaxFlames(true, 10.0f, 10.0f), 2);
	EXPECT_EQ(graphic::MaxFlames(false, 1.0f, 2.0f), 2);
	EXPECT_EQ(graphic::MaxFlames(false, 1.0f, 3.0f), 7);
	EXPECT_FLOAT_EQ(graphic::LocalFlameScale(graphic::FlameShape::Tree, 10.0f), 2.0f);
	EXPECT_FLOAT_EQ(graphic::LocalFlameScale(graphic::FlameShape::Jointed, 10.0f), 3.0f);
	EXPECT_FLOAT_EQ(graphic::LocalFlameScale(graphic::FlameShape::Plain, 10.0f), 5.0f);
	EXPECT_EQ(graphic::FlameCell(0.0f), 32);
	EXPECT_EQ(graphic::FlameCell(0.1f), 29);
	EXPECT_EQ(graphic::PuffCell(0.1f), 2);
	EXPECT_EQ(graphic::CharredGrey(0.0f), 255);
	EXPECT_EQ(graphic::CharredGrey(1.0f), 80);
	EXPECT_NEAR(graphic::LightStrength(1.0f, 0.0f, 0.0f), 0.6f, k_Epsilon);
	// The glow: full at 1000 degrees, 45/64 red and 15/64 green of it; none out of the fire's heat
	EXPECT_EQ(graphic::GlowColour(1000.0f, 0.0f, 0.0f), (179u << 16u) | (59u << 8u));
	EXPECT_EQ(graphic::GlowColour(2000.0f, 0.0f, 0.0f), (179u << 16u) | (59u << 8u));
	EXPECT_EQ(graphic::GlowColour(500.0f, 0.0f, 0.0f), (89u << 16u) | (29u << 8u));
	EXPECT_EQ(graphic::GlowColour(500.0f, 0.5f, 1.0f), (53u << 16u) | (17u << 8u));
	EXPECT_EQ(graphic::GlowColour(-10.0f, 0.0f, 0.0f), 0u);
	EXPECT_NEAR(graphic::LightStrength(1.0f, 0.5f, 1.0f), 0.6f * 1.2f * 0.5f, k_Epsilon);
}

TEST(FireGraphic, FlamesGrowInByTheFireAndFadeOver4Seconds)
{
	graphic::Graphic look;
	look.maxFlames = 7;
	look.localScale = 2.0f;
	const graphic::Sampler sampler {
	    .localPoint = [] { return std::optional(glm::vec3(1.0f)); },
	    .toWorld = [](const glm::vec3& p) { return p; },
	    .random = [](float range) { return range * 0.5f; },
	};
	// Catching adds half its flames at once
	(void)graphic::Update(look, {.fraction = 1.0f, .temperature = 300.0f, .flags = k_JustIgnited}, 0.0f, sampler);
	EXPECT_EQ(look.flames.size(), 4u);
	// Then as many as it keeps every 4.3 s at its fiercest
	(void)graphic::Update(look, {.fraction = 1.0f, .temperature = 300.0f}, 4.3f / 7.0f, sampler);
	EXPECT_EQ(look.flames.size(), 5u);
	EXPECT_NEAR(look.flames.front().scale, 2.0f, k_Epsilon);
	EXPECT_NEAR(look.flames.front().alpha, 250.0f * (4.3f / 7.0f), 0.01f);
	(void)graphic::Update(look, {.fraction = 0.0f, .temperature = 30.0f}, 4.0f, sampler);
	EXPECT_TRUE(look.flames.empty());
}

TEST(FireGraphic, SteamHissesOnceAsAHotFireIsCooledAndPuffsForThirtyTurns)
{
	graphic::Graphic look;
	look.localScale = 1.0f;
	const graphic::Sampler sampler {
	    .localPoint = [] { return std::optional(glm::vec3(0.0f)); },
	    .toWorld = [](const glm::vec3& p) { return p; },
	    .random = [](float range) { return range * 0.5f; },
	};
	EXPECT_TRUE(graphic::Update(look, {.temperature = 200.0f, .flags = k_Cooling, .turn = 10}, 0.0f, sampler));
	EXPECT_FALSE(graphic::Update(look, {.temperature = 150.0f, .flags = k_Cooling, .turn = 11}, 1.0f, sampler));
	EXPECT_EQ(look.steam.size(), 4u);
	(void)graphic::Update(look, {.temperature = 150.0f, .flags = k_Cooling, .turn = 41}, 0.0f, sampler);
	EXPECT_FALSE(look.steamStart.has_value());
	// Going out, it smokes
	(void)graphic::Update(look, {.temperature = 100.0f, .flags = k_JustExtinguished, .turn = 50}, 0.0f, sampler);
	(void)graphic::Update(look, {.temperature = 100.0f, .turn = 51}, 0.5f, sampler);
	EXPECT_EQ(look.smoke.size(), 2u);
	EXPECT_NEAR(look.smoke.front().alpha, std::round(180.0f * (1.0f - 0.5f / 3.0f)), 0.01f);
}

TEST(FireGraphic, AGraphicLongUndrawnCatchesUpByAFlamesLife)
{
	graphic::Graphic look;
	look.maxFlames = 7;
	look.localScale = 1.0f;
	const graphic::Sampler sampler {
	    .localPoint = [] { return std::optional(glm::vec3(0.0f)); },
	    .toWorld = [](const glm::vec3& p) { return p; },
	    .random = [](float range) { return range * 0.5f; },
	};
	(void)graphic::Update(look, {.fraction = 0.5f, .temperature = 300.0f, .turn = 100}, 0.0f, sampler);
	EXPECT_TRUE(look.flames.empty());
	// Ten turns unseen is not enough
	(void)graphic::Update(look, {.fraction = 0.6f, .temperature = 300.0f, .turn = 110}, 0.0f, sampler);
	EXPECT_TRUE(look.flames.empty());
	// Eleven, with the fire changed, and it moves on 4.3 s at once: 4.9 flames' worth at 0.7, so five
	(void)graphic::Update(look, {.fraction = 0.7f, .temperature = 300.0f, .turn = 121}, 0.0f, sampler);
	EXPECT_EQ(look.flames.size(), 5u);
	// Unchanged, it doesn't
	const auto before = look.flames.size();
	(void)graphic::Update(look, {.fraction = 0.7f, .temperature = 300.0f, .turn = 140}, 0.0f, sampler);
	EXPECT_EQ(look.flames.size(), before);
}

TEST(WaterRules, AFieldIsSownAtOnceThenRipensWithEachDrop)
{
	magic::WaterCrop crop;
	const magic::WaterCropType type {};
	magic::WaterField(crop, type);
	EXPECT_EQ(crop.timesSown, 31);
	magic::WaterField(crop, type);
	EXPECT_FLOAT_EQ(crop.age, 2.0f);
	EXPECT_NEAR(crop.food, 2.0f * 350.0f / 1200.0f, k_Epsilon);
	crop.age = 1201.0f;
	magic::WaterField(crop, type);
	EXPECT_FLOAT_EQ(crop.age, 1201.0f);
}

TEST(WaterRules, TheWaterGrowsYoungTreesAndTheExtremeAnyTree)
{
	const magic::WaterTreeType type {.growthAmount = 0.01f, .waterAccelerator = 1.0f};
	auto grown = magic::WaterTree(0.5f, 1.2f, type, false);
	EXPECT_TRUE(grown.canGrow);
	EXPECT_NEAR(grown.scale, 0.51f, k_Epsilon);
	// Full grown, the normal water does nothing
	grown = magic::WaterTree(1.2f, 1.2f, type, false);
	EXPECT_FALSE(grown.canGrow);
	EXPECT_FLOAT_EQ(grown.scale, 1.2f);
	// The extreme grows it past its size, ever more slowly
	grown = magic::WaterTree(1.2f, 1.2f, type, true);
	EXPECT_NEAR(grown.scale, 1.2f + 0.01f * 0.8855627f * 0.5f, 1e-6f);
	EXPECT_FLOAT_EQ(grown.target, grown.scale);
	EXPECT_FLOAT_EQ(magic::WaterTree(3.0f, 3.0f, type, true).scale, 3.0f);
}

TEST(MiracleDeeds, TheLastThingAMiracleReachedSaysWhatThePlayerDid)
{
	using magic::MiracleDeed;
	const magic::DeedTarget nothing {};
	// Near another player's town a miracle not meant to anger is an attempt to impress it
	EXPECT_EQ(MiracleDeed(MagicType::Water, false, true, nothing), magic::k_DeedImpressWithMagic);
	EXPECT_EQ(MiracleDeed(MagicType::Fireball, true, true, nothing), magic::k_DeedDamageWithFire);
	// The water: a field is crops, something burning a fire put out, anything else crops too
	EXPECT_EQ(MiracleDeed(MagicType::WaterPowerUpOne, false, false, {.field = true, .onFire = true}),
	          magic::k_DeedCastWaterOnCrops);
	EXPECT_EQ(MiracleDeed(MagicType::Water, false, false, {.onFire = true}), magic::k_DeedCastWaterToPutOutFire);
	EXPECT_EQ(MiracleDeed(MagicType::Water, false, false, nothing), magic::k_DeedCastWaterOnCrops);
	// The blast's second power-up shows nothing; the others damage with magic
	EXPECT_EQ(MiracleDeed(MagicType::ExplosionOnePuOne, true, false, nothing), magic::k_DeedDamageWithMagic);
	EXPECT_EQ(MiracleDeed(MagicType::ExplosionOnePuTwo, true, false, nothing), magic::k_NoDeed);
	EXPECT_EQ(MiracleDeed(MagicType::Food, false, false, {.storagePit = true}), magic::k_DeedCastFoodInStoragePit);
	EXPECT_EQ(MiracleDeed(MagicType::Wood, false, false, nothing), magic::k_NoDeed);
	EXPECT_EQ(MiracleDeed(MagicType::FlockGround, true, false, nothing), magic::k_DeedImpressWithMagic);
}

TEST(FireGraphic, ABurningTreeDarkensThinsAndShrinksAwayAtTheLast)
{
	// Hot through, it is drawn at 50 of 256, its foliage cut away below 254
	auto look = graphic::BurningTree(200.0f, 110.0f, 0.5f);
	EXPECT_EQ(look.grey, 50);
	EXPECT_FLOAT_EQ(look.alphaReference, 254.0f);
	EXPECT_FLOAT_EQ(look.scale, 1.0f);
	// Barely hurt and just warm, it is lighter and keeps its foliage nearer its own
	look = graphic::BurningTree(110.0f, 110.0f, 0.95f);
	EXPECT_EQ(look.grey, static_cast<int>(255.0f - 0.05f * 2550.0f));
	EXPECT_FLOAT_EQ(look.alphaReference, 230.0f);
	// In the last fifth of its life it shrinks to nothing
	EXPECT_NEAR(graphic::BurningTree(200.0f, 110.0f, 0.1f).scale, 0.5f, k_Epsilon);
	EXPECT_NEAR(graphic::BurningTree(200.0f, 110.0f, 0.0f).scale, 0.0f, k_Epsilon);
}

TEST(ViaPoint, AVillagerGoesRoundAFireInItsWayOnTheSideItLeansTo)
{
	using openblack::map_coords::MapCoords;
	using openblack::map_coords::ToFixed;
	const auto at = [](float x, float z) { return MapCoords {ToFixed(x), ToFixed(z), 0.0f}; };
	// A fire of 5 m in the middle of the way from 0 to 40 along x, passed by a margin of 1 m
	const auto via = openblack::fire::GetViaPoint(at(0.0f, 0.0f), at(40.0f, 1.0f), at(20.0f, 0.0f), 5.0f, 1.0f, 0.0f);
	EXPECT_TRUE(via.detour);
	EXPECT_FALSE(via.inside);
	EXPECT_NE(via.angle, 0.0f);
	// The point lies on the tangent from the start to the circle grown by the margin: 6 m from the centre
	const float dx = static_cast<float>(via.point.x - ToFixed(20.0f));
	const float dz = static_cast<float>(via.point.z);
	EXPECT_NEAR(std::sqrt(dx * dx + dz * dz) / openblack::map_coords::k_FixedPerMetre, 6.0f, 0.01f);
	// Kept to the same side when asked
	const auto again = openblack::fire::GetViaPoint(at(0.0f, 0.0f), at(40.0f, 1.0f), at(20.0f, 0.0f), 5.0f, 1.0f, via.angle);
	EXPECT_EQ(again.point.z > 0, via.point.z > 0);
	// A way that clears the circle needs no detour, and a start inside it none either
	EXPECT_FALSE(openblack::fire::GetViaPoint(at(0.0f, 20.0f), at(40.0f, 20.0f), at(20.0f, 0.0f), 5.0f, 1.0f, 0.0f).detour);
	EXPECT_TRUE(openblack::fire::GetViaPoint(at(19.0f, 0.0f), at(40.0f, 0.0f), at(20.0f, 0.0f), 5.0f, 1.0f, 0.0f).inside);
}
