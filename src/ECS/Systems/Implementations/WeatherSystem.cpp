/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "WeatherSystem.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "Common/RandomNumberManager.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"

namespace openblack::ecs::systems
{

using components::Climate;
using components::Storm;
using components::WeatherInfo;

namespace
{
// The calendar: a game year is 36000 turns and the calendar starts on 5 May 1998 at 18:05:30
constexpr float k_TurnsPerYear = 36000.0f;
constexpr float k_DaysInYear = 365.25f;
constexpr float k_SecondsInDay = 86400.0f;
constexpr int32_t k_StartYear = 1998;
constexpr int32_t k_StartMonth = 5;
constexpr int32_t k_StartDay = 5;
constexpr int32_t k_StartHour = 18;
constexpr int32_t k_StartMinute = 5;
constexpr int32_t k_StartSecond = 30;
/// First day of the year after each month
constexpr std::array<uint16_t, 12> k_MonthEnds = {31, 59, 90, 120, 151, 181, 212, 243, 273, 303, 333, 364};
/// First day of the year after winter, spring, summer and autumn
constexpr std::array<uint16_t, 4> k_SeasonStarts = {79, 171, 263, 354};

// How much of the day's temperature range is reached at each relative hour and in each month
constexpr std::array<float, 24> k_TemperatureByHour = {0.5f, 0.4f, 0.3f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f,
                                                       1.0f, 1.1f, 1.2f, 1.3f, 1.5f, 1.2f, 1.1f, 1.0f, 0.9f, 0.8f, 0.7f, 0.6f};
constexpr std::array<float, 13> k_TemperatureByMonth = {0.0f, 0.1f, 0.0f, 0.3f, 0.4f, 0.5f, 0.6f,
                                                        0.8f, 1.0f, 0.7f, 0.4f, 0.3f, 0.2f};
/// Hours of the morning the temperature table is laid out for: full night, dusk start and end, full day
constexpr std::array<float, 4> k_CanonicalDayTimes = {3.5f, 7.5f, 8.0f, 8.5f};

// The atmosphere: how the weather changes with height above the ground
constexpr float k_HighAltitude = 200.0f;
constexpr float k_LowAltitude = 50.0f;
constexpr float k_HighAltitudeBlend = 0.25f;
constexpr float k_TemperatureLapse = -0.075f;
constexpr int8_t k_HighAltitudeCooling = 11;

// Climates and the storms they create
constexpr float k_GlobalRadius = 5120.0f;
constexpr uint16_t k_GlobalCentreCell = 256;
constexpr uint32_t k_GlobalMaxStorms = 10;
constexpr float k_MaxStormsPerUnit = 0.001f;
constexpr uint32_t k_StormCloudHeight = 500;
constexpr std::array<float, 5> k_StormLightning = {1.0f, 5.0f, 60.0f, 5.0f, 60.0f};
constexpr float k_RainDesireRandom = 0.02f;
constexpr float k_TemperatureStep = 0.1f;
constexpr float k_WindSpread = 5.0f;
constexpr float k_StormDrift = 0.01f;
constexpr uint32_t k_MinStormRadius = 160;
constexpr uint32_t k_MaxStormRadius = 900;
constexpr uint32_t k_GlobalStormRadiusRange = 1000;
constexpr double k_StormOuterRadius = 1.1;
constexpr float k_StormHeight = 300.0f;
constexpr float k_StormFadeTime = 10.0f;
constexpr float k_MinStormLife = 20.0f;
constexpr float k_ShortStormLife = 8.0f;
constexpr int k_StormPlacementTries = 20;

// Storms as they live, move and fade
constexpr float k_TurnDuration = 0.1f;
constexpr float k_ArrivalDistance = 0.001f;
constexpr float k_EffectScale = 256.0f;
constexpr uint8_t k_DeadStormTurns = 2;

int32_t Ftol(double value)
{
	if (value <= -2147483649.0 || value >= 2147483648.0)
	{
		return INT32_MIN;
	}
	return static_cast<int32_t>(value);
}

int8_t ClampByte(int32_t value)
{
	return static_cast<int8_t>(std::clamp(value, -128, 127));
}

/// a + (b - a) * weight / 256 in the wrapping byte arithmetic of the original
int8_t LerpByte(int8_t a, int8_t b, int32_t weight)
{
	return static_cast<int8_t>(a + static_cast<int8_t>(((static_cast<int32_t>(b) - a) * weight) >> 8));
}

WeatherInfo Lerp(const WeatherInfo& a, const WeatherInfo& b, int32_t weight)
{
	WeatherInfo result = a;
	result.temperature = LerpByte(a.temperature, b.temperature, weight);
	result.rain = LerpByte(a.rain, b.rain, weight);
	result.snow = LerpByte(a.snow, b.snow, weight);
	result.overcast = LerpByte(a.overcast, b.overcast, weight);
	result.windX = LerpByte(a.windX, b.windX, weight);
	result.windZ = LerpByte(a.windZ, b.windZ, weight);
	result.snowCover = LerpByte(a.snowCover, b.snowCover, weight);
	return result;
}

std::array<float, 4> Seasons(float spring, float summer, float autumn, float winter)
{
	return {spring, summer, autumn, winter};
}

float RainMin(const GClimateInfo& info, uint32_t season)
{
	return Seasons(info.rainMinSpring, info.rainMinSummer, info.rainMinAutumn, info.rainMinWinter).at(season);
}

float RainMax(const GClimateInfo& info, uint32_t season)
{
	return Seasons(info.rainMaxSpring, info.rainMaxSummer, info.rainMaxAutumn, info.rainMaxWinter).at(season);
}

float TempMin(const GClimateInfo& info, uint32_t season)
{
	return Seasons(info.tempMinSpring, info.tempMinSummer, info.tempMinAutumn, info.tempMinWinter).at(season);
}

float TempMax(const GClimateInfo& info, uint32_t season)
{
	return Seasons(info.tempMaxSpring, info.tempMaxSummer, info.tempMaxAutumn, info.tempMaxWinter).at(season);
}

float WindMin(const GClimateInfo& info, uint32_t season)
{
	return Seasons(info.windMinSpring, info.windMinSummer, info.windMinAutumn, info.windMinWinter).at(season);
}

float WindMax(const GClimateInfo& info, uint32_t season)
{
	return Seasons(info.windMaxSpring, info.windMaxSummer, info.windMaxAutumn, info.windMaxWinter).at(season);
}

/// Maps an hour onto the sky's day so the temperature table follows dusk and dawn
float RelativeHour(float hour, const SkyInterface::DayNightTimes& sky)
{
	const std::array<float, 4> actual = {sky.nightFull, sky.duskStart, sky.duskEnd, sky.dayFull};
	const bool afternoon = hour > 12.0f;
	if (afternoon)
	{
		hour = 24.0f - hour;
	}

	float result;
	if (hour < k_CanonicalDayTimes[0])
	{
		result = (hour / k_CanonicalDayTimes[0]) * actual[0];
	}
	else if (hour >= k_CanonicalDayTimes[3])
	{
		const auto fraction = (hour - k_CanonicalDayTimes[3]) / (12.0f - k_CanonicalDayTimes[3]);
		result = (fraction * (12.0f - actual[3])) + actual[3];
	}
	else
	{
		size_t phase = 0;
		while (hour >= k_CanonicalDayTimes.at(phase + 1))
		{
			++phase;
		}
		const auto fraction =
		    (hour - k_CanonicalDayTimes.at(phase)) / (k_CanonicalDayTimes.at(phase + 1) - k_CanonicalDayTimes.at(phase));
		result = (fraction * (actual.at(phase + 1) - actual.at(phase))) + actual.at(phase);
	}
	return afternoon ? 24.0f - result : result;
}

/// Adds a storm's effect to the weather at a point
void ApplyStorm(const Storm& storm, const glm::vec3& point, WeatherInfo& info)
{
	const auto& centre = storm.currentPosition;
	const auto outer = storm.outerRadius;
	if (centre.x - outer > point.x || centre.x + outer < point.x || centre.z - outer > point.z || centre.z + outer < point.z)
	{
		return;
	}
	const auto dx = point.x - centre.x;
	const auto dz = point.z - centre.z;
	const auto distanceSquared = (dx * dx) + (dz * dz);
	if (distanceSquared > outer * outer)
	{
		return;
	}

	const auto inner = storm.currentInnerRadius;
	const auto falloff =
	    distanceSquared <= inner * inner ? 1.0f : 1.0f - ((std::sqrt(distanceSquared) - inner) / (outer - inner));
	const auto weight = Ftol(falloff * storm.currentStrength * k_EffectScale);
	if (weight == 0)
	{
		return;
	}

	const auto add = [weight](int8_t value, int8_t effect) {
		return ClampByte(((static_cast<int32_t>(effect) * weight) >> 8) + value);
	};
	info.temperature = LerpByte(info.temperature, storm.effect.temperature, weight);
	info.rain = add(info.rain, storm.effect.rain);
	info.snow = add(info.snow, storm.effect.snow);
	info.overcast = add(info.overcast, storm.effect.overcast);
	info.windX = add(info.windX, storm.effect.windX);
	info.windZ = add(info.windZ, storm.effect.windZ);
}

float Distance(const glm::vec3& a, const glm::vec3& b)
{
	return glm::distance(a, b);
}

glm::vec3 CellCentre(const Climate& climate)
{
	return {static_cast<float>(climate.cellX) * 10.0f, 0.0f, static_cast<float>(climate.cellZ) * 10.0f};
}
} // namespace

WeatherSystem::WeatherSystem()
{
	// The turn the calendar starts on
	const auto turnsPerDay = k_TurnsPerYear / k_DaysInYear;
	_secondsPerTurn = k_SecondsInDay / turnsPerDay;
	const auto secondsPerTurn = static_cast<double>(_secondsPerTurn);
	const auto secondsPerYear = static_cast<double>(k_SecondsInDay * k_DaysInYear);
	auto start = static_cast<double>(k_StartYear) * secondsPerYear / secondsPerTurn;
	static_assert(k_StartMonth > 1);
	start += static_cast<double>(k_MonthEnds[k_StartMonth - 2]) * k_SecondsInDay / secondsPerTurn;
	start += static_cast<double>(k_StartDay) * k_SecondsInDay / secondsPerTurn;
	start += static_cast<double>(k_StartHour) * k_SecondsInDay / 24.0 / secondsPerTurn;
	start += static_cast<double>(k_StartMinute) * k_SecondsInDay / 1440.0 / secondsPerTurn;
	start += static_cast<double>(k_StartSecond) / secondsPerTurn;
	_startTurn = start;

	_grid.resize(k_GridSize * k_GridSize);
}

void WeatherSystem::Reset()
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<Storm>([&registry](entt::entity entity, Storm&) { registry.Destroy(entity); });
	registry.Each<Climate>([&registry](entt::entity entity, Climate&) { registry.Destroy(entity); });
	_climateSystemEnabled = true;
	_stormCreationEnabled = true;
	_day.reset();
	_season = GetSeason(0);
	_activeStorms.clear();
	++_stamp;
}

