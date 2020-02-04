/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

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
