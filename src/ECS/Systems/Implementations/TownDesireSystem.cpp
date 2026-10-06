/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TownDesireSystem.h"

#include <algorithm>
#include <ranges>

#include "3D/DayNightClock.h"
#include "3D/SkyInterface.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/TownDesire.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/TownDesire.h"
#include "InfoConstants.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
namespace town_desire = openblack::ecs::town_desire;

namespace
{
constexpr size_t Index(TownDesireInfo desire)
{
	return static_cast<size_t>(static_cast<int>(desire));
}

bool Valid(TownDesireInfo desire)
{
	return static_cast<int>(desire) >= 0 && Index(desire) < town_desire::k_Count;
}

/// The info of a tribe's kind of abode, if it has one
const GAbodeInfo* FindAbodeInfo(Tribe tribe, AbodeNumber number)
{
	const auto& abodes = Locator::infoConstants::value().abode;
	const auto found = std::ranges::find_if(
	    abodes, [tribe, number](const GAbodeInfo& info) { return info.tribeType == tribe && info.abodeNumber == number; });
	return found != abodes.end() ? &*found : nullptr;
}

/// The info of a tribe's villager of a number
const GVillagerInfo* FindVillagerInfo(Tribe tribe, VillagerNumber number)
{
	const auto& villagers = Locator::infoConstants::value().villager;
	const auto found = std::ranges::find_if(villagers, [tribe, number](const GVillagerInfo& info) {
		return info.tribeType == tribe && info.villagerNumber == number;
	});
	return found != villagers.end() ? &*found : nullptr;
}

/// A town's people and buildings this turn, and what its desires read of them and the world
town_desire::DesireInputs GatherInputs(entt::entity townEntity, const Town& town, Tribe tribe)
{
	auto& registry = Locator::entitiesRegistry::value();
	town_desire::DesireInputs in;
	in.tribe = tribe;
	auto& stats = in.stats;

	registry.Each<const Villager>([&](const Villager& villager) {
		if (villager.town != townEntity)
		{
			return;
		}
		if (villager.lifeStage == Villager::LifeStage::Child)
		{
			++stats.children;
		}
		else
		{
			++stats.adults;
		}
		if (const auto* info = FindVillagerInfo(villager.tribe, villager.number); info != nullptr)
		{
			stats.foodForDinner += static_cast<float>(info->foodReqiredForDinner);
		}
	});

	registry.Each<const Abode>([&](const Abode& abode) {
		if (abode.townId != town.id)
		{
			return;
		}
		++in.abodeCount;
		const auto number = static_cast<size_t>(abode.type);
		if (number < stats.abodesByNumber.size())
		{
			++stats.abodesByNumber.at(number);
		}
		const auto* info = FindAbodeInfo(tribe, abode.type);
		if (info != nullptr && info->maxVillagersInAbode + info->maxChildrenInAbode != 0)
		{
			++stats.abodesWithPlaces;
			stats.adultPlaces += info->maxVillagersInAbode;
			stats.childPlaces += info->maxChildrenInAbode;
			stats.totalPlaces += info->maxVillagersInAbode + info->maxChildrenInAbode;
		}
		// Abodes don't wear down yet, so none wants repairing
		in.abodes.push_back({
		    .life = 1.0f,
		    .livingQuarters = info != nullptr &&
		                      (static_cast<uint32_t>(info->abodeType) & static_cast<uint32_t>(AbodeType::LivingQuarters)) != 0,
		    .inhabitants = static_cast<uint32_t>(abode.inhabitants.size()),
		    .desireToBeRepaired = 0.0f,
		});
		if (abode.type == AbodeNumber::StoragePit)
		{
			in.storageFood = abode.foodAmount;
			in.storageWood = abode.woodAmount;
		}
		if (abode.type == AbodeNumber::Creche)
		{
			in.crecheFunctional = true;
		}
	});

	for (size_t i = 0; i < in.populationWhenNeeded.size(); ++i)
	{
		if (const auto* info = FindAbodeInfo(tribe, static_cast<AbodeNumber>(i)); info != nullptr)
		{
			in.populationWhenNeeded.at(i) = info->populationWhenNeeded;
		}
	}
	in.homeless = static_cast<uint32_t>(town.homelessVillagers.size());

	// The game counts the sky from 2 at night to 0 by day
	if (Locator::skySystem::has_value())
	{
		const auto& clock = Locator::skySystem::value().GetClock();
		in.visualHour = clock.GetVisualTime();
		in.skyType = 2.0f - clock.SkyType(in.visualHour);
		in.dayFull = clock.GetVisualTimes()[3];
	}
	if (Locator::time::has_value())
	{
		in.turn = Locator::time::value().GetTurn();
	}
	return in;
}
} // namespace

void TownDesireSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& info = Locator::infoConstants::value();
	// The game takes a villager's load from its African farmer
	const auto& farmer = info.villager.at(10);
	registry.Each<const Town, const Tribe, TownDesire, TownStats>(
	    [&](entt::entity entity, const Town& town, const Tribe tribe, TownDesire& desire, TownStats& stats) {
		    const auto inputs = GatherInputs(entity, town, tribe);
		    stats = inputs.stats;
		    const town_desire::DesireContext context {
		        .desire = desire,
		        .in = inputs,
		        .town = info.town,
		        .info = info.townDesire,
		        .farmerMaxFood = farmer.maxFoodCarried,
		        .farmerMaxWood = farmer.maxWoodCarried,
		    };
		    town_desire::Process(desire, context);
	    });
}

uint32_t TownDesireSystem::OfferVillager(entt::entity town, bool child, float trigger,
                                         const std::function<uint32_t(TownDesireInfo)>& satisfy)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (town == entt::null || !registry.Valid(town) || !registry.AllOf<TownDesire, TownStats>(town))
	{
		return 0;
	}
	const auto& desire = registry.Get<const TownDesire>(town);
	const auto& stats = registry.Get<const TownStats>(town);
	return town_desire::CheckVillagerNeeded(desire, Locator::infoConstants::value().townDesire, stats.adults + stats.children,
	                                        child, trigger,
	                                        [&satisfy](size_t d) { return satisfy(static_cast<TownDesireInfo>(d)); });
}

float TownDesireSystem::GetDesire(entt::entity town, TownDesireInfo desire) const
{
	const auto* townDesire = Locator::entitiesRegistry::value().TryGet<const TownDesire>(town);
	return townDesire != nullptr && Valid(desire) ? town_desire::GetDesire(*townDesire, Index(desire)) : 0.0f;
}

float TownDesireSystem::GetRawDesire(entt::entity town, TownDesireInfo desire) const
{
	const auto* townDesire = Locator::entitiesRegistry::value().TryGet<const TownDesire>(town);
	return townDesire != nullptr && Valid(desire) ? town_desire::GetRawDesire(*townDesire, Index(desire)) : 0.0f;
}

TownDesireInfo TownDesireSystem::GetMostWanted(entt::entity town) const
{
	const auto* townDesire = Locator::entitiesRegistry::value().TryGet<const TownDesire>(town);
	return townDesire != nullptr ? static_cast<TownDesireInfo>(townDesire->sorted.at(0).index) : TownDesireInfo::None;
}

void TownDesireSystem::SetBoost(entt::entity town, TownDesireInfo desire, float boost, bool resort)
{
	auto* townDesire = Locator::entitiesRegistry::value().TryGet<TownDesire>(town);
	if (townDesire == nullptr || !Valid(desire))
	{
		return;
	}
	townDesire->boost.at(Index(desire)) = boost;
	if (resort)
	{
		town_desire::SortDesires(*townDesire);
	}
}