float WeatherSystem::GetDaysFromStart(uint32_t turn) const
{
	return static_cast<float>((static_cast<double>(turn) + _startTurn) * _secondsPerTurn / k_SecondsInDay);
}

float WeatherSystem::GetDayOfYear(uint32_t turn) const
{
	return std::fmod(GetDaysFromStart(turn), k_DaysInYear);
}

uint32_t WeatherSystem::GetSeason(uint32_t turn) const
{
	const auto day = GetDayOfYear(turn);
	const auto next = std::ranges::find_if(k_SeasonStarts, [day](uint16_t start) { return day < static_cast<float>(start); });
	const auto index = static_cast<uint32_t>(next - k_SeasonStarts.begin());
	return index != 0 ? index - 1 : 3;
}

uint32_t WeatherSystem::GetMonth(uint32_t turn) const
{
	const auto day = GetDayOfYear(turn);
	for (uint32_t i = 0; i < k_MonthEnds.size(); ++i)
	{
		if (day < static_cast<float>(k_MonthEnds.at(i)))
		{
			return i + 1;
		}
	}
	return 12;
}

float WeatherSystem::GetDayOfMonth(uint32_t turn) const
{
	const auto day = GetDayOfYear(turn);
	const auto month = GetMonth(turn);
	return month > 1 && day >= static_cast<float>(k_MonthEnds.at(month - 2)) ? day - k_MonthEnds.at(month - 2) : day;
}

