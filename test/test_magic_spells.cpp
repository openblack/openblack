/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <map>
#include <memory>
#include <vector>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "ECS/Components/Spell.h"
#include "Magic/DispenserRules.h"
#include "Magic/MagicTables.h"
#include "Magic/MagicWorldInterface.h"
#include "Magic/SpellBehaviours.h"
#include "Magic/SpellGrid.h"
#include "Magic/SpellRules.h"
#include "Magic/SpellSeedRules.h"

using namespace openblack;
using namespace openblack::magic;
using openblack::ecs::components::Spell;
using openblack::ecs::components::SpellCaster;
using openblack::particles::SpellEventInfo;

namespace
{
constexpr float k_Epsilon = 1e-4f;
constexpr float k_TurnSeconds = 0.1f;

/// Zeroed tables where each magic record names the magic type it sits at, as the file does
std::unique_ptr<InfoConstants> MakeTables()
{
	auto info = std::make_unique<InfoConstants>();
	uint32_t type = 0;
	const auto number = [&type](auto& records) {
		for (auto& record : records)
		{
			record.magicType = static_cast<MagicType>(type++);
		}
	};
	number(info->magicGeneral);
	number(info->magicHeal);
	number(info->magicTeleport);
	number(info->magicForest);
	number(info->magicFood);
	number(info->magicStormAndTornado);
	number(info->magicShield);
	number(info->magicWood);
	number(info->magicWater);
	number(info->magicFlockFlying);
	number(info->magicFlockGround);
	number(info->magicCreatureSpell);
	return info;
}

GMagicEffectInfo& Effect(InfoConstants& info, MagicType type)
{
	return info.magicEffect.at(static_cast<size_t>(type));
}

/// Flat dry land everywhere, which records what the miracles do to it
class FakeWorld final: public MagicWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 /*xz*/) const override { return 0.0f; }
	[[nodiscard]] bool InBounds(glm::vec3 point) const override { return point.x >= 0.0f && point.z >= 0.0f; }
	[[nodiscard]] bool IsDryLand(glm::vec3 point) const override { return InBounds(point) && !(point.x < wetBelowX); }
	[[nodiscard]] bool IsLand(glm::vec3 point) const override
	{
		return InBounds(point) && !(point.x < wetBelowX) && !(point.z < waterBelowZ);
	}
	[[nodiscard]] bool InInfluence(PlayerNames /*player*/, glm::vec3 /*point*/) const override { return influence; }
	[[nodiscard]] std::optional<glm::vec3> PositionOf(entt::entity object) const override
	{
		const auto found = objects.find(object);
		return found != objects.end() ? std::optional(found->second) : std::nullopt;
	}
	bool ApplyEffect(entt::entity object, const EffectValues& values, const EffectSource& source) override
	{
		lastSource = source;
		applied.emplace_back(object, values);
		return true;
	}
	std::vector<entt::entity> ApplyEffectAt(glm::vec3 point, const EffectValues& values, const EffectSource& source) override
	{
		lastSource = source;
		appliedAt.emplace_back(point, values);
		return {};
	}
	void Water(const WaterDrop& drop) override
	{
		drops.push_back(drop.position);
		rings.push_back(drop.ringGrowth);
		extremeDrops.push_back(drop.extreme);
	}
	[[nodiscard]] std::vector<entt::entity> HealTargets(glm::vec3 /*point*/, float radius, size_t maximum) const override
	{
		lastHealRadius = radius;
		std::vector<entt::entity> targets;
		for (const auto& [entity, position] : objects)
		{
			if (targets.size() < maximum)
			{
				targets.push_back(entity);
			}
		}
		return targets;
	}
	bool AddResource(ResourceType type, glm::vec3 point, uint32_t amount, bool sparkles, PlayerNames /*player*/) override
	{
		resources.push_back({type, point, amount, sparkles});
		return true;
	}

	struct ResourceRecord
	{
		ResourceType type;
		glm::vec3 point;
		uint32_t amount;
		bool sparkles;
	};
	bool influence {true};
	float wetBelowX {-1.0f};
	/// Cells with water in them, though their land stands above the sea, south of this
	float waterBelowZ {-1.0f};
	std::map<entt::entity, glm::vec3> objects;
	std::vector<std::pair<entt::entity, EffectValues>> applied;
	std::vector<std::pair<glm::vec3, EffectValues>> appliedAt;
	EffectSource lastSource;
	std::vector<glm::vec3> drops;
	std::vector<bool> extremeDrops;
	std::vector<std::optional<float>> rings;
	std::vector<ResourceRecord> resources;
	mutable float lastHealRadius {0.0f};
};

