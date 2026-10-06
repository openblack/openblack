/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SoundMap.h"

#include <cmath>

#include <algorithm>

#include <LNDFile.h>

#include "3D/LandIslandInterface.h"
#include "InfoConstants.h"

namespace openblack::audio
{

namespace
{
// Map coordinates are 16.16 fixed point cells, a cell is 10 world units
constexpr float k_MapCoordsPerUnit = 6553.6f;
constexpr float k_MapCoordsPerCell = 65536.0f;
constexpr float k_UnitsPerCell = 10.0f;
constexpr float k_CellsPerMapCoord = 1.0f / 65536.0f;
// Scanned cells are measured at their centre: half the map's cell size of 8, times 0x2000
constexpr int32_t k_HalfCell = 0x8000;
constexpr uint16_t k_MapSize = 0x200;

constexpr float k_Percent = 0.01f;
constexpr double k_WindVolumeStart = 15.0;
constexpr double k_WindVolumeRange = 1.0 / 30.0;
constexpr float k_MinimumWindVolume = 0.01f;

/// Truncation as the game does it, giving the most negative integer when out of range
int32_t Ftol(double value)
{
	if (value <= -2147483649.0 || value >= 2147483648.0)
	{
		return INT32_MIN;
	}
	return static_cast<int32_t>(value);
}

int16_t CellOf(int32_t coordinate)
{
	return static_cast<int16_t>(static_cast<uint32_t>(coordinate) >> 16);
}

/// Distance in world units of a map coordinate, as the game's scan computes it
double ToUnits(int32_t coordinate)
{
	return static_cast<double>(coordinate) * k_UnitsPerCell * k_CellsPerMapCoord;
}

} // namespace

void SoundMap::Update(const LandIslandInterface& island, const GSoundInfo& info, const Inputs& inputs)
{
	CalculateRadiusPointAndDistance(island, info, inputs.receiver);
	UpdateFromMap(island, _center);
	CalculateVolumes(island, info, inputs);
}

void SoundMap::CalculateRadiusPointAndDistance(const LandIslandInterface& island, const GSoundInfo& info,
                                               const glm::vec3& receiver)
{
	_receiver = receiver;
	_center.x = Ftol(static_cast<double>(receiver.x) * k_MapCoordsPerUnit);
	_center.z = Ftol(static_cast<double>(receiver.z) * k_MapCoordsPerUnit);
	_receiverHeight = receiver.y;

	const auto altitude =
	    GetAltitude(island, LandIslandInterface::ToMapCoords(receiver.x), LandIslandInterface::ToMapCoords(receiver.z));
	_heightAboveLand = static_cast<float>(static_cast<double>(_receiverHeight) - altitude);
	_radius = info.radiusForMinAtmosVolume;
}

void SoundMap::UpdateFromMap(const LandIslandInterface& island, const MapCoords& center)
{
	const auto radius = Ftol(static_cast<double>(_radius) / k_UnitsPerCell * k_MapCoordsPerCell);
	const MapCoords min {.x = center.x - radius, .z = center.z - radius};
	const MapCoords max {.x = center.x + radius, .z = center.z + radius};

	Reset();

	for (int32_t x = CellOf(min.x); x <= CellOf(max.x); ++x)
	{
		for (int32_t z = CellOf(min.z); z <= CellOf(max.z); ++z)
		{
			const auto type = GetAtmosType(island, static_cast<uint16_t>(x), static_cast<uint16_t>(z));
			const MapCoords cell {
			    .x = static_cast<int32_t>((static_cast<uint32_t>(static_cast<uint16_t>(x)) << 16) | k_HalfCell),
			    .z = static_cast<int32_t>((static_cast<uint32_t>(static_cast<uint16_t>(z)) << 16) | k_HalfCell),
			};
			AddAtmosType(type, cell);
		}
	}
}

void SoundMap::Reset()
{
	for (auto& scan : _scans)
	{
		scan.count = 0;
		scan.distanceSquared = 65535.0f;
	}
	_totalCount = 0;
}

// Counts a cell of a type, and keeps it if it is the nearest yet
void SoundMap::AddAtmosType(uint32_t type, const MapCoords& coords)
{
	auto& scan = _scans.at(type < k_AtmosTypeCount ? type : 0);

	const auto dz = ToUnits(_center.z) - ToUnits(coords.z);
	const auto dx = ToUnits(_center.x) - ToUnits(coords.x);
	const auto distanceSquared = (dx * dx) + (dz * dz);
	if (distanceSquared < static_cast<double>(scan.distanceSquared))
	{
		scan.distanceSquared = static_cast<float>(distanceSquared);
		scan.nearestX = static_cast<uint16_t>(Ftol(ToUnits(coords.x)));
		scan.nearestZ = static_cast<uint16_t>(Ftol(ToUnits(coords.z)));
	}
	++scan.count;

	if (type != 0)
	{
		++_totalCount;
	}
}

uint32_t SoundMap::GetAtmosType(const LandIslandInterface& island, uint16_t x, uint16_t z)
{
	const auto* cell = x < k_MapSize && z < k_MapSize ? island.FindCell({x, z}) : nullptr;
	if (cell == nullptr)
	{
		// Off the map is open sea
		return static_cast<uint32_t>(AtmosType::Sea);
	}
	return (cell->flags >> 2) & 0xF;
}

double SoundMap::GetAltitude(const LandIslandInterface& island, int32_t x, int32_t z)
{
	return island.GetAltitude(x, z);
}

float SoundMap::DistanceFade(const GSoundInfo& info, AtmosType type) const
{
	const auto distance = std::sqrt(static_cast<double>(_scans.at(static_cast<size_t>(type)).distanceSquared));
	if (distance <= static_cast<double>(info.radiusForMaxAtmosVolume))
	{
		return 1.0f;
	}
	const auto fade = 1.0 - ((distance - info.radiusForMaxAtmosVolume) /
	                         (static_cast<double>(info.radiusForMinAtmosVolume) - info.radiusForMaxAtmosVolume));
	return fade >= 0.0 ? static_cast<float>(fade) : 0.0f;
}

double SoundMap::HeightFade(const LandIslandInterface& island, const GSoundInfo& info, float x, float z) const
{
	const auto height = static_cast<double>(_receiver.y) -
	                    GetAltitude(island, LandIslandInterface::ToMapCoords(x), LandIslandInterface::ToMapCoords(z));
	if (height < static_cast<double>(info.normalAtmosFadeStartHeight))
	{
		return 1.0;
	}

	const auto stored = static_cast<double>(static_cast<float>(height));
	if (stored > static_cast<double>(info.normalAtmosFadeEndHeight))
	{
		return 0.0;
	}
	const auto fade = 1.0 - ((stored - info.normalAtmosFadeStartHeight) /
	                         (static_cast<double>(info.normalAtmosFadeEndHeight) - info.normalAtmosFadeStartHeight));
	return fade >= 0.0 ? fade : 0.0;
}

float SoundMap::StratosphereVolume(const GSoundInfo& info) const
{
	const auto height = static_cast<double>(_heightAboveLand);
	if (height < info.atmosphereHeight)
	{
		return 0.0f;
	}
	if (height < info.atmosphereMaxVolHeight)
	{
		const auto rise =
		    (height - info.atmosphereHeight) / (static_cast<double>(info.atmosphereMaxVolHeight) - info.atmosphereHeight);
		return rise > 1.0 ? 1.0f : static_cast<float>(rise);
	}
	if (height < info.spaceHeight)
	{
		const auto fall = 1.0 - ((height - info.atmosphereMaxVolHeight) /
		                         (static_cast<double>(info.spaceHeight) - info.atmosphereMaxVolHeight));
		return fall < 0.0 ? 0.0f : static_cast<float>(fall);
	}
	return 0.0f;
}

void SoundMap::CalculateVolumes(const LandIslandInterface& island, const GSoundInfo& info, const Inputs& inputs)
{
	const auto stratosphere = StratosphereVolume(info);

	// Full night only past dusk
	const auto nightDelta = static_cast<double>(inputs.skyType) - 1.0;
	const auto night = nightDelta < 0.0 ? 0.0f : static_cast<float>(nightDelta);

	// Wildlife fades out as the weather worsens
	const auto& weather = inputs.weather;
	auto clear = weather.snow > 0 ? 1.0 - (static_cast<double>(weather.snow) * k_Percent) : 1.0;
	clear -= static_cast<double>(weather.rain) * k_Percent;
	clear = std::max(clear, 0.0);
	const auto cutoff = static_cast<float>(static_cast<double>(info.weatherPercentageForMaxFade) * k_Percent);
	const auto badWeather = 1.0 - clear;
	const auto wildlifeWeather =
	    badWeather < static_cast<double>(cutoff) ? static_cast<float>(1.0 - (badWeather / cutoff)) : 0.0f;

	const auto windSquared =
	    (static_cast<int32_t>(weather.windX) * weather.windX) + (static_cast<int32_t>(weather.windZ) * weather.windZ);
	const auto windStrength = (std::sqrt(static_cast<double>(windSquared)) - k_WindVolumeStart) * k_WindVolumeRange;
	auto wind = static_cast<float>(windStrength);
	if (windStrength > 1.0)
	{
		wind = 1.0f;
	}
	else if (wind < k_MinimumWindVolume)
	{
		wind = 0.0f;
	}

	_volumes[static_cast<size_t>(AtmosType::Stratosphere)] = stratosphere;
	_volumes[static_cast<size_t>(AtmosType::None)] = 0.0f;

	const auto nearestHeightFade = [this, &island, &info](AtmosType type) {
		const auto& scan = _scans.at(static_cast<size_t>(type));
		return HeightFade(island, info, static_cast<float>(scan.nearestX), static_cast<float>(scan.nearestZ));
	};

	// Land painted types
	for (auto i = static_cast<size_t>(AtmosType::Sea); i <= static_cast<size_t>(AtmosType::RunningWater); ++i)
	{
		const auto type = static_cast<AtmosType>(i);
		if (_scans.at(i).count == 0)
		{
			_volumes.at(i) = 0.0f;
			continue;
		}
		auto volume = DistanceFade(info, type);
		if (k_AtmosTypeInfos.at(i).wildlife)
		{
			volume = static_cast<float>((static_cast<double>(volume) * wildlifeWeather) * (1.0 - night));
		}
		_volumes.at(i) = static_cast<float>(nearestHeightFade(type) * volume);
	}

	// The shore takes over from the open sea as it gets closer
	auto coastal = 0.0f;
	if (_scans[static_cast<size_t>(AtmosType::Coastal)].count == 0)
	{
		_volumes[static_cast<size_t>(AtmosType::Coastal)] = 0.0f;
	}
	else
	{
		coastal = DistanceFade(info, AtmosType::Coastal);
		_volumes[static_cast<size_t>(AtmosType::Coastal)] = static_cast<float>(nearestHeightFade(AtmosType::Coastal) * coastal);
	}

	if (_scans[static_cast<size_t>(AtmosType::Sea)].count == 0)
	{
		_volumes[static_cast<size_t>(AtmosType::Sea)] = 0.0f;
	}
	else
	{
		auto sea = DistanceFade(info, AtmosType::Sea);
		const auto limit = 1.0 - coastal;
		if (!(static_cast<double>(sea) < limit))
		{
			sea = static_cast<float>(limit);
		}
		_volumes[static_cast<size_t>(AtmosType::Sea)] = static_cast<float>(nearestHeightFade(AtmosType::Sea) * sea);
	}

	// Night sounds only over land
	const auto seaFraction = _totalCount != 0 ? static_cast<double>(_scans[static_cast<size_t>(AtmosType::Sea)].count) /
	                                                static_cast<double>(_totalCount)
	                                          : 0.0;
	const auto land = static_cast<float>(1.0 - seaFraction);
	const auto receiverHeightFade = HeightFade(island, info, _receiver.x, _receiver.z);
	_volumes[static_cast<size_t>(AtmosType::Night)] =
	    static_cast<float>(((receiverHeightFade * land) * wildlifeWeather) * night);

	const auto rain = static_cast<double>(weather.rain);
	_volumes[static_cast<size_t>(AtmosType::Rain)] = rain < static_cast<double>(info.weatherPercentageForMaxWeatherVolume)
	                                                     ? static_cast<float>(rain / info.weatherPercentageForMaxWeatherVolume)
	                                                     : 1.0f;

	_volumes[static_cast<size_t>(AtmosType::Wind)] = wind < 1.0f ? wind : 1.0f;
}

} // namespace openblack::audio