float WeatherSystem::RandomFloat(float max) const
{
	if (!(max > 0.0f))
	{
		return 0.0f;
	}
	return Locator::rng::value().NextValue(0.0f, max);
}

const GClimateInfo& WeatherSystem::GetInfo(const Climate& climate) const
{
	const auto& infos = Locator::infoConstants::value().climate;
	return infos.at(std::min<size_t>(climate.info, infos.size() - 1));
}

entt::entity WeatherSystem::FindClimate(int32_t index) const
{
	entt::entity found = entt::null;
	Locator::entitiesRegistry::value().Each<const Climate>([index, &found](entt::entity entity, const Climate& climate) {
		if (climate.index == index)
		{
			found = entity;
		}
	});
	return found;
}

void WeatherSystem::InitialiseClimate(Climate& climate) const
{
	const auto& info = GetInfo(climate);

	// The rain budget and desire start from the season's values
	climate.rainingDays = Ftol(RainMin(info, _season) * 100.0f);
	climate.rainDesire = RainMax(info, _season);

	// The temperature starts at the time of day's target
	const auto& sky = Locator::skySystem::value();
	const auto hour = RelativeHour(static_cast<float>(Ftol(sky.GetTime())), sky.GetDayNightTimes());
	const auto hourFactor = k_TemperatureByHour.at(std::clamp(Ftol(hour), 0, 23));
	const auto monthFactor = k_TemperatureByMonth.at(GetMonth(_turn));
	const auto minimum = TempMin(info, _season);
	climate.targetTemperature = ((TempMax(info, _season) - minimum) * hourFactor * monthFactor) + minimum;
	climate.temperature = climate.targetTemperature;

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"),
	                    "Climate {} (type {}): season {} rain {}-{} temperature {}-{} wind {}-{}, desire {} raining days {}",
	                    climate.index, climate.info, _season, RainMin(info, _season), RainMax(info, _season),
	                    TempMin(info, _season), TempMax(info, _season), WindMin(info, _season), WindMax(info, _season),
	                    climate.rainDesire, climate.rainingDays);

	climate.stormCloudHeight = static_cast<float>(k_StormCloudHeight);
	climate.stormSpeed = static_cast<float>((WindMax(info, _season) * (1.0 / 30.0)) + 0.5);
	climate.stormLightning = k_StormLightning;
	climate.stormOvercast = 0;
}