/// The miracle system's services on fakes: one player who gives nothing, a tribal power, shields by hand
class FakeServices final: public SpellServicesInterface
{
public:
	explicit FakeServices(InfoConstants& tables)
	    : info(tables)
	{
	}
	[[nodiscard]] const InfoConstants& Info() const override { return info; }
	[[nodiscard]] MagicWorldInterface& World() override { return world; }
	[[nodiscard]] SpellCasterInterface* CasterOf(const Spell& spell) override
	{
		return spell.caster.kind == SpellCaster::Kind::None ? nullptr : &caster;
	}
	[[nodiscard]] float TribalPower(const Spell& /*spell*/) const override { return tribalPower; }
	[[nodiscard]] float SeedPower(const Spell& /*spell*/) const override { return 1.0f; }
	void AddEffectTarget(const Spell& /*spell*/, entt::entity target) override { targets.push_back(target); }
	[[nodiscard]] std::optional<entt::entity> ShieldAt(glm::vec3 point, float margin) override
	{
		if (shield.has_value() && glm::distance(point, shieldCentre) < shieldRadius + margin)
		{
			return shield;
		}
		return std::nullopt;
	}
	void StrikeShield(entt::entity /*shieldSpell*/, glm::vec3 point) override { strikes.push_back(point); }
	[[nodiscard]] Spell* FindSpell(entt::entity entity) override
	{
		const auto found = spells.find(entity);
		return found != spells.end() ? found->second : nullptr;
	}
	[[nodiscard]] float GameRandom(float max) override { return max * 0.5f; }
	void ReactToSpell(Spell& spell, bool onCast) override
	{
		if (spell.reaction == 0)
		{
			spell.reaction = 1;
			reactions.push_back(onCast);
		}
	}
	void StartCastEffect(Spell& /*spell*/, ParticleType type) override { castEffects.push_back(type); }
	void ShieldStruck(const Spell& struckShield, bool destroyed) override
	{
		for (const auto& [entity, spell] : spells)
		{
			if (spell == &struckShield)
			{
				struck.emplace_back(entity, destroyed);
			}
		}
	}
	[[nodiscard]] bool HasWorldObjects(const Spell& /*spell*/) const override { return worldObjects; }
	bool PlantForest(Spell& spell) override
	{
		++planted;
		spell.forestPlanted = true;
		return true;
	}
	[[nodiscard]] bool ForestCanGrowAt(glm::vec3 /*point*/) const override { return forestGrows; }

	InfoConstants& info;
	FakeWorld world;
	PlayerSpellCaster caster {PlayerNames::PLAYER_ONE};
	float tribalPower {1.0f};
	std::vector<entt::entity> targets;
	std::optional<entt::entity> shield;
	glm::vec3 shieldCentre {0.0f};
	float shieldRadius {0.0f};
	std::vector<glm::vec3> strikes;
	std::map<entt::entity, Spell*> spells;
	std::vector<bool> reactions;
	std::vector<ParticleType> castEffects;
	std::vector<std::pair<entt::entity, bool>> struck;
	bool worldObjects {false};
	int planted {0};
	bool forestGrows {true};
};

Spell MakeSpell(MagicType type, float chants)
{
	Spell spell;
	spell.magicType = type;
	spell.spellClass = ClassOf(type);
	spell.caster = {.kind = SpellCaster::Kind::Player, .player = PlayerNames::PLAYER_ONE, .entity = entt::null};
	SetChants(spell.chants, chants);
	spell.castPosition = {100.0f, 0.0f, 100.0f};
	return spell;
}

SpellEventInfo EventAt(SpellEventInfo::Type type, glm::vec3 position, entt::entity target = entt::null)
{
	return {.type = type,
	        .position = position,
	        .velocity = glm::vec3(1.0f, 0.0f, 0.0f),
	        .strength = 1.0f,
	        .checkShields = false,
	        .target = target};
}
} // namespace

// The rules of the tables

TEST(SpellRules, EachMagicTypeIsOfItsSectionsKind)
{
	EXPECT_EQ(ClassOf(MagicType::Fireball), SpellClass::General);
	EXPECT_EQ(ClassOf(MagicType::LightningBoltPowerUpTwo), SpellClass::General);
	EXPECT_EQ(ClassOf(MagicType::HealPowerUpOne), SpellClass::Heal);
	EXPECT_EQ(ClassOf(MagicType::Food), SpellClass::Resource);
	EXPECT_EQ(ClassOf(MagicType::Wood), SpellClass::Resource);
	EXPECT_EQ(ClassOf(MagicType::Shield), SpellClass::Shield);
	EXPECT_EQ(ClassOf(MagicType::WaterPowerUpOne), SpellClass::Water);
	EXPECT_EQ(ClassOf(MagicType::CreatureSpellBig), SpellClass::Creature);
}

TEST(SpellRules, ASeedsCastScalesThePrayerPowerAndTimeAndAFireSeedIsAlwaysTheSameSize)
{
	auto info = MakeTables();
	Effect(*info, MagicType::LightningBolt).initialChants = 5000.0f;
	Effect(*info, MagicType::LightningBolt).timerWhenPlayerCasting = 6.0f;
	Effect(*info, MagicType::Food).timerWhenPlayerCasting = -1.0f;
	const auto lightning = SeedCastData(*info, MagicType::LightningBolt, SpellSeedType::LightningBolt, 2.0f, 7.0f);
	EXPECT_FLOAT_EQ(lightning.chants, 10000.0f);
	EXPECT_FLOAT_EQ(lightning.duration, 12.0f);
	EXPECT_FLOAT_EQ(lightning.magnitude, 7.0f);
	// No time limit stays none however long the multiplier
	EXPECT_FLOAT_EQ(SeedCastData(*info, MagicType::Food, SpellSeedType::Food, 3.0f, 1.0f).duration, k_NoTimeLimit);
	EXPECT_FLOAT_EQ(SeedCastData(*info, MagicType::Fireball, SpellSeedType::Fire, 1.0f, 30.0f).magnitude, 1.0f);
}

