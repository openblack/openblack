/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "InfoConstants.h"

#include <cctype>
#include <cstring>

#include <algorithm>
#include <string_view>

#include <spdlog/spdlog.h>

#include "Locator.h"

using namespace openblack;

namespace
{
/// _stricmp
bool EqualNoCase(std::string_view a, std::string_view b)
{
	return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
		       return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
	       });
}
} // namespace

VillagerInfo GVillagerInfo::Find(Tribe tribe, VillagerNumber villagerNumber)
{
	bool found = false;
	auto result = VillagerInfo::None;
	// TODO (#749) use std::views::enumerate
	for (size_t i = 0; const auto& villager : Locator::infoConstants::value().villager)
	{
		if (villager.tribeType == tribe && villager.villagerNumber == villagerNumber)
		{
			result = static_cast<VillagerInfo>(i);
			found = true;
		}
		++i;
	}
	if (found)
	{
		return result;
	}

	throw std::runtime_error(std::string("Could not find info for ") + k_TribeStrs.at(static_cast<size_t>(tribe)).data() +
	                         " + " + k_VillagerNumberStrs.at(static_cast<size_t>(villagerNumber)).data());
}

AbodeInfo GAbodeInfo::Find(const std::string& name)
{
	// GAbodeInfo::GetInfoFromText 0x405A70: "<tribe>_<abode>", the tribe prefix with _strnicmp and the abode's
	// description with _stricmp, the first match; -1 if none. The original then uses the info without a check;
	// here the miss is logged and the command skipped.
	// TODO (#749) use std::views::enumerate
	for (size_t i = 0; const auto& abode : Locator::infoConstants::value().abode)
	{
		if (abode.tribeType != Tribe::NONE && static_cast<size_t>(abode.tribeType) < k_TribeStrs.size())
		{
			const auto tribeName = k_TribeStrs.at(static_cast<size_t>(abode.tribeType));
			const std::string_view text(name);
			if (text.size() > tribeName.size() && text[tribeName.size()] == '_' &&
			    EqualNoCase(text.substr(0, tribeName.size()), tribeName) &&
			    EqualNoCase(text.substr(tribeName.size() + 1), abode.debugString.data()))
			{
				return static_cast<AbodeInfo>(i);
			}
		}
		++i;
	}
	SPDLOG_LOGGER_WARN(spdlog::get("scripting"), "No abode info named \"{}\"; the command is skipped", name);
	return AbodeInfo::None;
}

AbodeInfo GAbodeInfo::Find(Tribe tribe, AbodeNumber abodeNumber)
{
	bool found = false;
	auto result = AbodeInfo::None;
	// TODO (#749) use std::views::enumerate
	for (size_t i = 0; const auto& abode : Locator::infoConstants::value().abode)
	{
		if ((abode.tribeType == tribe || abode.tribeType == Tribe::NONE) && abode.abodeNumber == abodeNumber)
		{
			result = static_cast<AbodeInfo>(i);
			found = true;
		}
		++i;
	}
	if (found)
	{
		return result;
	}

	throw std::runtime_error(std::string("Could not find info for ") + k_TribeStrs.at(static_cast<size_t>(tribe)).data() +
	                         " + " + k_AbodeNumberStrs.at(static_cast<size_t>(abodeNumber)).data());
}

FeatureInfo GFeatureInfo::Find(const std::string& name)
{
	// TODO (#749) use std::views::enumerate
	for (size_t i = 0; const auto& feature : Locator::infoConstants::value().feature)
	{
		// fn_00527740: _stricmp with the description; the count (0x4C) if none, then used without a check (here
		// logged and the command skipped)
		if (EqualNoCase(name, feature.debugString.data()))
		{
			return static_cast<FeatureInfo>(i);
		}
		++i;
	}
	SPDLOG_LOGGER_WARN(spdlog::get("scripting"), "No feature info named \"{}\"; the command is skipped", name);
	return FeatureInfo::None;
}

AnimatedStaticInfo GAnimatedStaticInfo::Find(const std::string& name)
{
	// TODO (#749) use std::views::enumerate
	for (size_t i = 0; const auto& as : Locator::infoConstants::value().animatedStatic)
	{
		// fn_00422600: _stricmp with the description; the count (0x10) if none, then used without a check (here
		// logged and the command skipped)
		if (EqualNoCase(name, as.debugString.data()))
		{
			return static_cast<AnimatedStaticInfo>(i);
		}
		++i;
	}
	SPDLOG_LOGGER_WARN(spdlog::get("scripting"), "No animated static info named \"{}\"; the command is skipped", name);
	return AnimatedStaticInfo::None;
}

void openblack::UpdateInfo(CreatureActionInfo& info, const v100::CreatureActionInfo& old)
{
	memcpy(&info, &old, sizeof(old));
	// TODO(#684): Figure out what field0xfc is and what value it should hold
	info.field0xfc = 0;
}

