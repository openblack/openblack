/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TownDesire.h"

#include <cctype>
#include <cmath>

#include <algorithm>
#include <utility>

#include "InfoConstants.h"

namespace openblack::ecs::town_desire
{
using components::DesireSort;
using components::TownDesire;

namespace
{
constexpr float k_Milli = 0.001f;
constexpr float k_Tiny = 0.0001f;
constexpr float k_TinyAbodes = 1e-5f;
/// Added in double precision, then rounded once
constexpr double k_TinyPopulation = 1e-5;
constexpr float k_HomelessShare = 0.2f;
constexpr float k_SmallestTrigger = 0.001f;
/// Villagers only want playtime once the game has run this many turns
constexpr uint32_t k_PlaytimeAfterTurn = 4000;
constexpr uint32_t k_AverageEvery = 50;
constexpr float k_HoursInDay = 24.0f;

/// 0 below none, 1 above full, and none for a value that isn't a number
float Clamp01(float v)
{
	if (v < 0.0f || std::isnan(v))
	{
		return 0.0f;
	}
	return v <= 1.0f ? v : 1.0f;
}

/// At most 1, and 1 for a value that isn't a number
float MinOne(float x)
{
	return (1.0f < x || std::isnan(x)) ? 1.0f : x;
}

/// Rounds towards zero, keeping the low word
uint32_t Truncate(float v)
{
	return static_cast<uint32_t>(static_cast<int64_t>(v));
}

float Raw(const TownDesire& d, size_t i)
{
	return d.raw.at(i) + d.boost.at(i) + d.boostA.at(i);
}

float Desire(const TownDesire& d, size_t i)
{
	return d.desire.at(i) + d.boost.at(i) + d.boostA.at(i);
}

/// The food the town has: what its villagers carry and the storage pit's, or else a temporary pot's
uint32_t FoodAvailable(const DesireInputs& in)
{
	uint32_t food = Truncate(in.stats.foodCarried);
	if (in.storageFood.has_value())
	{
		return food + *in.storageFood;
	}
	if (in.potFood.has_value())
	{
		food += *in.potFood;
	}
	return food;
}

float DesiredFood(const DesireContext& c)
{
	return c.town.foodWantedMultiplier * c.in.stats.foodForDinner;
}

/// The wood the town has: at its building sites, carried, and the storage pit's or else a temporary pot's
uint32_t WoodAvailable(const DesireInputs& in)
{
	uint32_t wood = Truncate(in.stats.woodAtSites + in.stats.woodCarried);
	if (in.storageWood.has_value())
	{
		return wood + *in.storageWood;
	}
	if (in.potWood.has_value())
	{
		wood += *in.potWood;
	}
	return wood;
}

/// The town's wood wanted grows with its abodes past a number of buildings
float WoodScale(const DesireContext& c, float factor)
{
	const float k = static_cast<float>(c.in.abodeCount) / c.town.numOfBuildingsForDesiredWood;
	const float scale = (k <= 1.0f || std::isnan(k)) ? 1.0f : k;
	return scale * factor;
}

float WoodMinimum(const DesireContext& c)
{
	return WoodScale(c, c.town.minimumWoodForDesire);
}

float WoodMaximum(const DesireContext& c)
{
	return WoodScale(c, c.town.maximumWoodForDesire);
}

const std::array<DesireFunctions, k_Count> k_DesireTable = {{
    {"Food", DesireForFood, ModificationFood, false, true},
    {"Wood", DesireForWood, ModificationWood, false, true},
    {"Playtime", DesireForPlaytime, ModificationGeneral, true, true},
    {"Protection", DesireForProtection, ModificationGeneral, true, false},
    {"Mercy", DesireForMercy, ModificationGeneral, true, false},
    {"Abodes", DesireForAbodes, ModificationGeneral, false, true},
    {"Civic_Buildings", DesireForCivicBuildings, ModificationGeneral, false, true},
    {"Supply_Worship", DesireForSupplyWorship, ModificationGeneral, false, true},
    {"For_Children", DesireForChildren, ModificationGeneral, false, false},
    {"To_Build", DesireToBuild, ModificationToBuild, false, true},
    {"For_Rain", DesireForRain, ModificationGeneral, false, false},
    {"For_Sun", DesireForSun, ModificationGeneral, false, false},
    {"Repair_Town", DesireToRepair, ModificationGeneral, false, true},
    // The game's own spelling
    {"Suppy_Workshop", DesireToSupplyWorkshop, ModificationGeneral, false, true},
    {"For_Wonder", DesireToBuildWonder, ModificationGeneral, false, false},
    {"Relaxation", DesireForRelaxation, ModificationGeneral, true, true},
    {"Sleep", DesireForSleep, ModificationGeneral, true, true},
}};

/// Most first: 1 when a comes after b, as the game's comparison does, which also puts anything not a number after
int Compare(const DesireSort& a, const DesireSort& b)
{
	if (a.value < b.value || std::isnan(a.value) || std::isnan(b.value))
	{
		return 1;
	}
	if (a.value == b.value)
	{
		return 0;
	}
	return -1;
}

/// The runtime's sort of a short stretch: the last of [lo, hi] each time goes to hi
void ShortSort(std::array<DesireSort, k_Count>& a, int lo, int hi)
{
	while (hi > lo)
	{
		int max = lo;
		for (int p = lo + 1; p <= hi; ++p)
		{
			if (Compare(a.at(static_cast<size_t>(p)), a.at(static_cast<size_t>(max))) > 0)
			{
				max = p;
			}
		}
		std::swap(a.at(static_cast<size_t>(max)), a.at(static_cast<size_t>(hi)));
		--hi;
	}
}
} // namespace

const std::array<DesireFunctions, k_Count>& Table()
{
	return k_DesireTable;
}

float EveningRamp(float visualHour, float dayFull, float width, float offset)
{
	const float untilMidnight = k_HoursInDay - visualHour;
	const float start = dayFull + offset;
	if (untilMidnight < start)
	{
		return 1.0f;
	}
	if (!((dayFull + width) + offset > untilMidnight))
	{
		return 0.0f;
	}
	return 1.0f - (untilMidnight - start) / width;
}

float DesireForFood(const DesireContext& c)
{
	const auto available = static_cast<float>(FoodAvailable(c.in)) + k_Tiny;
	return Clamp01(1.0f - available / (DesiredFood(c) + k_Tiny));
}

float DesireForWood(const DesireContext& c)
{
	const auto& d = c.desire;
	// The wish to repair, build, and for civic buildings and abodes, as felt, in the game's order of adding, at most 3
	const float sum = d.raw.at(12) + d.raw.at(9) + d.raw.at(6) + d.raw.at(5) + d.boost.at(12) + d.boost.at(9) + d.boost.at(6) +
	                  d.boostA.at(12) + d.boost.at(5) + d.boostA.at(9) + d.boostA.at(6) + d.boostA.at(5);
	const float building = (sum < 3.0f || std::isnan(sum)) ? sum : 3.0f;
	// The share of craftsmen among the adults, and the building
	const auto craftsmen = c.in.stats.disciples.at(static_cast<size_t>(VillagerDisciple::Craftsman));
	const float a = (static_cast<float>(craftsmen) + k_Milli) / (static_cast<float>(c.in.stats.adults) + k_Milli) + building;
	const auto wood = static_cast<float>(WoodAvailable(c.in));
	const auto minimum = WoodMinimum(c);
	const auto maximum = WoodMaximum(c);
	float x = wood / minimum;
	x = (x < 1.0f || std::isnan(x)) ? x : 1.0f;
	const float t = 1.0f - x + a;
	float y = wood / maximum;
	y = (y < 1.0f || std::isnan(y)) ? y : 1.0f;
	return Clamp01((1.0f - y) * t);
}

float DesireForPlaytime(const DesireContext& c)
{
	// Only while food, wood, abodes, civic buildings and building are all wanted less than they make villagers act
	for (const size_t d : {0u, 1u, 5u, 6u, 9u})
	{
		if (!(Desire(c.desire, d) < c.info.at(d).desireTriggersVillagerAction))
		{
			return 0.0f;
		}
	}
	return c.in.turn > k_PlaytimeAfterTurn ? 0.1f : 0.0f;
}

float DesireForProtection(const DesireContext& c)
{
	return c.in.protection;
}

float DesireForMercy(const DesireContext& c)
{
	return c.in.mercy;
}

float DesireForAbodes(const DesireContext& c)
{
	const auto& s = c.in.stats;
	// How full the adults' and the children's places are, each at most one and a half, the fuller of the two
	const float adults =
	    static_cast<int32_t>(s.adults) / (static_cast<float>(static_cast<int32_t>(s.adultPlaces)) + k_TinyAbodes);
	const auto adultsFull = (adults < 1.5f || std::isnan(adults)) ? adults : 1.5f;
	const float children =
	    static_cast<int32_t>(s.children) / (static_cast<float>(static_cast<int32_t>(s.childPlaces)) + k_TinyAbodes);
	const float childrenFull = (children < 1.5f || std::isnan(children)) ? children : 1.5f;
	const float m = (childrenFull <= adultsFull || std::isnan(childrenFull)) ? adultsFull : childrenFull;
	// To the fourth, less what building and civic building already take up
	const float m4 = m * m * m * m;
	const auto p = m4 * (1.0f - Raw(c.desire, 9));
	return Clamp01((1.0f - Desire(c.desire, 6)) * p);
}

float DesireForCivicBuildings(const DesireContext& c)
{
	const auto& s = c.in.stats;
	const uint32_t population = s.adults + s.children;
	float sum = 0.0f;
	for (size_t i = 0; i < 16; ++i)
	{
		// Each kind of abode the town has none of, wanted once the town is big enough, and not while many are homeless
		if (s.abodesByNumber.at(i) != 0)
		{
			continue;
		}
		const auto& needed = c.in.populationWhenNeeded.at(i);
		if (!needed.has_value() || *needed <= -1)
		{
			continue;
		}
		const int32_t p = *needed;
		if (p != 0)
		{
			const float share = static_cast<float>(c.in.homeless) / static_cast<int32_t>(population);
			if (!(share < k_HomelessShare || std::isnan(share)))
			{
				continue;
			}
		}
		if (p > static_cast<int32_t>(population))
		{
			continue;
		}
		const float add =
		    (static_cast<float>(static_cast<int32_t>(population) - p) + k_Milli) / (static_cast<float>(p) + k_Milli);
		sum = add * 0.5f + sum + 0.5f;
	}
	return (sum < 1.0f || std::isnan(sum)) ? sum : 1.0f;
}

float DesireForSupplyWorship([[maybe_unused]] const DesireContext& c)
{
	return 0.0f;
}

float DesireForChildren(const DesireContext& c)
{
	const auto& s = c.in.stats;
	// Wanted less as food is wanted more
	float a = 1.0f;
	const float food = Raw(c.desire, 0);
	if (!(food < 1.0f || std::isnan(food)))
	{
		a = 0.0f;
	}
	else if (!(food <= 0.0f || std::isnan(food)))
	{
		a = 1.0f - food;
	}
	// Only with room for children, and half as much without room for adults
	const float b = s.children < s.childPlaces ? 1.0f : 0.0f;
	const float adultsRoom = s.adults < s.adultPlaces ? 1.0f : 0.5f;
	// Less in want of protection or mercy
	const float protection = Desire(c.desire, 3);
	const float p = (protection <= 0.0f || std::isnan(protection)) ? 0.0f : protection;
	const float mercy = Desire(c.desire, 4);
	const float m = (mercy <= 0.0f || std::isnan(mercy)) ? 0.0f : mercy;
	const auto q = (1.0f - p) * (1.0f - m);
	// More with a good player, by the cube of their alignment, and by their power over children
	float alignment = 0.0f;
	if (c.in.hasPlayer)
	{
		alignment = c.in.alignment * c.in.alignment * c.in.alignment * 0.5f;
	}
	const float power = c.in.hasPlayer ? c.in.tribalPower4 : 1.0f;
	auto v = power * (1.0f + alignment) * q * adultsRoom * b * a;
	// Half without a working creche
	if (!c.in.crecheFunctional)
	{
		v = v * 0.5f;
	}
	return Clamp01(v);
}

float DesireToBuild(const DesireContext& c)
{
	float sum = 0.0f;
	for (const float site : c.in.siteDesires)
	{
		sum = site + sum;
	}
	return (1.0f < sum || std::isnan(sum)) ? 1.0f : sum;
}

float DesireForRain([[maybe_unused]] const DesireContext& c)
{
	return 0.0f;
}

float DesireForSun([[maybe_unused]] const DesireContext& c)
{
	return 0.0f;
}

float AbodeDesireToBeRepaired(const RepairInput& abode, const GTownInfo& town)
{
	if (abode.life > town.thresholdToStartRepairing)
	{
		return 0.0f;
	}
	if (abode.livingQuarters && abode.inhabitants == 0)
	{
		return 0.0f;
	}
	if (!(abode.life < 1.0f || std::isnan(abode.life)))
	{
		return 0.0f;
	}
	const float v = ((1.0f - abode.life) * 0.5f + 0.5f) * abode.desireToBeRepaired;
	return (v < 1.0f || std::isnan(v)) ? v : 1.0f;
}

float DesireToRepair(const DesireContext& c)
{
	float sum = 0.0f;
	for (const auto& abode : c.in.abodes)
	{
		sum = AbodeDesireToBeRepaired(abode, c.town) + sum;
	}
	for (const float plan : c.in.planRepairDesires)
	{
		sum = plan + sum;
	}
	return (1.0f < sum || std::isnan(sum)) ? 1.0f : sum;
}

float DesireToSupplyWorkshop([[maybe_unused]] const DesireContext& c)
{
	return 0.0f;
}

float DesireToBuildWonder(const DesireContext& c)
{
	// By the town's belief, less as food, wood and abodes are wanted, down to half
	const auto abodesAndWood = Desire(c.desire, 1) + Desire(c.desire, 5);
	float h = (Desire(c.desire, 0) + abodesAndWood) * 0.5f;
	h = (h < 0.5f || std::isnan(h)) ? h : 0.5f;
	return (1.0f - h) * c.in.belief;
}

float DesireForRelaxation(const DesireContext& c)
{
	const auto width = c.town.relaxationMod * 0.5f;
	const float ramp = EveningRamp(c.in.visualHour, c.in.dayFull, width, width);
	float x = 1.0f - c.in.skyType;
	x = (x <= 0.0f || std::isnan(x)) ? 0.0f : x;
	const float r = ramp * x;
	if (r < 0.1f || std::isnan(r))
	{
		return 0.1f;
	}
	return r <= 1.0f ? r : 1.0f;
}

float DesireForSleep(const DesireContext& c)
{
	float s = EveningRamp(c.in.visualHour, c.in.dayFull, 1.0f, 0.0f) + c.in.skyType - c.town.bedTimeMod;
	s = (s <= 0.0f || std::isnan(s)) ? 0.0f : s;
	return s * s;
}

float GetDesire(const TownDesire& desire, size_t d)
{
	return Desire(desire, d);
}

float GetRawDesire(const TownDesire& desire, size_t d)
{
	return Raw(desire, d);
}

float GetDesireVillagerModification(const DesireContext& c, size_t d)
{
	return k_DesireTable.at(d).modification(c, d);
}

float ModificationGeneral(const DesireContext& c, size_t d)
{
	const auto population = static_cast<float>(static_cast<double>(c.in.stats.adults + c.in.stats.children) + k_TinyPopulation);
	return 1.0f - MinOne(c.desire.doingNow.at(d) / population);
}

float ModificationFood(const DesireContext& c, size_t d)
{
	// The food the villagers fetching it will bring, and what is in store
	auto n = static_cast<float>(c.farmerMaxFood) * c.desire.doingNow.at(d) + k_Tiny;
	const float store = c.in.storageFood.has_value() ? static_cast<float>(*c.in.storageFood) : 0.0f;
	n = store + n;
	return 1.0f - MinOne(n / (DesiredFood(c) + k_Tiny));
}

float ModificationWood(const DesireContext& c, size_t d)
{
	auto n = static_cast<float>(c.farmerMaxWood) * c.desire.doingNow.at(d) + k_Tiny;
	const float store = c.in.storageWood.has_value() ? static_cast<float>(*c.in.storageWood) : 0.0f;
	n = store + n;
	return 1.0f - MinOne(n / (WoodMaximum(c) + k_Tiny));
}

float ModificationToBuild(const DesireContext& c, [[maybe_unused]] size_t d)
{
	// The builders at the sites over the places for them
	auto builders = k_Tiny;
	auto places = k_Tiny;
	for (size_t i = 0; i < c.in.siteBuilders.size(); ++i)
	{
		builders = static_cast<float>(c.in.siteBuilders.at(i)) + builders;
		const int32_t sitePlaces = i < c.in.sitePlaces.size() ? c.in.sitePlaces.at(i) : 0;
		places = static_cast<float>(sitePlaces) + places;
	}
	return 1.0f - MinOne(builders / places);
}

float GetTemporaryDesireVillagerModification(const TownDesire& desire, uint32_t population, size_t k)
{
	const auto people = static_cast<float>(static_cast<double>(population) + k_TinyPopulation);
	float taken = desire.doingNow.at(k) - desire.doingNowAtStart.at(k);
	taken = (taken < 0.0f) ? 0.0f : taken;
	return 1.0f - MinOne(taken / people);
}

float CallDesireFunction(TownDesire& desire, const DesireContext& c, size_t d)
{
	const auto& entry = k_DesireTable.at(d);
	const float felt = entry.function(c);
	// Scaled by the town's tribe; a town of no tribe takes it as it is
	const auto tribe = static_cast<size_t>(static_cast<int32_t>(c.in.tribe));
	const auto& multipliers = c.info.at(d).tribeMultiplier;
	const float multiplier = tribe < multipliers.size() ? multipliers.at(tribe) : 1.0f;
	const auto raw = multiplier * felt;
	desire.raw.at(d) = raw;
	const float v = GetDesireVillagerModification(c, d) * raw;
	if (v < -1.0f || std::isnan(v))
	{
		return -1.0f;
	}
	return v <= 1.0f ? v : 1.0f;
}

void ProcessDesire(TownDesire& desire, const DesireContext& c, size_t d)
{
	auto& doing = desire.doingNow.at(d);
	if (doing < 0.0f || std::isnan(doing))
	{
		doing = 0.0f;
	}
	desire.doingNowAtStart.at(d) = desire.doingNow.at(d);
	desire.doingNowCountAtStart.at(d) = desire.doingNowCount.at(d);
	desire.desire.at(d) = CallDesireFunction(desire, c, d);
}

void MsvcQsort(std::array<DesireSort, k_Count>& entries)
{
	// Short stretches are sorted directly; longer ones split about their middle, the smaller side first
	constexpr int k_Cutoff = 8;
	std::array<int, 30> loStack {};
	std::array<int, 30> hiStack {};
	int stack = 0;
	int lo = 0;
	int hi = static_cast<int>(k_Count) - 1;
	for (;;)
	{
		const int size = hi - lo + 1;
		if (size <= k_Cutoff)
		{
			ShortSort(entries, lo, hi);
		}
		else
		{
			const int mid = lo + (size / 2);
			std::swap(entries.at(static_cast<size_t>(mid)), entries.at(static_cast<size_t>(lo)));
			int loGuy = lo;
			int hiGuy = hi + 1;
			for (;;)
			{
				do
				{
					++loGuy;
				} while (loGuy <= hi &&
				         Compare(entries.at(static_cast<size_t>(loGuy)), entries.at(static_cast<size_t>(lo))) <= 0);
				do
				{
					--hiGuy;
				} while (hiGuy > lo &&
				         Compare(entries.at(static_cast<size_t>(hiGuy)), entries.at(static_cast<size_t>(lo))) >= 0);
				if (hiGuy < loGuy)
				{
					break;
				}
				std::swap(entries.at(static_cast<size_t>(loGuy)), entries.at(static_cast<size_t>(hiGuy)));
			}
			std::swap(entries.at(static_cast<size_t>(lo)), entries.at(static_cast<size_t>(hiGuy)));
			if (hiGuy - lo > hi - loGuy)
			{
				if (lo + 1 < hiGuy)
				{
					loStack.at(static_cast<size_t>(stack)) = lo;
					hiStack.at(static_cast<size_t>(stack)) = hiGuy - 1;
					++stack;
				}
				if (loGuy < hi)
				{
					lo = loGuy;
					continue;
				}
			}
			else
			{
				if (loGuy < hi)
				{
					loStack.at(static_cast<size_t>(stack)) = loGuy;
					hiStack.at(static_cast<size_t>(stack)) = hi;
					++stack;
				}
				if (lo + 1 < hiGuy)
				{
					hi = hiGuy - 1;
					continue;
				}
			}
		}
		--stack;
		if (stack < 0)
		{
			return;
		}
		lo = loStack.at(static_cast<size_t>(stack));
		hi = hiStack.at(static_cast<size_t>(stack));
	}
}

void SortDesires(TownDesire& desire)
{
	for (size_t d = 0; d < k_Count; ++d)
	{
		auto& e = desire.sorted.at(d);
		e.boosts = desire.boost.at(d) + desire.boostA.at(d);
		e.value = GetDesire(desire, d);
		e.index = static_cast<uint32_t>(d);
	}
	MsvcQsort(desire.sorted);
}

void SortRawDesires(TownDesire& desire)
{
	for (size_t d = 0; d < k_Count; ++d)
	{
		auto& e = desire.sortedRaw.at(d);
		e.boosts = desire.boostA.at(d);
		e.value = GetRawDesire(desire, d);
		e.index = static_cast<uint32_t>(d);
	}
	MsvcQsort(desire.sortedRaw);
}

std::optional<float> Process(TownDesire& desire, const DesireContext& c)
{
	// Its people not off to worship, counted as the game does, unsigned
	const uint32_t people = c.in.stats.children - static_cast<uint32_t>(c.in.onWayToWorship) -
	                        static_cast<uint32_t>(c.in.worshipping) + c.in.stats.adults;
	desire.population = static_cast<float>(people);
	for (size_t d = 0; d < k_Count; ++d)
	{
		ProcessDesire(desire, c, d);
	}
	SortDesires(desire);
	SortRawDesires(desire);
	if (c.in.turn % k_AverageEvery != 0 || !c.in.hasPlayer)
	{
		return std::nullopt;
	}
	// How unhappy its villagers are: food twice, wood, and the more of abodes and civic buildings and of protection and
	// mercy
	const float r5 = Raw(desire, 5);
	const auto r6 = Raw(desire, 6);
	const float buildings = r5 > r6 ? r5 : r6;
	const float r4 = Raw(desire, 4);
	const auto r3 = Raw(desire, 3);
	const float safety = r4 > r3 ? r4 : r3;
	float sum = Raw(desire, 0);
	sum = sum + sum;
	sum = sum + desire.raw.at(1) + desire.boost.at(1) + desire.boostA.at(1);
	sum = sum + safety + buildings;
	return sum / c.town.divisorForAverageDesires;
}

uint32_t CheckVillagerNeeded(const TownDesire& desire, const std::array<GTownDesireInfo, k_Count>& info, uint32_t population,
                             bool child, float trigger, const std::function<uint32_t(size_t d)>& checkSatisfy)
{
	if (trigger == 0.0f || std::isnan(trigger))
	{
		trigger = k_SmallestTrigger;
	}
	for (size_t k = 0; k < k_Count; ++k)
	{
		const auto& e = desire.sorted.at(k);
		if (static_cast<size_t>(e.index) >= k_Count)
		{
			continue;
		}
		const size_t d = e.index;
		const auto& entry = k_DesireTable.at(d);
		const float sum = trigger + info.at(d).desireTriggersVillagerAction;
		const float threshold = (sum < 1.0f || std::isnan(sum)) ? sum : 1.0f;
		if ((child && !entry.children) || !entry.satisfiable)
		{
			continue;
		}
		// The game asks how much is left of this place in the order with the counts of the desire numbered the same
		// as the place, not of the desire in it
		const float m = GetTemporaryDesireVillagerModification(desire, population, k) * e.value;
		if (m <= threshold || std::isnan(m))
		{
			return 0;
		}
		if (checkSatisfy(d) == 1)
		{
			return 1;
		}
	}
	return 0;
}

float GetDesireSignificanceToVillager(const TownDesire& desire, const GTownDesireInfo& info, size_t d)
{
	const float v = Desire(desire, d) - info.desireTriggersVillagerAction;
	return (v <= 0.0f || std::isnan(v)) ? 0.0f : v;
}

int GetMostDesired(const TownDesire& desire)
{
	float best = 0.0f;
	int index = -1;
	for (size_t d = 0; d < k_Count; ++d)
	{
		if (best < desire.desire.at(d))
		{
			best = desire.desire.at(d);
			index = static_cast<int>(d);
		}
	}
	return index;
}

int GetMostSignificantRawDesire(const TownDesire& desire, float minimum)
{
	const auto& first = desire.sortedRaw.at(0);
	if (first.value < minimum || std::isnan(first.value) || std::isnan(minimum))
	{
		return -1;
	}
	return static_cast<int>(first.index);
}

int FindDesire(std::string_view name)
{
	for (size_t d = 0; d < k_Count; ++d)
	{
		const std::string_view entry(k_DesireTable.at(d).name);
		if (std::ranges::equal(entry, name, [](char a, char b) {
			    return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		    }))
		{
			return static_cast<int>(d);
		}
	}
	return -1;
}

float TownNeedsSum(const TownDesire& desire)
{
	// Workshops, repair, building, worship supplies, civic buildings, abodes, mercy, protection, wood and food, as felt
	const auto& raw = desire.raw;
	float sum = raw.at(13);
	for (const size_t d : {12u, 9u, 7u, 6u, 5u, 4u, 3u, 1u, 0u})
	{
		sum += raw.at(d);
	}
	sum *= 0.2f;
	return std::clamp(sum, 0.0f, 1.0f);
}

} // namespace openblack::ecs::town_desire