TEST(SpellRules, AMiracleOutlivesItsTimeOnlyWhenItHasOne)
{
	float age = 0.0f;
	for (int turn = 0; turn < 60; ++turn)
	{
		EXPECT_FALSE(AgeOneTurn(age, 6.0f, k_TurnSeconds)) << turn;
	}
	EXPECT_TRUE(AgeOneTurn(age, 6.0f, k_TurnSeconds));
	float forever = 0.0f;
	for (int turn = 0; turn < 1000; ++turn)
	{
		EXPECT_FALSE(AgeOneTurn(forever, k_NoTimeLimit, k_TurnSeconds));
	}
}

TEST(SpellRules, AnEffectHurtsByCrushAndHitAndHealsByItsDefences)
{
	EffectValues values;
	values[EffectKind::Crush] = 0.2f;
	values[EffectKind::Hit] = 0.3f;
	values[EffectKind::Heal] = 0.5f;
	values[EffectKind::FlyAway] = -1.0f;
	EffectDefence defence;
	defence.multipliers.at(static_cast<size_t>(EffectKind::Hit)) = 0.5f;
	EXPECT_NEAR(DamageFrom(values, defence), 0.2f + 0.15f, k_Epsilon);
	EXPECT_NEAR(HealFrom(values, defence), 0.5f, k_Epsilon);
	// Healed first, up to whole, then hurt
	EXPECT_NEAR(LifeAfter(0.8f, values, defence), 1.0f - 0.35f, k_Epsilon);
	EXPECT_TRUE(values.IsDestructive());
	values.Scale(2.0f);
	EXPECT_NEAR(values[EffectKind::Crush], 0.4f, k_Epsilon);
	// A negative value hurts nothing
	EffectValues soothing;
	soothing[EffectKind::Hit] = -1.0f;
	EXPECT_FLOAT_EQ(DamageFrom(soothing, defence), 0.0f);
	EXPECT_FALSE(soothing.IsDestructive());
}

TEST(SpellRules, HeatHurtsOnlyAboveTheCombustionTemperature)
{
	EffectDefence defence;
	defence.combustionTemperature = 200.0f;
	defence.multipliers.at(static_cast<size_t>(EffectKind::Burn)) = 0.02f;
	EXPECT_FLOAT_EQ(HeatDamage(199.0f, defence), 0.0f);
	// Three combustion temperatures over, times its burn defence, a tenth of that
	EXPECT_NEAR(HeatDamage(800.0f, defence), 3.0f * 0.02f * k_HeatDamageScale, k_Epsilon);
	EffectDefence fireproof;
	EXPECT_FLOAT_EQ(HeatDamage(10000.0f, fireproof), 0.0f);
}

TEST(SpellRules, FoodAndWoodCostByTheirUnitsAndTheFirstGrainBringsMore)
{
	GMagicResourceInfo food {};
	food.resourceAmountFirstEvent = 200;
	food.resourceAmountPerEvent = 18;
	food.costPerUnit = 7;
	const auto first = ResourceEvent(food, false);
	EXPECT_EQ(first.units, 200u);
	EXPECT_FLOAT_EQ(first.chants, 1400.0f);
	const auto next = ResourceEvent(food, true);
	EXPECT_EQ(next.units, 18u);
	EXPECT_FLOAT_EQ(next.chants, 126.0f);
	EXPECT_TRUE(HasEnoughChantsForResourceRecast(food, 1400.0f));
	EXPECT_FALSE(HasEnoughChantsForResourceRecast(food, 1399.0f));
}

TEST(SpellRules, AShieldsSizeIsClampedAndItsUpkeepGrowsWithItsArea)
{
	GMagicShieldInfo shield {};
	shield.minRadius = 5.0f;
	shield.maxRadius = 1000.0f;
	EXPECT_FLOAT_EQ(ClampShieldRadius(shield, 1.0f), 5.0f);
	EXPECT_FLOAT_EQ(ClampShieldRadius(shield, 40.0f), 40.0f);
	EXPECT_FLOAT_EQ(ClampShieldRadius(shield, 5000.0f), 1000.0f);
	EXPECT_FLOAT_EQ(ShieldCostToMaintain(20.0f, 30.0f, 30.0f), 20.0f);
	EXPECT_FLOAT_EQ(ShieldCostToMaintain(20.0f, 60.0f, 30.0f), 80.0f);
}

TEST(SpellRules, TheWaterRainsWiderForItsPowerUp)
{
	EXPECT_FLOAT_EQ(RainRadius(MagicType::Water), 6.0f);
	EXPECT_FLOAT_EQ(RainRadius(MagicType::WaterPowerUpOne), 12.0f);
	EXPECT_FLOAT_EQ(RippleGrowth(MagicType::WaterPowerUpOne), 4.0f);
	// A drop falls between 0.3 and 0.7 of the radius plus 0.3 from the middle
	EXPECT_FLOAT_EQ(DropDistance(0.0f), 0.3f);
	EXPECT_FLOAT_EQ(DropDistance(6.0f), 4.5f);
}

// Seeds in the hand

TEST(SpellSeedRules, TheCastTypeDecidesHowTheHandCasts)
{
	EXPECT_EQ(CastStyleOf(SpellCastType::SpellCastInHand), CastStyle::Held);
	EXPECT_EQ(CastStyleOf(SpellCastType::SpellCastHandGesture), CastStyle::OnRelease);
	EXPECT_EQ(CastStyleOf(SpellCastType::SpellCastHandPosition), CastStyle::OnPress);
}

