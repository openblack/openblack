/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <optional>
#include <vector>

#include <entt/entity/entity.hpp>

#include "ECS/Systems/WeatherSystemInterface.h"

#ifndef LOCATOR_IMPLEMENTATIONS
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{
struct GClimateInfo;
}

namespace openblack::ecs::systems
{

/// Port of Black & White's climate and atmosphere simulation.
///
/// Once a game day each climate's desire to rain grows at random; when it reaches 1 the climate creates a storm at a
/// random point in its area. Storms drift with the wind, fade in, last a while and fade out. The atmosphere is a grid
/// of 128x128 cells of 40 units whose weather is calm air plus every storm overlapping the cell.
class WeatherSystem final: public WeatherSystemInterface
{
public:
	WeatherSystem();

	void Reset() override;

	void CreateClimate(int32_t index, uint32_t info, glm::vec2 position, float radius1, float radius2) override;
	void SetClimateRain(int32_t index, float desire, int32_t dryDays, int32_t rainingDays, int32_t raining) override;
	void SetClimateTemperature(int32_t index, float temperature, float targetTemperature) override;
	void SetClimateWind(int32_t index, float windX, float windZ, float angle) override;
	void SetClimateSystemEnabled(bool enabled) override { _climateSystemEnabled = enabled; }
	void SetStormCreationEnabled(bool enabled) override { _stormCreationEnabled = enabled; }
	[[nodiscard]] bool IsClimateSystemEnabled() const override { return _climateSystemEnabled; }
	[[nodiscard]] bool IsStormCreationEnabled() const override { return _stormCreationEnabled; }

	void ForceStorm(const ForcedStorm& storm) override;
	void ClearStorms() override;
	void StrikeLightning(bool bolt) override;

	void Update(uint32_t turn) override;

	[[nodiscard]] components::WeatherInfo GetWeather(const glm::vec3& position) override;
	[[nodiscard]] components::WeatherInfo GetWeatherSmooth(const glm::vec3& position) override;
	[[nodiscard]] float GetOvercast(const glm::vec3& position) override;
	[[nodiscard]] uint8_t GetLightningFlash(const glm::vec3& camera) const override;

	[[nodiscard]] float GetDaysFromStart(uint32_t turn) const override;
	[[nodiscard]] uint32_t GetSeason(uint32_t turn) const override;

private:
	static constexpr int k_GridSize = 128;
	static constexpr float k_CellSize = 40.0f;

	[[nodiscard]] entt::entity FindClimate(int32_t index) const;
	entt::entity GetGlobalClimate();
	void InitialiseClimate(components::Climate& climate) const;

	void UpdateStorms();
	void ProcessClimate(entt::entity entity, bool newDay);
	void ProcessTemperature(components::Climate& climate) const;
	void ProcessRain(components::Climate& climate);
	void ProcessWind(components::Climate& climate) const;
	void CreateStorm(entt::entity climateEntity);
	[[nodiscard]] glm::ivec2 FindWhereToCreateStorm(const components::Climate& climate);

	[[nodiscard]] const components::WeatherInfo& GetCell(int x, int z);
	void ComputeCell(components::WeatherInfo& cell, int x, int z);

	[[nodiscard]] const GClimateInfo& GetInfo(const components::Climate& climate) const;
	[[nodiscard]] float GetDayOfYear(uint32_t turn) const;
	[[nodiscard]] uint32_t GetMonth(uint32_t turn) const;
	[[nodiscard]] float GetDayOfMonth(uint32_t turn) const;
	[[nodiscard]] float RandomFloat(float max) const;

	bool _climateSystemEnabled {true};
	bool _stormCreationEnabled {true};

	uint32_t _turn {0};
	std::optional<int32_t> _day;
	/// Season the climates use, updated when the day changes
	uint32_t _season {0};

	double _startTurn;
	float _secondsPerTurn;

	/// Live storms as they stood at the start of the turn, newest first
	std::vector<components::Storm> _activeStorms;
	uint32_t _nextStormSerial {0};
	/// The storm forced over the island, if there is one
	entt::entity _forcedStorm {entt::null};
	std::vector<components::WeatherInfo> _grid;
	uint8_t _stamp {1};
};

} // namespace openblack::ecs::systems
