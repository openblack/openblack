/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Villager.h"

#include "AllMeshes.h"
#include "Enums.h"

#include <unordered_map>

namespace openblack
{
const std::unordered_map<std::string, VillageEthnicities> villageEthnicitiesLookup {
    {"AZTEC", VillageEthnicities::AZTEC}, {"CELTIC", VillageEthnicities::CELTIC},   {"EGYPTIAN", VillageEthnicities::EGYPTIAN},
    {"GREEK", VillageEthnicities::GREEK}, {"INDIAN", VillageEthnicities::INDIAN},   {"JAPANESE", VillageEthnicities::JAPANESE},
    {"NORSE", VillageEthnicities::NORSE}, {"TIBETAN", VillageEthnicities::TIBETAN}, {"AFRICAN", VillageEthnicities::AFRICAN}};

const std::unordered_map<std::string, VillagerRoles> villagerTypesLookup {
    {"HOUSEWIFE", VillagerRoles::HOUSEWIFE}, // This is how the game decides male or female villagers lol
    {"FARMER", VillagerRoles::FARMER},       {"FISHERMAN", VillagerRoles::FISHERMAN}, {"FORESTER", VillagerRoles::FORESTER},
    {"BREEDER", VillagerRoles::BREEDER},     {"SHEPHERD", VillagerRoles::SHEPHERD},   {"MISSIONARY", VillagerRoles::MISSIONARY},
    {"LEADER", VillagerRoles::LEADER},       {"TRADER", VillagerRoles::TRADER}};

std::tuple<VillageEthnicities, VillagerRoles>
Villager::GetVillagerEthnicityAndRole(const std::string& villagerEthnicityWithType)
{
	const auto pos = villagerEthnicityWithType.find_first_of('_');
	const auto ethnicityStr = villagerEthnicityWithType.substr(0, pos);
	const auto typeStr = villagerEthnicityWithType.substr(pos + 1);

	try
	{
		const auto ethnicity = villageEthnicitiesLookup.at(ethnicityStr);
		const auto type = villagerTypesLookup.at(typeStr);
		return std::make_tuple(ethnicity, type);
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("Could not recognize either villager ethnicity or type"));
	}
}

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

VillagerType Villager::GetVillagerType() const
{
	auto importantRole = role;

	if (!Villager::IsImportantRole(role))
	{
		importantRole = VillagerRoles::NONE;
	}

	VillagerType villagerType = {ethnicity, lifeStage, sex, importantRole};

	return villagerType;
}
} // namespace openblack