TEST(SpellSeedRules, ASeedsChargeAndReadiness)
{
	EXPECT_FLOAT_EQ(ChantNeeded(3500.0f, 1000.0f), 2500.0f);
	EXPECT_FLOAT_EQ(SeedPower(1750.0f, 3500.0f), 0.5f);
	EXPECT_FLOAT_EQ(SeedPower(9000.0f, 3500.0f), 1.0f);
	EXPECT_FLOAT_EQ(SeedPower(0.0f, 0.0f), 1.0f);
	// Held for longer than its delay of a second and a half
	EXPECT_FALSE(SeedReadyAfter(15, k_TurnSeconds, 1.5f));
	EXPECT_TRUE(SeedReadyAfter(16, k_TurnSeconds, 1.5f));
}

// Dispensers

TEST(DispenserRules, ADispenserWaitsForItsBubbleToBeTakenThenMakesAnotherAfterItsPeriod)
{
	DispenserTimer timer {.tick = 0, .period = 300, .active = true};
	// Its bubble is there: it waits
	for (int turn = 0; turn < 1000; ++turn)
	{
		ASSERT_EQ(StepDispenser(timer, true, true, true), DispenserStep::Wait);
	}
	EXPECT_EQ(StepDispenser(timer, true, false, true), DispenserStep::OrbTaken);
	EXPECT_EQ(timer.tick, 0u);
	// Then 300 turns, 30 seconds, to the next
	for (int turn = 1; turn < 300; ++turn)
	{
		ASSERT_EQ(StepDispenser(timer, false, false, true), DispenserStep::Wait) << turn;
	}
	EXPECT_EQ(StepDispenser(timer, false, false, true), DispenserStep::MakeOrb);
	EXPECT_EQ(timer.tick, 0u);
}

TEST(DispenserRules, AnInactiveDispenserOrOneWithoutAMiracleMakesNothing)
{
	DispenserTimer inactive {.tick = 0, .period = 1, .active = false};
	DispenserTimer active {.tick = 0, .period = 1, .active = true};
	for (int turn = 0; turn < 10; ++turn)
	{
		EXPECT_EQ(StepDispenser(inactive, false, false, true), DispenserStep::Wait);
		EXPECT_EQ(StepDispenser(active, false, false, false), DispenserStep::Wait);
	}
	EXPECT_EQ(PeriodTurns(30.0f, std::chrono::milliseconds(100)), 300u);
	EXPECT_EQ(PeriodTurns(0.0f, std::chrono::milliseconds(100)), 0u);
}

TEST(DispenserRules, TheBubbleFloatsAboveItsDispenserAndFacesTheCamera)
{
	const auto orb = OrbPosition({10.0f, 2.0f, 20.0f}, 5.0f);
	EXPECT_NEAR(orb.y, 2.0f + 6.0f, k_Epsilon);
	EXPECT_TRUE(OrbStillThere({10.3f, 99.0f, 20.0f}, orb));
	EXPECT_FALSE(OrbStillThere({11.0f, 8.0f, 20.0f}, orb));
	for (const auto direction : {glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 2.0f, -3.0f), glm::vec3(0.0f, -1.0f, 0.0f)})
	{
		const auto turn = FaceTowards(direction);
		const auto up = turn * glm::vec3(0.0f, 1.0f, 0.0f);
		EXPECT_NEAR(glm::dot(up, glm::normalize(direction)), 1.0f, k_Epsilon);
		EXPECT_NEAR(glm::determinant(turn), 1.0f, k_Epsilon);
	}
}

// Where miracles have been

TEST(SpellGrid, ASquareFadesAfterAMiracleMarksIt)
{
	SpellGrid grid;
	grid.Mark({100.0f, 100.0f});
	EXPECT_EQ(grid.At({150.0f, 120.0f}), SpellGrid::k_Full);
	EXPECT_EQ(grid.At({200.0f, 100.0f}), 0);
	for (int turn = 0; turn < 7; ++turn)
	{
		grid.Fade();
	}
	EXPECT_EQ(grid.At({100.0f, 100.0f}), SpellGrid::k_Full - 7 * SpellGrid::k_FadePerTurn);
	grid.Fade();
	EXPECT_EQ(grid.At({100.0f, 100.0f}), 0);
	EXPECT_FALSE(SpellGrid::CellOf({-1.0f, 0.0f}).has_value());
	EXPECT_FALSE(SpellGrid::CellOf({SpellGrid::k_Side * SpellGrid::k_CellSize, 0.0f}).has_value());
}

// What the miracles do

TEST(SpellBehaviours, AnEventPaysItsCostAndActsOnItsTargetByTheMiraclesStrength)
{
	auto info = MakeTables();
	auto& bolt = Effect(*info, MagicType::LightningBolt);
	bolt.initialChants = 5000.0f;
	bolt.costPerEvent = 2.0f;
	bolt.costPerGameTurn = 50.0f;
	bolt.effectBurn = 800.0f;
	bolt.effectHit = 0.002f;
	bolt.radius = 1.0f;
	FakeServices services(*info);
	services.tribalPower = 1.5f;
	auto spell = MakeSpell(MagicType::LightningBolt, 5000.0f);
	const auto creature = static_cast<entt::entity>(7);
	services.world.objects[creature] = {100.0f, 0.0f, 120.0f};
	ASSERT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Object, {100.0f, 9.0f, 120.0f}, creature)));
	EXPECT_FLOAT_EQ(spell.chants.chants, 4998.0f);
	ASSERT_EQ(services.world.applied.size(), 1u);
	EXPECT_EQ(services.world.applied[0].first, creature);
	// Full strength, which carries the tribal power, times the tribal power once more
	EXPECT_NEAR(services.world.applied[0].second[EffectKind::Burn], 800.0f * 1.5f * 1.5f, 0.01f);
	// Only an event at an object acts on it alone; any other acts round its point, on the land under it
	ASSERT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Landed, {100.0f, 9.0f, 120.0f}, creature)));
	ASSERT_EQ(services.world.appliedAt.size(), 1u);
	EXPECT_FLOAT_EQ(services.world.appliedAt[0].first.y, 0.0f);
	services.world.appliedAt.clear();
	// From the player who cast it
	EXPECT_EQ(services.world.lastSource.player, PlayerNames::PLAYER_ONE);
	EXPECT_EQ(services.world.lastSource.casterCreature, entt::entity {entt::null});
	EXPECT_NEAR(spell.position.z, 120.0f, k_Epsilon);
	// Without a target it acts round the point
	ASSERT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Landed, {90.0f, 2.0f, 90.0f})));
	ASSERT_EQ(services.world.appliedAt.size(), 1u);
	// A closed miracle acts no more
	spell.closedDown = true;
	EXPECT_FALSE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Landed, {90.0f, 2.0f, 90.0f})));
}

