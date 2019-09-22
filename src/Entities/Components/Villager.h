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

#include <cstdint>
#include <unordered_map>
#include "Entities/Entities.h"

enum class VillageEthnicities
{
	GLOBAL,
	AZTEC,
	CELTIC,
	EGYPTIAN,
	GREEK,
	INDIAN,
	JAPANESE,
	NORSE,
	TIBETAN,
	AFRICAN
};

const std::unordered_map<std::string, VillageEthnicities> VillageEthnicitiesLookup {
	{ "AZTEC", VillageEthnicities::AZTEC },
	{ "CELTIC", VillageEthnicities::CELTIC },
	{ "EGYPTIAN", VillageEthnicities::EGYPTIAN },
	{ "GREEK", VillageEthnicities::GREEK },
	{ "INDIAN", VillageEthnicities::INDIAN },
	{ "JAPANESE", VillageEthnicities::JAPANESE },
	{ "NORSE", VillageEthnicities::NORSE },
	{ "TIBETAN", VillageEthnicities::TIBETAN },
	{ "AFRICAN", VillageEthnicities::AFRICAN },
	{ "GLOBAL", VillageEthnicities::GLOBAL }
};

enum class VillagerTypes
{
	HOUSEWIFE,
	FARMER,
	FISHERMAN,
	FORESTER,
	BREEDER,
	SHEPHERD,
	MISSIONARY,
	LEADER, // No idea what a leader is but they are spawned in land 2 and so on
	TRADER  // Exists on one of the MP land scripts
};

const std::unordered_map<std::string, VillagerTypes> VillagerTypesLookup {
	{ "HOUSEWIFE", VillagerTypes::HOUSEWIFE }, // This is how the game decides male or female villagers lol
	{ "FARMER", VillagerTypes::FARMER },
	{ "FISHERMAN", VillagerTypes::FISHERMAN },
	{ "FORESTER", VillagerTypes::FORESTER },
	{ "BREEDER", VillagerTypes::BREEDER },
	{ "SHEPHERD", VillagerTypes::SHEPHERD },
	{ "MISSIONARY", VillagerTypes::MISSIONARY },
	{ "LEADER", VillagerTypes::LEADER },
	{ "TRADER", VillagerTypes::TRADER }
};

enum class VillagerTasks
{
	IDLE
};

enum class VillagerSex
{
	MALE,
	FEMALE
};

struct Villager
{
	uint32_t health;
	uint32_t age;
	uint32_t hunger;
	VillagerSex sex;
	VillageEthnicities ethnicity;
	VillagerTypes type;
	VillagerTasks task;
};
