/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstring>

#include <memory>

#include <gtest/gtest.h>

#include "Magic/MagicTables.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
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

void SetName(std::array<char, 0x30>& name, const char* text)
{
	name.fill('\0');
	std::strncpy(name.data(), text, name.size() - 1);
}

/// A fire seed: fireball and its two power-ups, reached by an inverse spiral then a spiral
GSpellSeedInfo& MakeFireSeed(InfoConstants& info)
{
	auto& seed = info.spellSeed.at(static_cast<size_t>(SpellSeedType::Fire));
	seed.magicTypes = {MagicType::Fireball, MagicType::FireballPowerUpOne, MagicType::FireballPowerUpTwo, MagicType::None};
	seed.powerUpGestures = {GestureType::InverseSpiral, GestureType::Spiral, GestureType::None};
	return seed;
}
} // namespace

TEST(MagicTables, sectionOfEachMagicType)
{
	EXPECT_EQ(SlotOf(MagicType::None).section, MagicInfoSection::General);
	EXPECT_EQ(SlotOf(MagicType::ExplosionOnePuTwo).index, 9);
	EXPECT_EQ(SlotOf(MagicType::HealPowerUpOne).section, MagicInfoSection::Heal);
	EXPECT_EQ(SlotOf(MagicType::HealPowerUpOne).index, 1);
	EXPECT_EQ(SlotOf(MagicType::Teleport).section, MagicInfoSection::Teleport);
	EXPECT_EQ(SlotOf(MagicType::Forest).section, MagicInfoSection::Forest);
	EXPECT_EQ(SlotOf(MagicType::FoodPowerUpOne).section, MagicInfoSection::Food);
	EXPECT_EQ(SlotOf(MagicType::Tornado).section, MagicInfoSection::StormAndTornado);
	EXPECT_EQ(SlotOf(MagicType::Tornado).index, 2);
	EXPECT_EQ(SlotOf(MagicType::PhysicalShield).section, MagicInfoSection::Shield);
	EXPECT_EQ(SlotOf(MagicType::Wood).section, MagicInfoSection::Wood);
	EXPECT_EQ(SlotOf(MagicType::WaterPowerUpOne).section, MagicInfoSection::Water);
	EXPECT_EQ(SlotOf(MagicType::FlockFlying).section, MagicInfoSection::FlockFlying);
	EXPECT_EQ(SlotOf(MagicType::FlockGround).section, MagicInfoSection::FlockGround);
	EXPECT_EQ(SlotOf(MagicType::CreatureSpellFreeze).index, 0);
	EXPECT_EQ(SlotOf(MagicType::CreatureSpellItchy).section, MagicInfoSection::CreatureSpell);
	EXPECT_EQ(SlotOf(MagicType::CreatureSpellItchy).index, 15);
}

TEST(MagicTables, recordsInMagicTypeOrder)
{
	const auto info = MakeTables();
	for (uint32_t type = 0; type < k_MagicTypeCount; ++type)
	{
		EXPECT_EQ(static_cast<uint32_t>(GetMagicInfo(*info, static_cast<MagicType>(type)).magicType), type);
	}
	EXPECT_EQ(GetMagicInfoAs<GMagicHealInfo>(*info, MagicType::Heal), &info->magicHeal[0]);
	EXPECT_EQ(GetMagicInfoAs<GMagicHealInfo>(*info, MagicType::Fireball), nullptr);
	EXPECT_EQ(GetMagicInfoAs<GMagicResourceInfo>(*info, MagicType::Wood), &info->magicWood[0]);
	EXPECT_EQ(GetMagicInfoAs<GMagicResourceInfo>(*info, MagicType::FoodPowerUpOne), &info->magicFood[1]);
	EXPECT_EQ(GetMagicInfoAs<GMagicRadiusSpellInfo>(*info, MagicType::Shield), &info->magicShield[0]);
	EXPECT_EQ(GetMagicInfoAs<GMagicCreatureSpellInfo>(*info, MagicType::CreatureSpellAngry), &info->magicCreatureSpell[9]);
}