entt::entity WeatherSystem::GetGlobalClimate()
{
	auto entity = FindClimate(0);
	if (entity != entt::null)
	{
		return entity;
	}

	// The global climate covers the whole map with the first climate type
	auto& registry = Locator::entitiesRegistry::value();
	entity = registry.Create();
	auto& climate = registry.Assign<Climate>(entity);
	climate.index = 0;
	climate.global = true;
	climate.info = 0;
	climate.cellX = k_GlobalCentreCell;
	climate.cellZ = k_GlobalCentreCell;
	climate.innerRadius = k_GlobalRadius;
	climate.outerRadius = k_GlobalRadius;
	climate.maxStorms = k_GlobalMaxStorms;
	InitialiseClimate(climate);
	return entity;
}

void WeatherSystem::CreateClimate(int32_t index, uint32_t info, glm::vec2 position, float radius1, float radius2)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (const auto existing = FindClimate(index); existing != entt::null)
	{
		for (const auto storm : registry.Get<Climate>(existing).storms)
		{
			if (registry.Valid(storm))
			{
				registry.Destroy(storm);
			}
		}
		registry.Destroy(existing);
	}

	if (index == 0)
	{
		// The global climate ignores the script's type, position and radii
		GetGlobalClimate();
		return;
	}

	// A local climate around a point, between two radii
	const auto entity = registry.Create();
	auto& climate = registry.Assign<Climate>(entity);
	climate.index = index;
	climate.global = false;
	climate.info = info;
	climate.cellX = static_cast<uint16_t>(Ftol(position.x * 0.1));
	climate.cellZ = static_cast<uint16_t>(Ftol(position.y * 0.1));
	climate.innerRadius = std::min(radius1, radius2);
	climate.outerRadius = std::max(radius1, radius2);
	climate.maxStorms = static_cast<uint32_t>(Ftol((climate.outerRadius * k_MaxStormsPerUnit) + 1.0f));
	InitialiseClimate(climate);
}

void WeatherSystem::SetClimateRain(int32_t index, float desire, int32_t dryDays, int32_t rainingDays, int32_t raining)
{
	const auto entity = index == 0 ? GetGlobalClimate() : FindClimate(index);
	if (entity == entt::null)
	{
		return;
	}
	auto& climate = Locator::entitiesRegistry::value().Get<Climate>(entity);
	climate.rainDesire = desire;
	climate.dryDays = dryDays;
	climate.rainingDays = rainingDays;
	climate.raining = (raining & 1) != 0;
}

