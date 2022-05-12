/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "InfoConstants.h"

#include "Game.h"

using namespace openblack;

VillagerInfo GVillagerInfo::Find(Tribe tribe, VillagerNumber villagerNumber)
{
	bool found = false;
	auto result = VillagerInfo::None;
	for (size_t i = 0; const auto& villager : Game::instance()->GetInfoConstants().villager)
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

	throw std::runtime_error(std::string("Could not find info for ") + TribeStrs.at(static_cast<size_t>(tribe)).data() + " + " +
	                         VillagerNumberStrs.at(static_cast<size_t>(villagerNumber)).data());
}

AbodeInfo GAbodeInfo::Find(const std::string& name)
{
	for (size_t i = 0; const auto& abode : Game::instance()->GetInfoConstants().abode)
	{
		const auto tribeName = TribeStrs.at(static_cast<uint8_t>(abode.tribeType));
		const auto abodeName = std::string(abode.debugString.data());
		if (std::string(tribeName.data()) + "_" + abodeName == name)
		{
			return static_cast<AbodeInfo>(i);
		}
		++i;
	}

	throw std::runtime_error("Could not find info for " + name);
}

AbodeInfo GAbodeInfo::Find(Tribe tribe, AbodeNumber abodeNumber)
{
	bool found = false;
	auto result = AbodeInfo::None;
	for (size_t i = 0; const auto& abode : Game::instance()->GetInfoConstants().abode)
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

	throw std::runtime_error(std::string("Could not find info for ") + TribeStrs.at(static_cast<size_t>(tribe)).data() + " + " +
	                         AbodeNumberStrs.at(static_cast<size_t>(abodeNumber)).data());
}

FeatureInfo GFeatureInfo::Find(const std::string& name)
{
	for (size_t i = 0; const auto& feature : Game::instance()->GetInfoConstants().feature)
	{
		if (name == feature.debugString.data())
		{
			return static_cast<FeatureInfo>(i);
		}
		++i;
	}
	throw std::runtime_error("Could not find info for " + name);
}

AnimatedStaticInfo GAnimatedStaticInfo::Find(const std::string& name)
{
	for (size_t i = 0; const auto& as : Game::instance()->GetInfoConstants().animatedStatic)
	{
		if (name == as.debugString.data())
		{
			return static_cast<AnimatedStaticInfo>(i);
		}
		++i;
	}
	throw std::runtime_error("Could not find info for " + name);
}
