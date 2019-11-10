/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <Entities/Components/Villager.h>
#include <map>
#include <stdexcept>
#include <exception>

namespace openblack {
	const std::unordered_map<std::string, VillageEthnicities> villageEthnicitiesLookup
	{
			{"AZTEC",    VillageEthnicities::AZTEC},
			{"CELTIC",   VillageEthnicities::CELTIC},
			{"EGYPTIAN", VillageEthnicities::EGYPTIAN},
			{"GREEK",    VillageEthnicities::GREEK},
			{"INDIAN",   VillageEthnicities::INDIAN},
			{"JAPANESE", VillageEthnicities::JAPANESE},
			{"NORSE",    VillageEthnicities::NORSE},
			{"TIBETAN",  VillageEthnicities::TIBETAN},
			{"AFRICAN",  VillageEthnicities::AFRICAN}
	};

	const std::unordered_map<std::string, VillagerRoles> villagerTypesLookup
	{
			{"HOUSEWIFE",  VillagerRoles::HOUSEWIFE}, // This is how the game decides male or female villagers lol
			{"FARMER",     VillagerRoles::FARMER},
			{"FISHERMAN",  VillagerRoles::FISHERMAN},
			{"FORESTER",   VillagerRoles::FORESTER},
			{"BREEDER",    VillagerRoles::BREEDER},
			{"SHEPHERD",   VillagerRoles::SHEPHERD},
			{"MISSIONARY", VillagerRoles::MISSIONARY},
			{"LEADER",     VillagerRoles::LEADER},
			{"TRADER",     VillagerRoles::TRADER}
	};

	std::tuple<VillageEthnicities, VillagerRoles> Villager::GetVillagerEthnicityAndRole(const std::string &villagerEthnicityWithType)
	{
		const auto pos = villagerEthnicityWithType.find_first_of('_');
		const auto ethnicityStr = villagerEthnicityWithType.substr(0, pos);
		const auto typeStr = villagerEthnicityWithType.substr(pos + 1);

		try {
			const auto ethnicity = villageEthnicitiesLookup.at(ethnicityStr);
			const auto type = villagerTypesLookup.at(typeStr);
			return std::make_tuple(ethnicity, type);
		}
		catch (...) {
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

		if (!Villager::IsImportantRole(role)) {
			importantRole = VillagerRoles::NONE;
		}

		VillagerType villagerType = {
			ethnicity,
			lifeStage,
			sex,
			importantRole
		};

		return villagerType;
	}
}
