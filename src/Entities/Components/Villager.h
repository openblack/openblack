/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

#include "Enums.h"

namespace openblack::entities::components
{

struct Villager
{
	/// Originally VillagerRoles
	enum class Role : uint8_t
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
		SailorAccordian,

		_COUNT
	};
	static constexpr std::array<std::string_view, static_cast<uint8_t>(Role::_COUNT)> RoleStrs = {
	    "NONE",            //
	    "HOUSEWIFE",       //
	    "FARMER",          //
	    "FISHERMAN",       //
	    "FORESTER",        //
	    "BREEDER",         //
	    "SHEPHERD",        //
	    "MISSIONARY",      //
	    "LEADER",          //
	    "TRADER",          //
	    "PiedPiper",       //
	    "Shaolin",         //
	    "IdolBuilder",     //
	    "Hermit",          //
	    "Hippy",           //
	    "Priest",          //
	    "Priestess",       //
	    "Marauder",        //
	    "Footballer_1",    //
	    "Footballer_2",    //
	    "Engineer",        //
	    "Shepered",        //
	    "Nomade",          //
	    "AztecLeader",     //
	    "CreatureTrainer", //
	    "NorseSailor",     //
	    "Breeder",         //
	    "Healer",          //
	    "Sculptor",        //
	    "Crusader",        //
	    "SailorAccordian"  //
	};

	/// Originally VillagerTasks
	enum class Task : uint8_t
	{
		IDLE,

		_COUNT
	};
	static constexpr std::array<std::string_view, static_cast<uint8_t>(Task::_COUNT)> TaskStrs = {
	    "IDLE", //
	};

	/// Originally VillagerSex
	enum class Sex : uint8_t
	{
		MALE,
		FEMALE,

		_COUNT
	};
	static constexpr std::array<std::string_view, static_cast<uint8_t>(Sex::_COUNT)> SexStrs = {
	    "MALE",   //
	    "FEMALE", //
	};

	/// Originally VillagerLifeStage
	enum class LifeStage : uint8_t
	{
		Child,
		Adult,

		_COUNT
	};
	static constexpr std::array<std::string_view, static_cast<uint8_t>(LifeStage::_COUNT)> LifeStageStrs = {
	    "Child", //
	    "Adult", //
	};

	using Type = std::tuple<Tribe, Villager::LifeStage, Villager::Sex, Villager::Role>;

	struct TypeId
	{
		std::size_t operator()(const Villager::Type& type) const
		{
			auto tribe = std::get<Tribe>(type);
			auto villagerSex = std::get<Sex>(type);
			auto lifeStage = std::get<LifeStage>(type);
			auto role = std::get<Role>(type);
			auto h1 = std::hash<decltype(tribe)>()(tribe);
			auto h2 = std::hash<decltype(villagerSex)>()(villagerSex);
			auto h3 = std::hash<decltype(lifeStage)>()(lifeStage);

			if (!Villager::IsImportantRole(role))
			{
				role = Role::NONE;
			}

			auto h4 = std::hash<decltype(role)>()(role);
			return h1 ^ h2 ^ h3 ^ h4;
		}
	};

	uint32_t health;
	uint32_t age;
	uint32_t hunger;
	LifeStage lifeStage;
	Sex sex;
	Tribe tribe;
	Role role;
	Task task;

	static bool IsImportantRole(Role role);
	Type GetVillagerType() const;
};
} // namespace openblack::entities::components