TEST(SpellBehaviours, TheStartOfTheEffectIsNotAnEventToActOn)
{
	auto info = MakeTables();
	FakeServices services(*info);
	auto spell = MakeSpell(MagicType::LightningBolt, 100.0f);
	EXPECT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Started, {})));
	EXPECT_TRUE(services.world.applied.empty() && services.world.appliedAt.empty());
	EXPECT_FLOAT_EQ(spell.chants.chants, 100.0f);
}

TEST(SpellBehaviours, AMiracleWithoutACasterHasNoStrengthAndDoesNothing)
{
	auto info = MakeTables();
	Effect(*info, MagicType::Heal).effectHeal = 1.0f;
	FakeServices services(*info);
	auto spell = MakeSpell(MagicType::Heal, 5000.0f);
	spell.caster.kind = SpellCaster::Kind::None;
	EXPECT_FLOAT_EQ(spells::StrengthOf(services, spell), 0.0f);
	EXPECT_FALSE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Point, {1.0f, 0.0f, 1.0f})));
}

TEST(SpellBehaviours, AFireballsStepMovesItButBurnsNothingByItself)
{
	auto info = MakeTables();
	Effect(*info, MagicType::Fireball).initialChants = 1000.0f;
	Effect(*info, MagicType::Fireball).radius = 5.0f;
	FakeServices services(*info);
	auto spell = MakeSpell(MagicType::Fireball, 1000.0f);
	ASSERT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Point, {50.0f, 20.0f, 60.0f})));
	EXPECT_FLOAT_EQ(spell.position.x, 50.0f);
	// Its zero effect values reach what is round it, doing nothing: its ball's own fire does the burning
	ASSERT_EQ(services.world.appliedAt.size(), 1u);
	EXPECT_FLOAT_EQ(services.world.appliedAt[0].second[EffectKind::Burn], 0.0f);
}

TEST(SpellBehaviours, EveryFireballBurnsAsTheFirstRow)
{
	auto info = MakeTables();
	info->magicFireBall.at(0).initialTemperature = 6000.0f;
	info->magicFireBall.at(1).initialTemperature = 10000.0f;
	info->magicFireBall.at(2).initialTemperature = 14000.0f;
	for (const auto type : {MagicType::Fireball, MagicType::FireballPowerUpOne, MagicType::FireballPowerUpTwo})
	{
		EXPECT_FLOAT_EQ(spells::FireballTemperature(*info, type), 6000.0f);
	}
}

TEST(SpellBehaviours, FoodLandsOnDryLandAsPilesAndPaysForEveryGrain)
{
	auto info = MakeTables();
	auto& food = info->magicFood.at(0);
	food.resourceType = ResourceType::Food;
	food.resourceAmountFirstEvent = 200;
	food.resourceAmountPerEvent = 18;
	food.costPerUnit = 7;
	Effect(*info, MagicType::Food).initialChants = 5000.0f;
	FakeServices services(*info);
	services.world.wetBelowX = 50.0f;
	auto spell = MakeSpell(MagicType::Food, 5000.0f);
	spells::Prepare(services, spell);
	ASSERT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Landed, {100.0f, 0.0f, 100.0f})));
	ASSERT_EQ(services.world.resources.size(), 1u);
	EXPECT_EQ(services.world.resources[0].type, ResourceType::Food);
	EXPECT_EQ(services.world.resources[0].amount, 200u);
	EXPECT_FALSE(services.world.resources[0].sparkles);
	EXPECT_FLOAT_EQ(spell.chants.chants, 5000.0f - 1400.0f);
	spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Landed, {101.0f, 0.0f, 100.0f}));
	EXPECT_EQ(services.world.resources.back().amount, 18u);
	// In the water it is paid for but lost
	spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Landed, {10.0f, 0.0f, 100.0f}));
	EXPECT_EQ(services.world.resources.size(), 2u);
	EXPECT_FLOAT_EQ(spell.chants.chants, 5000.0f - 1400.0f - 2.0f * 126.0f);
	EXPECT_TRUE(spells::HasEnoughForRecast(*info, spell));
}