void WeatherSystem::SetClimateTemperature(int32_t index, float temperature, float targetTemperature)
{
	const auto entity = index == 0 ? GetGlobalClimate() : FindClimate(index);
	if (entity == entt::null)
	{
		return;
	}
	auto& climate = Locator::entitiesRegistry::value().Get<Climate>(entity);
	climate.temperature = temperature;
	climate.targetTemperature = targetTemperature;
}

void WeatherSystem::SetClimateWind(int32_t index, float windX, float windZ, float angle)
{
	const auto entity = index == 0 ? GetGlobalClimate() : FindClimate(index);
	if (entity == entt::null)
	{
		return;
	}
	auto& climate = Locator::entitiesRegistry::value().Get<Climate>(entity);
	climate.windX = windX;
	climate.windZ = windZ;
	climate.windAngle = angle;
}

void WeatherSystem::Update(uint32_t turn)
{
	_turn = turn;

	// Storms move on, then every cell of the atmosphere is stale
	UpdateStorms();
	_activeStorms.clear();
	Locator::entitiesRegistry::value().Each<const Storm>([this](entt::entity, const Storm& storm) {
		if (!storm.dead)
		{
			_activeStorms.push_back(storm);
		}
	});
	std::ranges::sort(_activeStorms, [](const Storm& a, const Storm& b) { return a.serial > b.serial; });
	++_stamp;
	if (_stamp == 0)
	{
		for (auto& cell : _grid)
		{
			cell.stamp = 0;
		}
		_stamp = 1;
	}

	// Every climate, with the season changing on a new day
	GetGlobalClimate();
	const auto day = Ftol(GetDayOfMonth(turn));
	const auto newDay = !_day.has_value() || *_day != day;
	if (newDay)
	{
		_day = day;
		_season = GetSeason(turn);
	}

	std::vector<entt::entity> climates;
	Locator::entitiesRegistry::value().Each<const Climate>(
	    [&climates](entt::entity entity, const Climate&) { climates.push_back(entity); });
	for (const auto entity : climates)
	{
		ProcessClimate(entity, newDay);
	}
}

// Storms age, fade in and out and travel towards their destination; dead ones linger a couple of turns
void WeatherSystem::UpdateStorms()
{
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> expired;
	registry.Each<Storm>([&expired](entt::entity entity, Storm& storm) {
		if (storm.dead)
		{
			if (++storm.deadTurns > k_DeadStormTurns)
			{
				expired.push_back(entity);
			}
			return;
		}

		storm.age += k_TurnDuration;
		if (storm.age >= storm.lastsFor)
		{
			storm.dead = true;
			return;
		}

		float fade = 1.0f;
		if (storm.age < storm.fadeTime)
		{
			fade = storm.age / storm.fadeTime;
		}
		else if (storm.age > storm.lastsFor - storm.fadeTime)
		{
			fade = (storm.lastsFor - storm.age) / storm.fadeTime;
		}
		storm.currentStrength = fade * storm.strength;
		storm.currentInnerRadius = fade * storm.innerRadius;

		storm.currentPosition = storm.position;
		if (storm.speed != 0.0f)
		{
			const auto toDestination = storm.destination - storm.currentPosition;
			const auto distance = std::sqrt((toDestination.x * toDestination.x) + (toDestination.z * toDestination.z));
			if (distance <= k_ArrivalDistance)
			{
				storm.currentPosition = storm.destination;
				storm.position = storm.destination;
				storm.arrived = true;
			}
			else
			{
				const auto step = k_TurnDuration * storm.speed < distance ? k_TurnDuration * storm.speed / distance : 1.0f;
				storm.currentPosition.x += toDestination.x * step;
				storm.currentPosition.z += toDestination.z * step;
				storm.position = storm.currentPosition;
				storm.arrived = false;
			}
		}
	});
	for (const auto entity : expired)
	{
		registry.Destroy(entity);
	}
}

