/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/MapCoords.h"
#include "Common/GUtilsDistance.h"
#include "ECS/Components/Firefly.h"

/// The fireflies' rules: when they come out and go home, where they fly, how they drift and fade, and the land's table of
/// rewards for catching one. Pure maths on plain values, tested on its own; the firefly system applies it to the land.
namespace openblack::fireflies
{

/// A land holds at most this many; each nightfall after a morning makes new ones up to it
inline constexpr size_t k_DefaultMaximum = 50;
/// The searches for somewhere to hover and somewhere to hide look this far each way across the land
inline constexpr float k_SearchReach = 300.0f;
/// The searches go over the land's cells, this wide
inline constexpr float k_SearchCellSize = 10.0f;
/// With nowhere to go, a firefly goes this far east (along the map's x) of where it is
inline constexpr float k_NowhereOffset = 15.0f;
/// ...and hovers this high above the ground there; going home it settles on the ground
inline constexpr float k_NowhereHoverHeight = 4.0f;
/// It hovers this far above the top of the building it found
inline constexpr float k_AboveBuilding = 2.0f;
/// A flight is its length across the ground over this many metres a second, times the firefly's speed, and never shorter
/// than the shortest
inline constexpr float k_FlightMetresPerSecond = 3.0f;
inline constexpr float k_ShortestFlight = 0.5f;
/// Each firefly's two speeds are drawn between these
inline constexpr float k_LeastSpeed = 0.6f;
inline constexpr float k_SpeedSpread = 0.8f;
inline constexpr float k_TwoPi = 6.2831855f;
/// The two loops it drifts on while out: a slow wide one and a quick small one, each half as tall as it is wide. Each
/// loop turns by two angles, at these rates times its speed
inline constexpr float k_SlowLoopRate = 0.1f;
inline constexpr float k_QuickLoopRateAround = 1.0f;
inline constexpr float k_QuickLoopRateUp = 1.21f;
inline constexpr float k_SlowLoopRadius = 8.0f;
inline constexpr float k_QuickLoopRadius = 1.0f;
inline constexpr float k_LoopHeightShare = 0.5f;
/// The drift grows over the first fifth of the flight out and dies away over the last fifth of the flight home
inline constexpr float k_DriftGrowsUntil = 0.2f;
inline constexpr float k_DriftGrowth = 5.0f;
inline constexpr float k_DriftDiesFrom = 0.8f;
inline constexpr float k_DriftDying = 5.0000005f;
/// It is drawn at this opacity of 255 within the near distance, fading by the square of the distance to nothing at the
/// far one, beyond which it isn't drawn
inline constexpr float k_NearSquared = 10000.0f;
inline constexpr float k_FarSquared = 90000.0f;
inline constexpr float k_Opacity = 190.0f;
/// Its sprite: half as wide as it is across, and the picture it shows on the game's third sprite sheet of 8 by 8
inline constexpr float k_SpriteHalfSize = 0.3f;
inline constexpr uint32_t k_SpritePicture = 37;
inline constexpr uint32_t k_SheetPictures = 8;
/// The reward table has a weight for every magic type
inline constexpr size_t k_RewardKinds = 42;

/// What the night sends the fireflies to do this turn
enum class Sending : uint8_t
{
	Nothing,
	Out,
	Home,
};

using State = ecs::components::Firefly::State;
using Drift = ecs::components::FireflyDrift;
using Firefly = ecs::components::Firefly;

/// A firefly's drift drawn from the game's random numbers, in the game's order: its two speeds, then its six angles
template <typename FloatRand>
[[nodiscard]] Drift DrawDrift(FloatRand&& floatRand)
{
	Drift drift;
	drift.flightSpeed = floatRand(k_SpeedSpread) + k_LeastSpeed;
	drift.quickSpeed = floatRand(k_SpeedSpread) + k_LeastSpeed;
	for (auto& phase : drift.phases)
	{
		phase = floatRand(k_TwoPi);
	}
	return drift;
}

/// A new firefly hiding at a spot
[[nodiscard]] Firefly Make(const map_coords::MapCoords& spot, const Drift& drift);

/// The sky's stage at an hour of the shown clock, as the fireflies read it: 2 at night, 1 at dusk and dawn, 0 by day,
/// between them as the sky changes. `times` are the hours of full night, the start and end of dusk, and full day.
[[nodiscard]] float NightStage(float visualHour, const std::array<float, 4>& times);
/// The evening, once the sky darkens past dusk, sends them out until midnight; the morning, once it brightens past dawn,
/// sends them home until noon
[[nodiscard]] Sending SendingAt(float visualHour, const std::array<float, 4>& times);

/// How many seconds a flight of so many metres across the ground takes at a speed
[[nodiscard]] float FlightSeconds(float metres, float speed);
/// The share of the way along a flight at a share of its time, easing in and out
[[nodiscard]] float Ease(float progress);

/// Sets a firefly off to hover at a spot, from its hiding place: `metres` is the flight's length across the ground, and
/// `homePoint` its hiding place in the world, where it is drawn from
void FlyOut(Firefly& firefly, const map_coords::MapCoords& hover, float metres, glm::vec3 homePoint);
/// Sets a firefly off to hide at a spot, from where it hovered
void FlyHome(Firefly& firefly, const map_coords::MapCoords& home, float metres);

/// Where a firefly goes in a turn: the place it moves to, if it moves
struct Step
{
	std::optional<map_coords::MapCoords> moveTo;
	/// The share of the way between the two ends, for a place along the flight
	std::optional<float> along;
};
/// A game turn of `seconds` along its flight. A flight that ends puts it at the far end: hovering, or resting at home.
/// Along the way, the place is the given share of the way between where it flies from and to, which the caller works
/// out in the world (`along`, with `moveTo` none). The place it was is kept as the previous one
[[nodiscard]] Step Advance(Firefly& firefly, float seconds);

/// How much of its drift a firefly shows
[[nodiscard]] float Amplitude(State state, float progress);
/// Where the two loops put a firefly from the middle of its drift, after its loops have turned for `clock` seconds
[[nodiscard]] glm::vec3 DriftOffset(const Drift& drift, float clock, float amplitude);

/// The opacity, of 255, a firefly is drawn at this far from the camera (squared), or none beyond its reach
[[nodiscard]] std::optional<uint8_t> OpacityAt(float distanceSquared);

/// The cells the searches cover across a reach: a square of so many cells a side
[[nodiscard]] int32_t SearchCells(float reach);

/// A thing a search met, with its spot
struct Found
{
	entt::entity thing {entt::null};
	map_coords::MapCoords spot;
};

/// The searches for a building to hover by or a tree or rock to hide in: a square spiral of cells out from the start,
/// each cell on the map looked at on a coin toss, and in a cell each thing that passes nearer than the best so far
/// taken, with a one in three chance of looking no further in that cell. So roughly the nearest.
/// `visit(cell, meet)` calls `meet(thing, spot)` for each thing in the cell that would do, in the cell's order, and
/// stops when it returns true; `rand(n)` is the game's random number below n.
template <typename Visit, typename Rand>
[[nodiscard]] std::optional<Found> SearchRoughlyNearest(const map_coords::MapCoords& start, float reach, Visit&& visit,
                                                        Rand&& rand)
{
	std::optional<Found> best;
	float bestMetres = 0.0f;
	auto cell = start;
	map_coords::Spiral spiral;
	for (int32_t left = SearchCells(reach); left > 0; --left)
	{
		if (map_coords::InBounds(cell) && rand(2u) != 0)
		{
			visit(map_coords::Cell(cell), [&](entt::entity thing, const map_coords::MapCoords& spot) {
				const float metres = gutils::GetDistanceInMetres(start, spot);
				if (best.has_value() && !(metres < bestMetres))
				{
					return false;
				}
				best = Found {.thing = thing, .spot = spot};
				bestMetres = metres;
				return rand(3u) == 0;
			});
		}
		map_coords::AddCells(cell, spiral.Next());
	}
	return best;
}

/// The spot east of a place a firefly goes to with nowhere better: hovering above the ground, or on it going home
[[nodiscard]] map_coords::MapCoords NowhereFrom(const map_coords::MapCoords& from, float altitude);

/// The land's table of what a caught firefly gives: a weight for each magic type, and their running totals
class RewardTable
{
public:
	/// A magic type's weight, by its number; numbers beyond the table are ignored. The running totals follow
	void SetWeight(size_t kind, float weight);
	/// As a land closes: every weight goes to nothing. The running totals stay as they were until the next weight is set,
	/// as in the game
	void ClearWeights();
	[[nodiscard]] float Total() const { return _totals.back(); }
	[[nodiscard]] float Weight(size_t kind) const { return _weights.at(kind); }
	/// The magic type, by number, that a roll of `roll` (from 0 to the total) gives: none for a roll of nothing, past the
	/// table, or the first kind, which is no miracle
	[[nodiscard]] std::optional<size_t> Pick(float roll) const;

private:
	std::array<float, k_RewardKinds> _weights {};
	std::array<float, k_RewardKinds> _totals {};
};

} // namespace openblack::fireflies