void openblack::UpdateInfo(InfoConstants& info, const v100::InfoConstants& old)
{
	info.magicGeneral = old.magicGeneral;
	info.magicHeal = old.magicHeal;
	info.magicTeleport = old.magicTeleport;
	info.magicForest = old.magicForest;
	info.magicFood = old.magicFood;
	info.magicStormAndTornado = old.magicStormAndTornado;
	info.magicShield = old.magicShield;
	info.magicWood = old.magicWood;
	info.magicWater = old.magicWater;
	info.magicFlockFlying = old.magicFlockFlying;
	info.magicFlockGround = old.magicFlockGround;
	info.magicCreatureSpell = old.magicCreatureSpell;
	info.magicEffect = old.magicEffect;
	info.spellSeed = old.spellSeed;
	info.animal = old.animal;
	info.creature = old.creature;
	info.differentCreature = old.differentCreature;
	info.creatureDesireForType = old.creatureDesireForType;
	info.creatureDevelopmentPhaseEntry = old.creatureDevelopmentPhaseEntry;
	info.creatureDevelopmentDurationEntry = old.creatureDevelopmentDurationEntry;
	info.citadel = old.citadel;
	info.citadelHeart = old.citadelHeart;
	info.creaturePen = old.creaturePen;
	info.worshipSite = old.worshipSite;
	info.spellIcon = old.spellIcon;
	info.abode = old.abode;
	info.villager = old.villager;
	info.specialVillager = old.specialVillager;
	info.tree = old.tree;
	info.singleMapFixed = old.singleMapFixed;
	info.scriptHighlight = old.scriptHighlight;
	info.mapShield = old.mapShield;
	info.ball = old.ball;
	info.town = old.town;
	info.job = old.job;
	info.feature = old.feature;
	info.flowers = old.flowers;
	info.animatedStatic = old.animatedStatic;
	info.mobileObject = old.mobileObject;
	info.scaffold = old.scaffold;
	info.mobileStatic = old.mobileStatic;
	info.pot = old.pot;
	info.prayerIcon = old.prayerIcon;
	info.prayerSite = old.prayerSite;
	info.showNeeds = old.showNeeds;
	info.townDesire = old.townDesire;
	info.worshipSiteUpgrade = old.worshipSiteUpgrade;
	info.helpSpirit = old.helpSpirit;
	info.arrow = old.arrow;
	info.spotVisual = old.spotVisual;
	info.effect = old.effect;
	info.field = old.field;
	info.fieldType = old.fieldType;
	info.fishFarm = old.fishFarm;
	info.footballPosition = old.footballPosition;
	info.playtime = old.playtime;
	info.player = old.player;
	info.sound = old.sound;
	info.belief = old.belief;
	info.helpSpritesGuidance = old.helpSpritesGuidance;
	info.influence = old.influence;
	info.helpSystem = old.helpSystem;
	info.alignment = old.alignment;
	info.reaction = old.reaction;
	for (size_t i = 0; i < old.creatureAction.size(); ++i)
	{
		UpdateInfo(info.creatureAction.at(i), old.creatureAction.at(i));
	}
	info.creatureDesireAction1 = old.creatureDesireAction1;
	info.creatureDesireAction2 = old.creatureDesireAction2;
	info.creatureDesireAction3 = old.creatureDesireAction3;
	info.creatureDesireDependency = old.creatureDesireDependency;
	info.creatureInitialDesire = old.creatureInitialDesire;
	info.desireSourceTable = old.desireSourceTable;
	info.creatureInitialSource1 = old.creatureInitialSource1;
	info.creatureInitialSource2 = old.creatureInitialSource2;
	info.creatureSourceBounds = old.creatureSourceBounds;
	info.creatureDesireAttributeEntry = old.creatureDesireAttributeEntry;
	info.creatireActionKnownAboutEntry = old.creatireActionKnownAboutEntry;
	info.creatureMagicActionKnownAboutEntry = old.creatureMagicActionKnownAboutEntry;
	info.creatureMimic = old.creatureMimic;
	info.terrainMaterial = old.terrainMaterial;
	info.tribe = old.tribe;
	info.speedThreshold = old.speedThreshold;
	info.pBall = old.pBall;
	info.pFootball = old.pFootball;
	info.pFootballPosition = old.pFootballPosition;
	info.dance = old.dance;
	info.villagerStateTable = old.villagerStateTable;
	info.animalStateTable = old.animalStateTable;
	info.totemStatue = old.totemStatue;
	info.bigForest = old.bigForest;
	info.furniture = old.furniture;
	info.weather = old.weather;
	info.climate = old.climate;
	info.spellSystem = old.spellSystem;
	info.vortex = old.vortex;
	info.magicFireBall = old.magicFireBall;
	info.reward = old.reward;
	info.leashSelector = old.leashSelector;
	info.rewardProgressGood = old.rewardProgressGood;
	info.rewardProgressEvil = old.rewardProgressEvil;
	info.spookyVoice = old.spookyVoice;
	info.scriptOpposingCreature = old.scriptOpposingCreature;
	info.toolTips = old.toolTips;
}