// A climate's turn: its temperature, its storms drifting and clearing, and once a day its rain and wind
void WeatherSystem::ProcessClimate(entt::entity entity, bool newDay)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& climate = registry.Get<Climate>(entity);
	const auto& info = GetInfo(climate);

	if (_climateSystemEnabled)
	{
		ProcessTemperature(climate);
	}

	for (auto iter = climate.storms.begin(); iter != climate.storms.end();)
	{
		auto* storm = registry.Valid(*iter) ? registry.TryGet<Storm>(*iter) : nullptr;
		if (storm == nullptr || storm->dead)
		{
			iter = climate.storms.erase(iter);
			if (climate.storms.empty())
			{
				climate.raining = false;
			}
			continue;
		}

		// Storms drift with the wind where they are, which includes their own
		const auto wind = GetWeather(storm->position);
		storm->position.x += static_cast<float>(wind.windX) * k_StormDrift;
		storm->position.z += static_cast<float>(wind.windZ) * k_StormDrift;

		if (newDay)
		{
			// A storm leaving its climate, or (for the global climate) entering any climate, starts to clear and the
			// climate wants a new one
			const auto clearing = storm->lastsFor - (storm->fadeTime + storm->fadeTime);
			bool leave = false;
			if (!climate.global)
			{
				leave = Distance(CellCentre(climate), storm->position) > climate.outerRadius;
			}
			else
			{
				registry.Each<const Climate>([&leave, storm](entt::entity, const Climate& other) {
					leave = leave || Distance(CellCentre(other), storm->position) < other.outerRadius;
				});
			}
			if (leave && storm->age < clearing)
			{
				storm->age = clearing;
				climate.rainDesire = 1.0f;
			}

			// Out of raining days
			if (RainMax(info, _season) < static_cast<float>(climate.rainingDays) * 0.01f && storm->age < clearing)
			{
				storm->age = clearing;
			}
		}
		++iter;
	}

	if (newDay && _climateSystemEnabled)
	{
		ProcessRain(climate);
		ProcessWind(climate);
		if (climate.rainDesire == 1.0f && _stormCreationEnabled)
		{
			CreateStorm(entity);
		}
	}
}

// The temperature steps towards the time of day's
void WeatherSystem::ProcessTemperature(Climate& climate) const
{
	const auto& info = GetInfo(climate);
	const auto& sky = Locator::skySystem::value();
	const auto hour = RelativeHour(static_cast<float>(Ftol(sky.GetTime())), sky.GetDayNightTimes());
	const auto hourFactor = k_TemperatureByHour.at(std::clamp(Ftol(hour), 0, 23));
	const auto monthFactor = k_TemperatureByMonth.at(GetMonth(_turn));
	const auto minimum = TempMin(info, _season);
	const auto target = ((TempMax(info, _season) - minimum) * hourFactor * monthFactor) + minimum;
	climate.targetTemperature = target;

	// Black & White steps the whole way every turn, so once there the temperature swings back and forth across the
	// target ten times a second. It settles on the target here instead: the step scales with the temperature, so
	// near freezing, where rain turns to snow, the swing was too small to matter anyway.
	const auto step = static_cast<float>(std::abs(Ftol(target))) * k_TemperatureStep;
	if (std::abs(target - climate.temperature) <= step)
	{
		climate.temperature = target;
	}
	else
	{
		climate.temperature += climate.temperature < target ? step : -step;
	}
}

// Once a game day
void WeatherSystem::ProcessRain(Climate& climate)
{
	if (climate.raining)
	{
		++climate.rainingDays;
		return;
	}

	++climate.dryDays;
	climate.rainingDays = std::max(climate.rainingDays - 1, 0);

	// The monthly and hourly terms come from a table of rain figures that the game never loads, so they are 0
	const auto& info = GetInfo(climate);
	const auto fromMin = RandomFloat(RainMin(info, _season) * k_RainDesireRandom);
	const auto fromMax = RandomFloat(RainMax(info, _season) * k_RainDesireRandom);
	climate.rainDesire = std::min(climate.rainDesire + fromMax + fromMin, 1.0f);
}

// The wind, between the season's extremes by how close the climate is to rain
void WeatherSystem::ProcessWind(Climate& climate) const
{
	const auto& info = GetInfo(climate);
	const auto minimum = WindMin(info, _season);
	const auto maximum = WindMax(info, _season);
	const auto spread = (climate.rainDesire - 0.5) * k_WindSpread;
	// Only exactly halfway to rain does this reach 1: the wind is otherwise the sum of the season's extremes
	const auto weight = static_cast<float>(Ftol(std::exp(-(spread * spread))));

	const auto blend = [weight](float low, float high) { return high - ((high - low) * weight) + low; };
	climate.windX = blend(minimum * std::cos(climate.windAngle), maximum * std::cos(climate.windAngle));
	climate.windZ = blend(minimum * std::sin(climate.windAngle), maximum * std::sin(climate.windAngle));
}

