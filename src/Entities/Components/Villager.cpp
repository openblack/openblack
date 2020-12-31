/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
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
using openblack::entities::components::Villager;

bool Villager::IsImportantRole(Villager::Role role)
{
	switch (role)
	{
	case Role::PiedPiper:
	case Role::Shaolin:
	case Role::IdolBuilder:
	case Role::Hermit:
	case Role::Hippy:
	case Role::Priest:
	case Role::Priestess:
	case Role::Marauder:
	case Role::Footballer_1:
	case Role::Footballer_2:
	case Role::Engineer:
	case Role::Shepered:
	case Role::Nomade:
	case Role::AztecLeader:
	case Role::CreatureTrainer:
	case Role::NorseSailor:
	case Role::Breeder:
	case Role::Healer:
	case Role::Sculptor:
	case Role::Crusader:
	case Role::SailorAccordian:
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
		importantRole = Role::NONE;
	}

	Villager::Type villagerType = {tribe, lifeStage, sex, importantRole};

	return villagerType;
}
