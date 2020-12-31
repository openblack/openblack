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

namespace
{
const auto villagerEthnicityLookup = makeLookup<Villager::Ethnicity>(Villager::EthnicityStrs);
const auto villagerRoleLookup = makeLookup<Villager::Role>(Villager::RoleStrs);
} // namespace

std::tuple<Villager::Ethnicity, Villager::Role>
Villager::GetVillagerEthnicityAndRole(const std::string& villagerEthnicityWithType)
{
	const auto pos = villagerEthnicityWithType.find_first_of('_');
	const auto ethnicityStr = villagerEthnicityWithType.substr(0, pos);
	const auto roleStr = villagerEthnicityWithType.substr(pos + 1);

	try
	{
		const auto ethnicity = villagerEthnicityLookup.at(ethnicityStr);
		const auto role = villagerRoleLookup.at(roleStr);
		return std::make_tuple(ethnicity, role);
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("Could not recognize either villager ethnicity or type"));
	}
}

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

	Villager::Type villagerType = {ethnicity, lifeStage, sex, importantRole};

	return villagerType;
}
