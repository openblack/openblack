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
	const auto& constants = Game::instance()->GetInfoConstants();
	bool found = false;
	auto result = VillagerInfo::None;
	for (size_t i = 0; i < constants.villager.size(); ++i)
	{
		if (constants.villager[i].tribeType == tribe && constants.villager[i].villagerNumber == villagerNumber)
		{
			result = static_cast<VillagerInfo>(i);
			found = true;
		}
	}
	if (found)
	{
		return result;
	}

	throw std::runtime_error(std::string("Could not find info for ") + TribeStrs[static_cast<size_t>(tribe)].data() + " + " +
	                         VillagerNumberStrs[static_cast<size_t>(villagerNumber)].data());
}

AbodeInfo GAbodeInfo::Find(const std::string& name)
{
	const auto& constants = Game::instance()->GetInfoConstants();
	for (size_t i = 0; i < constants.abode.size(); ++i)
	{
		auto tribeName = TribeStrs[static_cast<uint8_t>(constants.abode[i].tribeType)];
		auto abodeName = std::string(constants.abode[i].debugString.data());
		if (std::string(tribeName.data()) + "_" + abodeName == name)
		{
			return static_cast<AbodeInfo>(i);
		}
	}

	throw std::runtime_error("Could not find info for " + name);
}

AbodeInfo GAbodeInfo::Find(Tribe tribe, AbodeNumber abodeNumber)
{
	auto& constants = Game::instance()->GetInfoConstants();
	bool found = false;
	auto result = AbodeInfo::None;
	for (size_t i = 0; i < constants.abode.size(); ++i)
	{
		if ((constants.abode[i].tribeType == tribe || constants.abode[i].tribeType == Tribe::NONE) &&
		    constants.abode[i].abodeNumber == abodeNumber)
		{
			result = static_cast<AbodeInfo>(i);
			found = true;
		}
	}
	if (found)
	{
		return result;
	}

	throw std::runtime_error(std::string("Could not find info for ") + TribeStrs[static_cast<size_t>(tribe)].data() + " + " +
	                         AbodeNumberStrs[static_cast<size_t>(abodeNumber)].data());
}

FeatureInfo GFeatureInfo::Find(const std::string& name)
{
	auto& constants = Game::instance()->GetInfoConstants();
	for (size_t i = 0; i < constants.feature.size(); ++i)
	{
		if (name == constants.feature[i].debugString.data())
		{
			return static_cast<FeatureInfo>(i);
		}
	}
	throw std::runtime_error("Could not find info for " + name);
}

AnimatedStaticInfo GAnimatedStaticInfo::Find(const std::string& name)
{
	auto& constants = Game::instance()->GetInfoConstants();
	for (size_t i = 0; i < constants.animatedStatic.size(); ++i)
	{
		if (name == constants.animatedStatic[i].debugString.data())
		{
			return static_cast<AnimatedStaticInfo>(i);
		}
	}
	throw std::runtime_error("Could not find info for " + name);
}
