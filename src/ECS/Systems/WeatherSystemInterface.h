/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <span>

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "ECS/Components/Weather.h"

namespace openblack::ecs::systems
{

/// A storm laid over the whole island from the debug tools
struct ForcedStorm
{
	/// What it brings: the temperature it pulls towards, and the rain, snow, cloud and wind it adds, in percent
	components::WeatherInfo effect;
	/// How long it lasts, and how long it takes to come in and to clear, in seconds
	float seconds {600.0f};
	float fadeSeconds {5.0f};
	/// The least and most seconds between its flashes of lightning with thunder, and between those with a bolt; none
	/// for a most of 0
	glm::vec2 thunderWait {0.0f};
	glm::vec2 boltWait {0.0f};
	/// How high its clouds are and how fast its rain falls; the global climate's when 0
	float cloudHeight {0.0f};
	float rainSpeed {0.0f};
};

/// The island's weather: climates breed storms that drift with the wind and bring rain, snow and wind
class WeatherSystemInterface
{
public:
	virtual ~WeatherSystemInterface() = default;

	/// Removes every climate and storm, ready for a new island
	virtual void Reset() = 0;

	// Land script commands (CREATE_WEATHER_CLIMATE*). Index 0 is the global climate.
	virtual void CreateClimate(int32_t index, uint32_t info, glm::vec2 position, float radius1, float radius2) = 0;
	virtual void SetClimateRain(int32_t index, float desire, int32_t dryDays, int32_t rainingDays, int32_t raining) = 0;
	virtual void SetClimateTemperature(int32_t index, float temperature, float targetTemperature) = 0;
	virtual void SetClimateWind(int32_t index, float windX, float windZ, float angle) = 0;

	// Game script commands
	virtual void SetClimateSystemEnabled(bool enabled) = 0;
	virtual void SetStormCreationEnabled(bool enabled) = 0;
	[[nodiscard]] virtual bool IsClimateSystemEnabled() const = 0;
	[[nodiscard]] virtual bool IsStormCreationEnabled() const = 0;

	// Debug tools
	/// Lays a storm over the whole island, in place of any forced before it
	virtual void ForceStorm(const ForcedStorm& storm) = 0;
	/// A storm clears from the strength it has now over its fading time, as though at the end of its life
	virtual void EndStorm(entt::entity storm) = 0;
	/// Every storm clears
	virtual void ClearStorms() = 0;
	/// Every storm with lightning flashes on its next turn, with a bolt or with thunder
	virtual void StrikeLightning(bool bolt) = 0;

	/// One game turn
	virtual void Update(uint32_t turn) = 0;
	/// The storms alive after the last turn, newest first
	[[nodiscard]] virtual std::span<const components::Storm> GetActiveStorms() const = 0;

	/// Weather in the atmosphere cell containing a point
	[[nodiscard]] virtual components::WeatherInfo GetWeather(const glm::vec3& position) = 0;
	/// Weather interpolated between the four nearest cells, as heard and seen at the camera. High above the
	/// ground it blends back towards calm air.
	[[nodiscard]] virtual components::WeatherInfo GetWeatherSmooth(const glm::vec3& position) = 0;
	/// The cloud cover seen from a point, 0 for a clear sky and 1 for a full one, a little over in a storm
	[[nodiscard]] virtual float GetOvercast(const glm::vec3& position) = 0;
	/// How brightly lightning lights the land at the camera, 0 to 255: the last flash of the nearest storm, while the
	/// camera is inside a storm
	[[nodiscard]] virtual uint8_t GetLightningFlash(const glm::vec3& camera) const = 0;

	// Calendar the climates follow: a game year lasts 36000 turns from 5 May 1998
	[[nodiscard]] virtual float GetDaysFromStart(uint32_t turn) const = 0;
	/// 0 spring, 1 summer, 2 autumn, 3 winter
	[[nodiscard]] virtual uint32_t GetSeason(uint32_t turn) const = 0;
};

} // namespace openblack::ecs::systems