TEST(SpellBehaviours, TheHealGivesItsEffectThePeopleRoundIt)
{
	auto info = MakeTables();
	info->magicHeal.at(0).dummyVar = 10.0f;
	info->magicHeal.at(0).maxToHeal = 2;
	FakeServices services(*info);
	services.tribalPower = 2.0f;
	for (uint32_t i = 1; i <= 6; ++i)
	{
		services.world.objects[static_cast<entt::entity>(i)] = {100.0f, 0.0f, 100.0f};
	}
	auto spell = MakeSpell(MagicType::Heal, 5000.0f);
	ASSERT_TRUE(spells::Start(services, spell));
	// Twice as far and twice as many with twice the tribal power
	EXPECT_EQ(services.targets.size(), 4u);
	EXPECT_FLOAT_EQ(services.world.lastHealRadius, 20.0f);
}

TEST(SpellBehaviours, TheHealCanOnlyBeCastWhereThereIsSomeoneToHeal)
{
	auto info = MakeTables();
	info->magicHeal.at(0).dummyVar = 10.0f;
	info->magicHeal.at(0).maxToHeal = 20;
	FakeServices services(*info);
	EXPECT_FALSE(spells::CanCastAt(services, MagicType::Heal, PlayerNames::PLAYER_ONE, {100.0f, 0.0f, 100.0f}, false));
	services.world.objects[static_cast<entt::entity>(1)] = {100.0f, 0.0f, 100.0f};
	EXPECT_TRUE(spells::CanCastAt(services, MagicType::Heal, PlayerNames::PLAYER_ONE, {100.0f, 0.0f, 100.0f}, false));
}

TEST(SpellBehaviours, TheCastRulesAskForLandAndInfluence)
{
	auto info = MakeTables();
	info->magicGeneral.at(1).castRuleType = CastRuleType::OnLandInInfluence;
	FakeServices services(*info);
	services.world.wetBelowX = 50.0f;
	services.world.influence = false;
	const glm::vec3 land {100.0f, 0.0f, 100.0f};
	EXPECT_FALSE(spells::CanCastAt(services, MagicType::Fireball, PlayerNames::PLAYER_ONE, land, false));
	EXPECT_TRUE(spells::CanCastAt(services, MagicType::Fireball, PlayerNames::PLAYER_ONE, land, true));
	EXPECT_FALSE(spells::CanCastAt(services, MagicType::Fireball, PlayerNames::PLAYER_ONE, {10.0f, 0.0f, 100.0f}, true));
	EXPECT_FALSE(spells::CanCastAt(services, MagicType::Fireball, PlayerNames::PLAYER_ONE, {-5.0f, 0.0f, 100.0f}, true));
	// The creature spells are cast on creatures, never at a point
	EXPECT_FALSE(spells::CanCastAt(services, MagicType::CreatureSpellBig, PlayerNames::PLAYER_ONE, land, true));
}

TEST(SpellBehaviours, LandForACastRuleIsACellWithoutWaterWhateverItsHeight)
{
	auto info = MakeTables();
	info->magicGeneral.at(1).castRuleType = CastRuleType::OnLand;
	FakeServices services(*info);
	services.world.waterBelowZ = 50.0f;
	// Above the sea but with water in its cell: not land
	EXPECT_TRUE(services.world.IsDryLand({100.0f, 0.0f, 10.0f}));
	EXPECT_FALSE(spells::CanCastAt(services, MagicType::Fireball, PlayerNames::PLAYER_ONE, {100.0f, 0.0f, 10.0f}, true));
	EXPECT_TRUE(spells::CanCastAt(services, MagicType::Fireball, PlayerNames::PLAYER_ONE, {100.0f, 0.0f, 100.0f}, true));
	// Food and wood go on land by the same test
	EXPECT_FALSE(spells::CanCastAt(services, MagicType::Food, PlayerNames::PLAYER_ONE, {100.0f, 0.0f, 10.0f}, true));
}

TEST(SpellBehaviours, AShieldStopsAnEventUntilItIsWornDown)
{
	auto info = MakeTables();
	auto& fire = Effect(*info, MagicType::Fireball);
	fire.initialChants = 1000.0f;
	fire.costPerShieldCollide = 600.0f;
	fire.effectHit = 0.5f;
	auto& dome = Effect(*info, MagicType::Shield);
	dome.initialChants = 5000.0f;
	dome.costPerGameTurn = 20.0f;
	info->magicShield.at(0).radiusForNormalCost = 30.0f;
	FakeServices services(*info);
	auto shield = MakeSpell(MagicType::Shield, 5000.0f);
	shield.magnitude = 30.0f;
	const auto shieldEntity = static_cast<entt::entity>(9);
	services.spells[shieldEntity] = &shield;
	services.shield = shieldEntity;
	services.shieldCentre = {100.0f, 0.0f, 100.0f};
	services.shieldRadius = 30.0f;
	auto ball = MakeSpell(MagicType::Fireball, 1000.0f);
	auto event = EventAt(SpellEventInfo::Type::HitSpell, {100.0f, 10.0f, 75.0f}, shieldEntity);
	// The shield pays the ball's strength times its cost per impact and holds
	EXPECT_FALSE(spells::OnEvent(services, ball, event));
	EXPECT_FLOAT_EQ(shield.chants.chants, 4400.0f);
	// A maintained shield's strength is what it has over what it started with
	EXPECT_NEAR(spells::StrengthOf(services, shield), 4400.0f / 5000.0f, k_Epsilon);
	EXPECT_EQ(services.struck, (std::vector<std::pair<entt::entity, bool>> {{shieldEntity, false}}));
	// An event checking for shields inside one strikes itself, as the game has it: it pays for another event and its own
	// cost of striking and is stopped, the shield neither sparking nor losing anything
	auto checked = EventAt(SpellEventInfo::Type::Point, {100.0f, 0.0f, 90.0f});
	checked.checkShields = true;
	const float ballBefore = ball.chants.chants;
	EXPECT_FALSE(spells::OnEvent(services, ball, checked));
	EXPECT_TRUE(services.world.appliedAt.empty());
	EXPECT_FLOAT_EQ(shield.chants.chants, 4400.0f);
	EXPECT_LT(ball.chants.chants, ballBefore);
	EXPECT_TRUE(services.strikes.empty());
	// Worn down to nothing, it lets the next through
	SetChants(shield.chants, 0.0f);
	EXPECT_TRUE(spells::OnEvent(services, ball, event));
}