TEST(MagicTables, magicTypeByName)
{
	auto info = MakeTables();
	SetName(info->magicEffect[16].debugString, "MAGIC_TYPE_STORM_WIND_RAIN");
	EXPECT_EQ(FindMagicTypeByName(*info, "magic_type_storm_wind_rain"), MagicType::StormWindRain);
	EXPECT_EQ(FindMagicTypeByName(*info, "MAGIC_TYPE_STORM"), std::nullopt);
}

TEST(MagicTables, maintainedSpells)
{
	EXPECT_TRUE(IsMaintainedSpell(MagicType::Forest));
	EXPECT_TRUE(IsMaintainedSpell(MagicType::Shield));
	EXPECT_TRUE(IsMaintainedSpell(MagicType::PhysicalShield));
	EXPECT_FALSE(IsMaintainedSpell(MagicType::Teleport));
	EXPECT_FALSE(IsMaintainedSpell(MagicType::Tornado));
	EXPECT_FALSE(IsMaintainedSpell(MagicType::Wood));
}

TEST(MagicTables, effectGetters)
{
	auto info = MakeTables();
	auto& storm = info->magicEffect[16];
	storm.timerWhenPlayerCasting = 40.0f;
	storm.timerWhenCreatureCasting = 20.0f;
	storm.costToCreate = 3500.0f;
	storm.agressiveRangeMin = 10.0f;
	storm.agressiveRangeMax = 50.0f;
	info->magicStormAndTornado[0].isCreatureCastFromAbove = 1;
	EXPECT_FLOAT_EQ(GetTimerWhenPlayerCasting(*info, MagicType::StormWindRain), 40.0f);
	EXPECT_FLOAT_EQ(GetTimerWhenCreatureCasting(*info, MagicType::StormWindRain), 20.0f);
	EXPECT_FLOAT_EQ(GetChantsRequiredToCreate(*info, MagicType::StormWindRain), 3500.0f);
	EXPECT_TRUE(IsCreatureCastFromAbove(*info, MagicType::StormWindRain));
	EXPECT_FALSE(IsCreatureCastFromAbove(*info, MagicType::Tornado));
	EXPECT_TRUE(IsInAggressiveRange(*info, MagicType::StormWindRain, 10.0f));
	EXPECT_TRUE(IsInAggressiveRange(*info, MagicType::StormWindRain, 50.0f));
	EXPECT_FALSE(IsInAggressiveRange(*info, MagicType::StormWindRain, 50.5f));
}

TEST(MagicTables, tribalPower)
{
	GMagicEffectInfo effect {};
	effect.useTribalPowerMultiplier[0] = 1;
	effect.useTribalPowerMultiplier[2] = 1;
	std::array<float, k_TribeCount> power {};
	power.fill(1.0f);
	EXPECT_FLOAT_EQ(GetTribalPower(effect, power), 1.0f);
	EXPECT_EQ(GetTribalPowerTribe(effect, power), std::nullopt);
	power[1] = 5.0f; // a tribe the miracle does not use
	power[2] = 3.0f;
	power[0] = 2.0f;
	EXPECT_FLOAT_EQ(GetTribalPower(effect, power), 6.0f);
	EXPECT_EQ(GetTribalPowerTribe(effect, power), Tribe::CELTIC);
	power[0] = 100.0f;
	EXPECT_FLOAT_EQ(GetTribalPower(effect, power), 100.0f);
	power[0] = 0.1f;
	EXPECT_FLOAT_EQ(GetTribalPower(effect, power), 0.5f);
	EXPECT_EQ(GetTribalPowerTribe(effect, power), Tribe::AZTEC);
	power[0] = -1.0f;
	EXPECT_FLOAT_EQ(GetTribalPower(effect, power), 0.5f);
}

