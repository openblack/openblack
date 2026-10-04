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

#include <array>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

/// The weather at a point (LH3D WeatherInfo). Storms add to calm, temperature 0 air.
struct WeatherInfo
{
	int8_t temperature {0};
	/// Rain intensity, percent
	int8_t rain {0};
	/// Snowfall intensity, percent
	int8_t snow {0};
	/// Cloud cover, percent
	int8_t overcast {0};
	int8_t windX {0};
	int8_t windZ {0};
	int8_t snowCover {0};
	uint8_t stamp {0};
};

/// A region of the island that breeds storms (GClimate). Every island has a global climate covering the whole
/// map; the land scripts add local ones with CREATE_WEATHER_CLIMATE.
struct Climate
{
	/// Script index, 0 for the global climate
	int32_t index;
	bool global;
	/// Index into InfoConstants::climate (DETAIL_CLIMATE_INFO)
	uint32_t info;
	/// Centre on the map in cells (the high word of the original's 16.16 map coordinates)
	uint16_t cellX;
	uint16_t cellZ;
	/// Height of the centre above the land
	float height;
	float innerRadius;
	float outerRadius;

	/// Builds up once a game day; a storm forms when it reaches 1
	float rainDesire;
	/// Game days since the climate last rained
	int32_t dryDays;
	/// Raining days owed: counts up while raining, down while dry, and caps how long it may rain
	int32_t rainingDays;
	/// A storm of this climate is alive
	bool raining;

	float temperature;
	float targetTemperature;

	float windX;
	float windZ;
	/// Direction of the climate's wind, radians
	float windAngle;

	uint32_t maxStorms;
	/// Parameters handed to the storms this climate creates
	float stormCloudHeight;
	float stormSpeed;
	std::array<float, 5> stormLightning;
	uint8_t stormOvercast;

	/// Newest first
	std::vector<entt::entity> storms;
};

/// A moving weather system (GWeather / LH3DStorm). Within its outer radius it pulls the temperature towards its own
/// and adds its rain, snow, cloud and wind, at full strength inside its inner radius.
struct Storm
{
	/// Centre the climate drifts with the wind
	glm::vec3 position;
	glm::vec3 destination;
	float speed;
	bool arrived;

	float innerRadius;
	float outerRadius;
	/// Seconds to fade in and to fade out
	float fadeTime;
	/// Seconds the storm lives for
	float lastsFor;
	float strength;
	float cloudHeight;
	float age;

	/// State after the last update: centre, inner radius and strength scaled by the fade
	glm::vec3 currentPosition;
	float currentInnerRadius;
	float currentStrength;

	/// What the storm brings: temperature it pulls towards, then amounts added to the weather
	WeatherInfo effect;

	/// Creation order: later storms are applied on top of earlier ones
	uint32_t serial;
	bool dead;
	/// Turns since the storm died, it is removed after a couple
	uint8_t deadTurns;
	entt::entity climate;
};

} // namespace openblack::ecs::components