TEST(SpellBehaviours, AShieldDestroyedByABlowLetsItThroughAndItsPeopleSeeItFall)
{
	auto info = MakeTables();
	auto& blast = Effect(*info, MagicType::ExplosionOne);
	blast.initialChants = 10000.0f;
	blast.costPerShieldCollide = 3000.0f;
	auto& dome = Effect(*info, MagicType::Shield);
	dome.initialChants = 5000.0f;
	FakeServices services(*info);
	auto shield = MakeSpell(MagicType::Shield, 2000.0f);
	const auto shieldEntity = static_cast<entt::entity>(4);
	services.spells[shieldEntity] = &shield;
	auto beam = MakeSpell(MagicType::ExplosionOne, 10000.0f);
	// The beam's 3000 is more than the shield's 2000: it goes through and the shield is destroyed
	EXPECT_TRUE(spells::StrikeSpell(services, beam, shield));
	EXPECT_EQ(services.struck, (std::vector<std::pair<entt::entity, bool>> {{shieldEntity, true}}));
}

TEST(SpellBehaviours, TheForestPlantsOnceAsItsSeedLandsAndPaysForEachTree)
{
	auto info = MakeTables();
	auto& nature = Effect(*info, MagicType::Forest);
	nature.initialChants = 10000.0f;
	nature.costPerEvent = 1.0f;
	nature.costPerGameTurn = 5.0f;
	info->magicForest.at(0).finalNoTrees = 18;
	FakeServices services(*info);
	auto forest = MakeSpell(MagicType::Forest, 10000.0f);
	// Its start is no event; its seed landing plants it, and pays for the event
	EXPECT_TRUE(spells::OnEvent(services, forest, EventAt(SpellEventInfo::Type::Started, forest.castPosition)));
	EXPECT_EQ(services.planted, 0);
	EXPECT_TRUE(spells::OnEvent(services, forest, EventAt(SpellEventInfo::Type::Landed, forest.castPosition)));
	EXPECT_EQ(services.planted, 1);
	EXPECT_FLOAT_EQ(forest.chants.chants, 9999.0f);
	// Planted, it plants no more
	EXPECT_FALSE(spells::OnEvent(services, forest, EventAt(SpellEventInfo::Type::Landed, forest.castPosition)));
	EXPECT_EQ(services.planted, 1);
	// Its upkeep is its own and one event's cost for each tree
	forest.objectCount = 18;
	EXPECT_FLOAT_EQ(spells::CostToMaintain(*info, forest), 23.0f);
	// It lives on while its trees stand
	services.worldObjects = true;
	EXPECT_TRUE(spells::KeptByKind(services, forest));
	services.worldObjects = false;
	EXPECT_FALSE(spells::KeptByKind(services, forest));
}

TEST(SpellBehaviours, AForestIsCastOnlyWhereATreeMayGrow)
{
	auto info = MakeTables();
	info->magicForest.at(0).castRuleType = CastRuleType::OnLandInInfluence;
	FakeServices services(*info);
	EXPECT_TRUE(spells::CanCastAt(services, MagicType::Forest, PlayerNames::PLAYER_ONE, {10.0f, 0.0f, 10.0f}, false));
	services.forestGrows = false;
	EXPECT_FALSE(spells::CanCastAt(services, MagicType::Forest, PlayerNames::PLAYER_ONE, {10.0f, 0.0f, 10.0f}, false));
}

TEST(SpellBehaviours, AShieldsUpkeepGrowsWithItsSize)
{
	auto info = MakeTables();
	Effect(*info, MagicType::Shield).costPerGameTurn = 20.0f;
	info->magicShield.at(0).radiusForNormalCost = 30.0f;
	info->magicShield.at(0).minRadius = 5.0f;
	info->magicShield.at(0).maxRadius = 1000.0f;
	FakeServices services(*info);
	auto shield = MakeSpell(MagicType::Shield, 5000.0f);
	shield.magnitude = 1.0f;
	spells::Prepare(services, shield);
	EXPECT_FLOAT_EQ(shield.magnitude, 5.0f);
	shield.magnitude = 60.0f;
	EXPECT_FLOAT_EQ(spells::CostToMaintain(*info, shield), 80.0f);
}