TEST(MagicTables, powerUpLevels)
{
	auto info = MakeTables();
	const auto& seed = MakeFireSeed(*info);
	EXPECT_EQ(GetPowerUpFromMagicType(seed, MagicType::Fireball), k_BasePowerUpLevel);
	EXPECT_EQ(GetPowerUpFromMagicType(seed, MagicType::FireballPowerUpOne), 0);
	EXPECT_EQ(GetPowerUpFromMagicType(seed, MagicType::FireballPowerUpTwo), 1);
	EXPECT_EQ(GetPowerUpFromMagicType(seed, MagicType::None), 2); // the empty slot matches none, as in the original
	EXPECT_EQ(GetPowerUpFromMagicType(seed, MagicType::Heal), k_BasePowerUpLevel);
	EXPECT_EQ(GetNumPowerUpLevels(seed), 3);
	EXPECT_EQ(GetMagicTypeFromPowerUpLevel(seed, k_BasePowerUpLevel), MagicType::Fireball);
	EXPECT_EQ(GetMagicTypeFromPowerUpLevel(seed, 1), MagicType::FireballPowerUpTwo);
	EXPECT_EQ(&GetMagicInfoFromPowerUpLevel(*info, seed, 0), &info->magicGeneral[2]);
	EXPECT_EQ(&GetMagicInfoFromPowerUpLevel(*info, seed, 2), &info->magicGeneral[1]); // empty: the plain miracle

	const auto two = GetPowerUpGesture(seed, MagicType::FireballPowerUpTwo);
	EXPECT_EQ(two.gesture, GestureType::Spiral);
	EXPECT_EQ(two.level, 1);
	const auto plain = GetPowerUpGesture(seed, MagicType::Fireball);
	EXPECT_EQ(plain.gesture, GestureType::None);
	EXPECT_EQ(plain.level, k_BasePowerUpLevel);
}

TEST(MagicTables, seedLookups)
{
	auto info = MakeTables();
	MakeFireSeed(*info);
	auto& heal = info->spellSeed.at(static_cast<size_t>(SpellSeedType::Heal));
	heal.magicTypes = {MagicType::Heal, MagicType::HealPowerUpOne, MagicType::None, MagicType::None};
	heal.exists = 1;
	heal.iconIndex = 6;
	SetName(heal.debugString, "SPELL_SEED_TYPE_HEAL");

	const auto& fire = GetSpellSeedInfo(*info, SpellSeedType::Fire);
	EXPECT_TRUE(SpellSeedIsOfMagicType(fire, MagicType::FireballPowerUpOne));
	EXPECT_FALSE(SpellSeedIsOfMagicType(fire, MagicType::Heal));
	EXPECT_EQ(FindFirstSpellSeedForMagicType(*info, MagicType::FireballPowerUpTwo), SpellSeedType::Fire);
	EXPECT_EQ(FindFirstSpellSeedForMagicType(*info, MagicType::HealPowerUpOne), SpellSeedType::Heal);
	EXPECT_EQ(FindFirstSpellSeedForMagicType(*info, MagicType::Tornado), std::nullopt);
	EXPECT_EQ(FindSpellSeedByName(*info, "spell_seed_type_heal"), SpellSeedType::Heal);
	EXPECT_EQ(FindSpellSeedByName(*info, "SPELL_SEED_TYPE_FIRE"), std::nullopt);
	EXPECT_EQ(FindSpellSeedByIcon(*info, 6), SpellSeedType::Heal);
	EXPECT_EQ(FindSpellSeedByIcon(*info, 9), std::nullopt);

	const auto one = GetPowerUpGestureForMagicType(*info, MagicType::FireballPowerUpOne);
	EXPECT_EQ(one.gesture, GestureType::InverseSpiral);
	EXPECT_EQ(one.level, 0);
	const auto none = GetPowerUpGestureForMagicType(*info, MagicType::Tornado);
	EXPECT_EQ(none.gesture, GestureType::None);
	EXPECT_EQ(none.level, k_BasePowerUpLevel);
	EXPECT_EQ(GetPowerUpLevel(*info, MagicType::Fireball), k_BasePowerUpLevel);
	EXPECT_EQ(GetPowerUpLevel(*info, MagicType::FireballPowerUpOne), 0);
	EXPECT_EQ(GetPowerUpLevel(*info, MagicType::HealPowerUpOne), 0);
	EXPECT_EQ(GetPowerUpLevel(*info, MagicType::Tornado), k_BasePowerUpLevel);
}