// A cell for a new storm: near the middle of a local climate, anywhere for the global one
glm::ivec2 WeatherSystem::FindWhereToCreateStorm(const Climate& climate)
{
	auto& island = Locator::terrainSystem::value();
	glm::ivec2 cell {0, 0};
	for (int tries = 0; tries < k_StormPlacementTries; ++tries)
	{
		if (!climate.global)
		{
			const auto angle = RandomFloat(2.0f * std::numbers::pi_v<float>);
			const auto random = RandomFloat(1.0f);
			const auto distance = random * random * climate.innerRadius * 0.1f;
			cell.x = static_cast<int16_t>(Ftol((std::cos(angle) * distance) + climate.cellX));
			cell.y = static_cast<int16_t>(Ftol((std::sin(angle) * distance) + climate.cellZ));
		}
		else
		{
			cell.x = static_cast<int16_t>(Locator::rng::value().NextValue<int>(0, 0x1FF));
			cell.y = static_cast<int16_t>(Locator::rng::value().NextValue<int>(0, 0x1FF));
		}

		// The game's water test reads the cell's water bit, which is never 1: only points off the island retry
		if (cell.x >= 0 && cell.y >= 0 && cell.x < 512 && cell.y < 512 &&
		    island.FindCell({static_cast<uint16_t>(cell.x), static_cast<uint16_t>(cell.y)}) != nullptr)
		{
			break;
		}
	}
	return cell;
}

// A new storm, if the climate still has rain left and room for another
void WeatherSystem::CreateStorm(entt::entity climateEntity)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& climate = registry.Get<Climate>(climateEntity);
	const auto& info = GetInfo(climate);
	const auto rainMin = RainMin(info, _season);
	const auto rainMax = RainMax(info, _season);
	const auto rainedFor = static_cast<float>(climate.rainingDays) * 0.01f;

	const auto resetDesire = [&climate] {
		climate.rainDesire = 0.0f;
		climate.dryDays = 0;
	};
	if (!(rainMax > rainedFor))
	{
		resetDesire();
		return;
	}
	if (climate.storms.size() >= climate.maxStorms)
	{
		return;
	}

	const auto cell = FindWhereToCreateStorm(climate);
	auto& island = Locator::terrainSystem::value();
	const auto groundHeight = [&island](float x, float z) { return island.GetHeightAt({x, z}); };

	uint32_t radius;
	if (!climate.global)
	{
		const glm::vec3 point {cell.x * 10.0f, 0.0f, cell.y * 10.0f};
		const glm::vec3 centre {climate.cellX * 10.0f, 0.0f, climate.cellZ * 10.0f};
		const auto distance = Distance({point.x, groundHeight(point.x, point.z), point.z},
		                               {centre.x, groundHeight(centre.x, centre.z) + climate.height, centre.z});
		radius = static_cast<uint32_t>(Ftol(distance));
	}
	else
	{
		radius = static_cast<uint32_t>(Locator::rng::value().NextValue<int>(0, k_GlobalStormRadiusRange - 1));
	}
	radius = std::clamp(radius, k_MinStormRadius, k_MaxStormRadius);

	auto lastsFor = rainMax * 10.0f * 10.0f;
	if (!(rainMin < rainedFor))
	{
		lastsFor = (rainMax * 100.0f - static_cast<float>(climate.rainingDays)) * 10.0f;
	}
	lastsFor = std::max(lastsFor, k_MinStormLife);

	const auto temperature = static_cast<int8_t>(Ftol(climate.temperature));
	const auto heat = (static_cast<double>(temperature) - 30.0) * (1.0 / 15.0);
	auto cloud = static_cast<float>(std::exp(-(heat * heat)));
	if (temperature > 30 || climate.stormOvercast != 0)
	{
		cloud = static_cast<float>(climate.stormOvercast);
	}

	WeatherInfo effect;
	effect.temperature = temperature;
	effect.overcast = static_cast<int8_t>(Ftol(cloud * 100.0));
	if (temperature < 0)
	{
		effect.snow = 100;
		effect.rain = 0;
	}
	else
	{
		// Cold storms snow: the snow share falls away above freezing
		const auto cold = static_cast<double>(temperature) * 0.2;
		effect.snow = static_cast<int8_t>(Ftol(std::exp(-(cold * cold)) * 100.0));
		effect.rain = static_cast<int8_t>(100 - effect.snow);
	}
	effect.windX = static_cast<int8_t>(Ftol(climate.windX));
	effect.windZ = static_cast<int8_t>(Ftol(climate.windZ));

	if (lastsFor > 0.0f)
	{
		if (lastsFor < k_ShortStormLife)
		{
			lastsFor = k_MinStormLife;
		}

		const glm::vec3 position {cell.x * 10.0f, k_StormHeight, cell.y * 10.0f};
		const auto stormEntity = registry.Create();
		auto& storm = registry.Assign<Storm>(stormEntity);
		storm.position = position;
		storm.destination = position;
		storm.currentPosition = position;
		storm.speed = 1.0f;
		storm.arrived = true;
		storm.innerRadius = static_cast<float>(radius);
		storm.outerRadius = static_cast<float>(Ftol(static_cast<double>(radius) * k_StormOuterRadius));
		storm.fadeTime = k_StormFadeTime;
		storm.lastsFor = lastsFor;
		storm.strength = 1.0f;
		storm.cloudHeight = climate.stormCloudHeight;
		storm.effect = effect;
		storm.climate = climateEntity;
		storm.serial = _nextStormSerial++;

		climate.storms.insert(climate.storms.begin(), stormEntity);
		climate.raining = true;
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"),
		                    "Climate {} formed a storm at ({}, {}): radius {}, {}s, rain {} snow {} wind ({}, {})",
		                    climate.index, position.x, position.z, storm.outerRadius, lastsFor, effect.rain, effect.snow,
		                    effect.windX, effect.windZ);
	}
	resetDesire();
}

