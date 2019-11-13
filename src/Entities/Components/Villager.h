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
#include <string>
#include <tuple>

namespace openblack
{
enum class VillageEthnicities
{
	NONE,
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

enum class VillagerRoles
{
	NONE,
	HOUSEWIFE,
	FARMER,
	FISHERMAN,
	FORESTER,
	BREEDER,
	SHEPHERD,
	MISSIONARY,
	LEADER, // No idea what a leader is but they are spawned in land 2 and so on
	TRADER, // Exists on one of the MP land scripts
	PiedPiper,
	Shaolin,
	IdolBuilder,
	Hermit,
	Hippy,
	Priest,
	Priestess,
	Marauder,
	Footballer_1,
	Footballer_2,
	Engineer,
	Shepered,
	Nomade,
	AztecLeader,
	CreatureTrainer,
	NorseSailor,
	Breeder,
	Healer,
	Sculptor,
	Crusader,
	SailorAccordian
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

enum class VillagerLifeStage
{
	Child,
	Adult
};

using VillagerType = std::tuple<VillageEthnicities, VillagerLifeStage, VillagerSex, VillagerRoles>;

struct Villager
{
	uint32_t health;
	uint32_t age;
	uint32_t hunger;
	VillagerLifeStage lifeStage;
	VillagerSex sex;
	VillageEthnicities ethnicity;
	VillagerRoles role;
	VillagerTasks task;

	static bool IsImportantRole(VillagerRoles role);
	static std::tuple<VillageEthnicities, VillagerRoles>
	GetVillagerEthnicityAndRole(const std::string& villagerEthnicityWithType);
	VillagerType GetVillagerType() const;
};

struct VillagerTypeId
{
	std::size_t operator()(const VillagerType& type) const
	{
		auto ethnicity = std::get<VillageEthnicities>(type);
		auto villagerSex = std::get<VillagerSex>(type);
		auto lifeStage = std::get<VillagerLifeStage>(type);
		auto role = std::get<VillagerRoles>(type);
		auto h1 = std::hash<decltype(ethnicity)>()(ethnicity);
		auto h2 = std::hash<decltype(villagerSex)>()(villagerSex);
		auto h3 = std::hash<decltype(lifeStage)>()(lifeStage);

		if (!Villager::IsImportantRole(role))
		{
			role = VillagerRoles::NONE;
		}

		auto h4 = std::hash<decltype(role)>()(role);
		return h1 ^ h2 ^ h3 ^ h4;
	}
};
} // namespace openblack
