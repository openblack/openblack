/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "ECS/Components/TownDesire.h"
#include "Enums.h"

namespace openblack
{
struct GTownInfo;
struct GTownDesireInfo;
} // namespace openblack

/// What a town wants: seventeen desires, from food and wood to relaxation and sleep, worked out each game turn from the
/// town's people, buildings and stores and the time of day. Each is scaled by the town's tribe, then lowered by what its
/// villagers are already doing for it. The desires are kept in order, most first, and an idle villager offers itself to
/// the first it can serve.
///
/// The game works these out in single precision, with integers loaded exactly, as here.
namespace openblack::ecs::town_desire
{

inline constexpr size_t k_Count = components::k_TownDesireCount;

/// One of the town's abodes, for its wish to be repaired
struct RepairInput
{
	/// How whole it is, 0 to 1
	float life {1.0f};
	/// Somewhere people live
	bool livingQuarters {false};
	uint32_t inhabitants {0};
	/// How much its kind wants repairing
	float desireToBeRepaired {0.0f};
};

/// What the desires read of a town and the world, gathered once a turn
struct DesireInputs
{
	components::TownStats stats;
	/// Villagers worshipping and on their way to
	int32_t worshipping {0};
	int32_t onWayToWorship {0};
	uint32_t homeless {0};
	uint32_t abodeCount {0};
	/// The food and wood in the town's storage pit, if it has one, or else in a temporary pot
	std::optional<uint32_t> storageFood;
	std::optional<uint32_t> storageWood;
	std::optional<uint32_t> potFood;
	std::optional<uint32_t> potWood;
	/// Each building site's wish for builders, the builders at it and the places for them
	std::vector<float> siteDesires;
	std::vector<uint32_t> siteBuilders;
	std::vector<int32_t> sitePlaces;
	std::vector<RepairInput> abodes;
	/// The wish of each planned building to be repaired
	std::vector<float> planRepairDesires;
	/// For each abode number, the population at which the town wants one, if its tribe has that kind
	std::array<std::optional<int32_t>, 16> populationWhenNeeded {};
	bool hasPlayer {true};
	bool isLocalPlayer {false};
	/// The player's alignment, -1 to 1
	float alignment {0.0f};
	/// The player's tribal power over children
	float tribalPower4 {1.0f};
	bool crecheFunctional {false};
	float belief {0.0f};
	float protection {0.0f};
	float mercy {0.0f};
	/// The sky as the game counts it, 2 at night to 0 by day, and the visual hour with the hour full day comes
	float skyType {0.0f};
	float visualHour {12.0f};
	float dayFull {8.0f};
	uint32_t turn {0};
	Tribe tribe {Tribe::CELTIC};
};

/// Everything a desire reads: the town's desires (this turn's for those already worked out, last turn's for the rest),
/// the inputs and the info
struct DesireContext
{
	const components::TownDesire& desire;
	const DesireInputs& in;
	const GTownInfo& town;
	const std::array<GTownDesireInfo, k_Count>& info;
	/// The food and wood a villager carries, as the game takes them from its African farmer
	uint32_t farmerMaxFood {150};
	uint32_t farmerMaxWood {250};
};

using DesireFn = float (*)(const DesireContext&);
using ModificationFn = float (*)(const DesireContext&, size_t d);

/// A desire: its name, how the town feels it, how what its villagers do lowers it, whether a child may serve it and
/// whether a villager can satisfy it at all
struct DesireFunctions
{
	const char* name;
	DesireFn function;
	ModificationFn modification;
	bool children;
	bool satisfiable;
};

/// The seventeen desires, in the order of TownDesireInfo
[[nodiscard]] const std::array<DesireFunctions, k_Count>& Table();

[[nodiscard]] float DesireForFood(const DesireContext& c);
[[nodiscard]] float DesireForWood(const DesireContext& c);
[[nodiscard]] float DesireForPlaytime(const DesireContext& c);
[[nodiscard]] float DesireForProtection(const DesireContext& c);
[[nodiscard]] float DesireForMercy(const DesireContext& c);
[[nodiscard]] float DesireForAbodes(const DesireContext& c);
[[nodiscard]] float DesireForCivicBuildings(const DesireContext& c);
[[nodiscard]] float DesireForSupplyWorship(const DesireContext& c);
[[nodiscard]] float DesireForChildren(const DesireContext& c);
[[nodiscard]] float DesireToBuild(const DesireContext& c);
[[nodiscard]] float DesireForRain(const DesireContext& c);
[[nodiscard]] float DesireForSun(const DesireContext& c);
[[nodiscard]] float DesireToRepair(const DesireContext& c);
[[nodiscard]] float DesireToSupplyWorkshop(const DesireContext& c);
[[nodiscard]] float DesireToBuildWonder(const DesireContext& c);
/// Comes up through the evening and stays through the night, never under a tenth
[[nodiscard]] float DesireForRelaxation(const DesireContext& c);
/// Comes up as night falls, past bed time, squared
[[nodiscard]] float DesireForSleep(const DesireContext& c);

/// An abode's wish to be repaired: none when whole enough, nor for an empty home
[[nodiscard]] float AbodeDesireToBeRepaired(const RepairInput& abode, const GTownInfo& town);

/// The evening's ramp: 1 from `offset` hours before the day's full light, coming up over `width` hours, counted back from
/// midnight
[[nodiscard]] float EveningRamp(float visualHour, float dayFull, float width, float offset);

/// A desire with the boosts, as the villagers see it and as the town feels it
[[nodiscard]] float GetDesire(const components::TownDesire& desire, size_t d);
[[nodiscard]] float GetRawDesire(const components::TownDesire& desire, size_t d);

/// How much is left of a desire, 0 to 1, after what the town's villagers are doing for it
[[nodiscard]] float GetDesireVillagerModification(const DesireContext& c, size_t d);
[[nodiscard]] float ModificationGeneral(const DesireContext& c, size_t d);
[[nodiscard]] float ModificationFood(const DesireContext& c, size_t d);
[[nodiscard]] float ModificationWood(const DesireContext& c, size_t d);
[[nodiscard]] float ModificationToBuild(const DesireContext& c, size_t d);
/// How much is left of a desire after what the villagers took up for it this turn
[[nodiscard]] float GetTemporaryDesireVillagerModification(const components::TownDesire& desire, uint32_t population, size_t k);

/// Works out a desire as the town feels it, keeps that and returns it lowered by its villagers, -1 to 1
float CallDesireFunction(components::TownDesire& desire, const DesireContext& c, size_t d);
void ProcessDesire(components::TownDesire& desire, const DesireContext& c, size_t d);
/// The game's C runtime quicksort, most first, which is not stable
void MsvcQsort(std::array<components::DesireSort, k_Count>& entries);
void SortDesires(components::TownDesire& desire);
void SortRawDesires(components::TownDesire& desire);
/// A town's turn of desires, in order, then both orders. Every 50 turns, returns how unhappy its villagers are.
std::optional<float> Process(components::TownDesire& desire, const DesireContext& c);

/// Offers a villager to the town's desires, most first: each it can serve, a child only some, is tried while it is still
/// wanted past the villager's trigger, by `checkSatisfy`. 1 when one took the villager.
uint32_t CheckVillagerNeeded(const components::TownDesire& desire, const std::array<GTownDesireInfo, k_Count>& info,
                             uint32_t population, bool child, float trigger,
                             const std::function<uint32_t(size_t d)>& checkSatisfy);

[[nodiscard]] float GetDesireSignificanceToVillager(const components::TownDesire& desire, const GTownDesireInfo& info,
                                                    size_t d);
/// The desire wanted most above none, or -1
[[nodiscard]] int GetMostDesired(const components::TownDesire& desire);
/// The desire felt most, if at least `minimum`, or -1
[[nodiscard]] int GetMostSignificantRawDesire(const components::TownDesire& desire, float minimum);
/// A desire by its name, any case, or -1
[[nodiscard]] int FindDesire(std::string_view name);
/// How much the town needs, 0 to 1: a fifth of its practical desires as it feels them
[[nodiscard]] float TownNeedsSum(const components::TownDesire& desire);

} // namespace openblack::ecs::town_desire
