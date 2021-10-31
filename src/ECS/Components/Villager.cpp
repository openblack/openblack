/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Villager.h"

#include <stdexcept>
#include <string_view>

#include "Common/MeshLookup.h"

using namespace openblack;
using openblack::ecs::components::Villager;

bool Villager::IsImportantRole(VillagerRoles role)
{
	switch (role)
	{
	case VillagerRoles::PiedPiper:
	case VillagerRoles::Shaolin:
	case VillagerRoles::IdolBuilder:
	case VillagerRoles::Hermit:
	case VillagerRoles::Hippy:
	case VillagerRoles::Priest:
	case VillagerRoles::Priestess:
	case VillagerRoles::Marauder:
	case VillagerRoles::Footballer_1:
	case VillagerRoles::Footballer_2:
	case VillagerRoles::Engineer:
	case VillagerRoles::Shepered:
	case VillagerRoles::Nomade:
	case VillagerRoles::AztecLeader:
	case VillagerRoles::CreatureTrainer:
	case VillagerRoles::NorseSailor:
	case VillagerRoles::Breeder:
	case VillagerRoles::Healer:
	case VillagerRoles::Sculptor:
	case VillagerRoles::Crusader:
	case VillagerRoles::SailorAccordian:
		return true;
	default:
		return false;
	}
}

Villager::Type Villager::GetVillagerType() const
{
	auto importantRole = role;

	if (!Villager::IsImportantRole(role))
	{
		importantRole = VillagerRoles::NONE;
	}

	Villager::Type villagerType = {tribe, lifeStage, sex, importantRole};

	return villagerType;
}