TEST(SpellBehaviours, AStormIsAsWideAsItsCircleWithinItsLimitsAndCostsByItsArea)
{
	auto info = MakeTables();
	Effect(*info, MagicType::StormWindRainLightning).costPerGameTurn = 25.0f;
	auto& storm = info->magicStormAndTornado.at(1);
	storm.radiusForNormalCost = 40.0f;
	storm.minRadius = 20.0f;
	storm.maxRadius = 1000.0f;
	FakeServices services(*info);
	auto spell = MakeSpell(MagicType::StormWindRainLightning, 5000.0f);
	spell.magnitude = 5.0f;
	spells::Prepare(services, spell);
	EXPECT_FLOAT_EQ(spell.magnitude, 20.0f);
	spell.magnitude = 5000.0f;
	spells::Prepare(services, spell);
	EXPECT_FLOAT_EQ(spell.magnitude, 1000.0f);
	spell.magnitude = 40.0f;
	EXPECT_FLOAT_EQ(spells::CostToMaintain(*info, spell), 25.0f);
	spell.magnitude = 120.0f;
	EXPECT_FLOAT_EQ(spells::CostToMaintain(*info, spell), 225.0f);
	// Its swirl plays where it is cast
	EXPECT_TRUE(spells::Start(services, spell));
	ASSERT_EQ(services.castEffects.size(), 1u);
	EXPECT_EQ(services.castEffects.front(), ParticleType::StormCast);
}

TEST(SpellBehaviours, TheWaterRainsADropEachTurnAndLeavesRingsEveryTenthOfASecond)
{
	auto info = MakeTables();
	Effect(*info, MagicType::Water).initialChants = 5000.0f;
	Effect(*info, MagicType::Water).effectBurn = -4000.0f;
	FakeServices services(*info);
	auto spell = MakeSpell(MagicType::Water, 5000.0f);
	spells::Prepare(services, spell);
	spell.age = 0.2f;
	spells::ProcessTurn(services, spell);
	ASSERT_EQ(services.world.drops.size(), 1u);
	// Half way out in the fake's random numbers: 0.5 of the radius times 0.7 plus 0.3, half way round
	const float distance = glm::distance(glm::vec2(services.world.drops[0].x, services.world.drops[0].z),
	                                     glm::vec2(spell.castPosition.x, spell.castPosition.z));
	EXPECT_NEAR(distance, DropDistance(3.0f), k_Epsilon);
	EXPECT_NEAR(services.world.drops[0].y, 0.2f, k_Epsilon);
	ASSERT_TRUE(services.world.rings[0].has_value());
	EXPECT_FLOAT_EQ(*services.world.rings[0], 2.0f);
	// The drop cools what it lands on
	ASSERT_EQ(services.world.appliedAt.size(), 1u);
	EXPECT_LT(services.world.appliedAt[0].second[EffectKind::Burn], 0.0f);
	EXPECT_FALSE(services.world.extremeDrops[0]);
	// No ring in the same tenth of a second
	spells::ProcessTurn(services, spell);
	EXPECT_FALSE(services.world.rings[1].has_value());
	// Other miracles rain nothing
	auto bolt = MakeSpell(MagicType::LightningBolt, 5000.0f);
	spells::ProcessTurn(services, bolt);
	EXPECT_EQ(services.world.drops.size(), 2u);
}

TEST(SpellBehaviours, AHeldMiraclesPrayerPowerRunsDownWithItsUpkeep)
{
	auto info = MakeTables();
	auto& bolt = Effect(*info, MagicType::LightningBolt);
	bolt.initialChants = 5000.0f;
	bolt.costPerGameTurn = 50.0f;
	FakeServices services(*info);
	auto spell = MakeSpell(MagicType::LightningBolt, 5000.0f);
	const auto rules = spells::RulesOf(services, spell);
	// Its safety level is five seconds of upkeep: full strength above it
	EXPECT_FLOAT_EQ(GetChantSafetyLevel(spell.chants, rules), 2500.0f);
	for (int turn = 0; turn < 50; ++turn)
	{
		PayForOneTurn(spell.chants, rules, services.CasterOf(spell));
	}
	EXPECT_FLOAT_EQ(spells::StrengthOf(services, spell), 1.0f);
	for (int turn = 0; turn < 25; ++turn)
	{
		PayForOneTurn(spell.chants, rules, services.CasterOf(spell));
	}
	// A human player's hand gives nothing more: it weakens below the safety level
	EXPECT_NEAR(spells::StrengthOf(services, spell), 1250.0f / 2500.0f, k_Epsilon);
}

TEST(SpellBehaviours, AnEventMakesTheLivingReactOnceWhenItsTableSaysSo)
{
	auto info = MakeTables();
	auto& bolt = Effect(*info, MagicType::LightningBolt);
	bolt.initialChants = 5000.0f;
	bolt.effectHit = 0.1f;
	bolt.reactionType = Reaction::FleeFromSpell;
	FakeServices services(*info);
	auto spell = MakeSpell(MagicType::LightningBolt, 5000.0f);
	ASSERT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Landed, {90.0f, 0.0f, 90.0f})));
	EXPECT_TRUE(services.reactions.empty());
	bolt.createReactionOnEvent = 1;
	ASSERT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Landed, {90.0f, 0.0f, 90.0f})));
	ASSERT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::Landed, {91.0f, 0.0f, 90.0f})));
	ASSERT_EQ(services.reactions.size(), 1u);
	EXPECT_FALSE(services.reactions[0]);
}

TEST(SpellBehaviours, AMiracleWithoutAnEffectStartsWithoutActing)
{
	auto info = MakeTables();
	Effect(*info, MagicType::FlockFlying).effectHit = 0.5f;
	FakeServices services(*info);
	auto spell = MakeSpell(MagicType::FlockFlying, 5000.0f);
	EXPECT_TRUE(spells::OnEvent(services, spell, EventAt(SpellEventInfo::Type::InitWithoutEffect, {90.0f, 0.0f, 90.0f})));
	EXPECT_TRUE(services.world.appliedAt.empty());
	EXPECT_FALSE(spells::KeptByKind(services, spell));
}