void WeatherSystem::ComputeCell(WeatherInfo& cell, int x, int z)
{
	// Calm air, then every live storm, newest first
	cell = WeatherInfo {};
	const glm::vec3 point {static_cast<float>(x) * k_CellSize, 0.0f, static_cast<float>(z) * k_CellSize};
	for (const auto& storm : _activeStorms)
	{
		ApplyStorm(storm, point, cell);
	}
	cell.stamp = _stamp;
}

const WeatherInfo& WeatherSystem::GetCell(int x, int z)
{
	static const WeatherInfo ambient {};
	if (x < 0 || z < 0 || x >= k_GridSize || z >= k_GridSize)
	{
		return ambient;
	}
	auto& cell = _grid[(z * k_GridSize) + x];
	if (cell.stamp != _stamp)
	{
		ComputeCell(cell, x, z);
	}
	return cell;
}

// The weather of the atmosphere's cell at a point
WeatherInfo WeatherSystem::GetWeather(const glm::vec3& position)
{
	auto info = GetCell(Ftol(position.x * 0.025f), Ftol(position.z * 0.025f));
	if (position.y > k_LowAltitude)
	{
		info.temperature =
		    position.y > k_HighAltitude
		        ? static_cast<int8_t>(info.temperature - k_HighAltitudeCooling)
		        : static_cast<int8_t>(info.temperature + Ftol((position.y - k_LowAltitude) * k_TemperatureLapse));
	}
	return info;
}

// The weather at a point, blended between the four nearest cells
WeatherInfo WeatherSystem::GetWeatherSmooth(const glm::vec3& position)
{
	const auto fx = position.x * 0.025f;
	const auto fz = position.z * 0.025f;
	const auto x = Ftol(fx);
	const auto z = Ftol(fz);
	const auto& c00 = GetCell(x, z);
	const auto& c10 = GetCell(x + 1, z);
	const auto& c01 = GetCell(x, z + 1);
	const auto& c11 = GetCell(x + 1, z + 1);

	const auto wx = Ftol((fx - static_cast<float>(x)) * k_EffectScale);
	const auto wz = Ftol((fz - static_cast<float>(z)) * k_EffectScale);
	auto info = Lerp(Lerp(c00, c10, wx), Lerp(c01, c11, wx), wz);
	info.stamp = c00.stamp;

	// High above the ground the weather gives way to calm air
	if (position.y > k_HighAltitude)
	{
		const auto weight = std::min(Ftol((position.y - k_HighAltitude) * k_HighAltitudeBlend), 256);
		const auto temperature = info.temperature;
		info = Lerp(info, WeatherInfo {}, weight);
		info.stamp = c00.stamp;
		info.temperature = static_cast<int8_t>(LerpByte(temperature, 0, weight) - k_HighAltitudeCooling);
	}
	else if (position.y > k_LowAltitude)
	{
		info.temperature = static_cast<int8_t>(info.temperature + Ftol((position.y - k_LowAltitude) * k_TemperatureLapse));
	}
	return info;
}

} // namespace openblack::ecs::systems
