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
#include <string_view>

#include <glm/vec3.hpp>

namespace openblack
{
class LandIslandInterface;
struct GSoundInfo;
} // namespace openblack

namespace openblack::audio
{

/// Ambient sound categories. SEA to RUNNING_WATER are painted into the landscape (bits 2-5 of LNDCell::flags),
/// the others depend on altitude, time of day and weather.
enum class AtmosType : uint8_t
{
	None,
	Sea,
	StillFreshWater,
	Coastal,
	Jungle,
	Arctic,
	Desert,
	Countryside,
	Swamp,
	RunningWater,
	Stratosphere,
	Night,
	Rain,
	Wind,

	_Count
};

constexpr auto k_AtmosTypeCount = static_cast<size_t>(AtmosType::_Count);

struct AtmosTypeInfo
{
	std::string_view name;
	/// Sound bank relative to the Audio directory
	std::string_view bank;
	/// Wildlife ambience: silenced by bad weather and at night
	bool wildlife;
};

constexpr std::array<AtmosTypeInfo, k_AtmosTypeCount> k_AtmosTypeInfos = {{
    {.name = "ATMOS_TYPE_NONE", .bank = "", .wildlife = false},
    {.name = "ATMOS_TYPE_SEA", .bank = "sfx/atmos/ocean.sad", .wildlife = false},
    {.name = "ATMOS_TYPE_STILL_FRESH_WATER", .bank = "sfx/atmos/lake.sad", .wildlife = false},
    {.name = "ATMOS_TYPE_COASTAL", .bank = "sfx/atmos/shore.sad", .wildlife = false},
    {.name = "ATMOS_TYPE_JUNGLE", .bank = "sfx/atmos/jungle.sad", .wildlife = true},
    {.name = "ATMOS_TYPE_ARCTIC", .bank = "sfx/atmos/arctic.sad", .wildlife = false},
    {.name = "ATMOS_TYPE_DESERT", .bank = "sfx/atmos/desert.sad", .wildlife = true},
    {.name = "ATMOS_TYPE_COUNTRYSIDE", .bank = "sfx/atmos/country.sad", .wildlife = true},
    {.name = "ATMOS_TYPE_SWAMP", .bank = "sfx/atmos/swamp.sad", .wildlife = true},
    {.name = "ATMOS_TYPE_RUNNING_WATER", .bank = "sfx/atmos/stream.sad", .wildlife = false},
    {.name = "ATMOS_TYPE_STRATOSPHERE", .bank = "sfx/atmos/high.sad", .wildlife = false},
    {.name = "ATMOS_TYPE_NIGHT", .bank = "sfx/atmos/night.sad", .wildlife = true},
    {.name = "ATMOS_TYPE_RAIN", .bank = "sfx/atmos/rain.sad", .wildlife = false},
    {.name = "ATMOS_TYPE_WIND", .bank = "sfx/atmos/wind.sad", .wildlife = false},
}};

/// The bytes of the LH3D WeatherInfo cell at the listener that the ambience reads
struct AtmosWeather
{
	/// Rain intensity in percent
	int8_t rain {0};
	/// Snowfall intensity in percent
	int8_t snow {0};
	int8_t windX {0};
	int8_t windZ {0};
};

/// Port of GSoundMap: decides how loud each ambience type should be from the land around the listener.
///
/// Each game turn the listener (the camera) scans the land cells within GSoundInfo::radiusForMinAtmosVolume,
/// recording per type how many cells it found and which one is nearest. Every type then fades with the distance
/// to its nearest cell and with the camera's height above the ground there.
///
/// Computations reproduce the original x87 code, which evaluates in double precision and rounds to float on
/// every store, so they are carried out in double here with float casts where the original stored a value.
class SoundMap
{
public:
	struct Inputs
	{
		/// Camera position
		glm::vec3 receiver;
		AtmosWeather weather;
		/// LH3DSky sky type: 0 day, 1 dusk, 2 night
		float skyType;
	};

	/// Per type scan result (AtmosMapTypeInfo)
	struct TypeScan
	{
		uint16_t count;
		float distanceSquared;
		/// World position of the nearest cell's centre, truncated
		uint16_t nearestX;
		uint16_t nearestZ;
	};

	void Update(const LandIslandInterface& island, const GSoundInfo& info, const Inputs& inputs);

	/// Target volume of each type, 0 to 1
	[[nodiscard]] const std::array<float, k_AtmosTypeCount>& GetVolumes() const { return _volumes; }
	[[nodiscard]] const std::array<TypeScan, k_AtmosTypeCount>& GetScans() const { return _scans; }
	[[nodiscard]] float GetHeightAboveLand() const { return _heightAboveLand; }
	[[nodiscard]] float GetReceiverHeight() const { return _receiverHeight; }

	/// LH3DIsland::GetAltitude: interpolated land height at a map position in 1/65536 cell units
	[[nodiscard]] static double GetAltitude(const LandIslandInterface& island, int32_t x, int32_t z);
	/// Terrain::GetAtmosType
	[[nodiscard]] static uint32_t GetAtmosType(const LandIslandInterface& island, uint16_t x, uint16_t z);

private:
	struct MapCoords
	{
		int32_t x;
		int32_t z;
	};

	void CalculateRadiusPointAndDistance(const LandIslandInterface& island, const GSoundInfo& info, const glm::vec3& receiver);
	void UpdateFromMap(const LandIslandInterface& island, const MapCoords& center);
	void Reset();
	void AddAtmosType(uint32_t type, const MapCoords& coords);
	void CalculateVolumes(const LandIslandInterface& island, const GSoundInfo& info, const Inputs& inputs);

	[[nodiscard]] float DistanceFade(const GSoundInfo& info, AtmosType type) const;
	[[nodiscard]] double HeightFade(const LandIslandInterface& island, const GSoundInfo& info, float x, float z) const;
	[[nodiscard]] float StratosphereVolume(const GSoundInfo& info) const;

	std::array<TypeScan, k_AtmosTypeCount> _scans {};
	/// Number of scanned cells with any ambience
	uint16_t _totalCount {0};
	std::array<float, k_AtmosTypeCount> _volumes {};
	glm::vec3 _receiver {0.0f, 0.0f, 0.0f};
	MapCoords _center {.x = 0, .z = 0};
	float _radius {0.0f};
	float _heightAboveLand {0.0f};
	float _receiverHeight {0.0f};
};

} // namespace openblack::audio
