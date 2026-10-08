/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Fireflies.h"

#include <cmath>

using namespace openblack;
using namespace openblack::fireflies;

namespace
{
/// Noon splits the shown day into its morning and its evening
constexpr float k_Noon = 12.0f;
constexpr float k_Day = 24.0f;
} // namespace

Firefly fireflies::Make(const map_coords::MapCoords& spot, const Drift& drift)
{
	return {
	    .state = State::Resting,
	    .at = spot,
	    .previous = spot,
	    .home = spot,
	    .hover = {},
	    .drawn = glm::vec3(0.0f),
	    .clock = 0.0f,
	    .amplitude = 0.0f,
	    .progress = 0.0f,
	    .flightSeconds = k_ShortestFlight,
	    .drift = drift,
	    .hidden = true,
	};
}

float fireflies::NightStage(float visualHour, const std::array<float, 4>& times)
{
	const float hour = visualHour > k_Noon ? k_Day - visualHour : visualHour;
	const auto& [nightFull, duskStart, duskEnd, dayFull] = times;
	if (hour < nightFull)
	{
		return 2.0f;
	}
	if (hour < duskStart)
	{
		return 2.0f - ((hour - nightFull) / (duskStart - nightFull));
	}
	if (hour < duskEnd)
	{
		return 1.0f;
	}
	if (hour < dayFull)
	{
		return 1.0f - ((hour - duskEnd) / (dayFull - duskEnd));
	}
	return 0.0f;
}

Sending fireflies::SendingAt(float visualHour, const std::array<float, 4>& times)
{
	if (visualHour > k_Noon && NightStage(visualHour, times) > 1.0f)
	{
		return Sending::Out;
	}
	if (visualHour < k_Noon && NightStage(visualHour, times) < 1.0f)
	{
		return Sending::Home;
	}
	return Sending::Nothing;
}

float fireflies::FlightSeconds(float metres, float speed)
{
	const float seconds = metres / (k_FlightMetresPerSecond * speed);
	return seconds <= k_ShortestFlight ? k_ShortestFlight : seconds;
}

float fireflies::Ease(float progress)
{
	return (3.0f - (progress + progress)) * progress * progress;
}

void fireflies::FlyOut(Firefly& firefly, const map_coords::MapCoords& hover, float metres, glm::vec3 homePoint)
{
	firefly.state = State::FlyingOut;
	firefly.hover = hover;
	firefly.previous = firefly.at;
	firefly.drawn = homePoint;
	firefly.progress = 0.0f;
	firefly.flightSeconds = FlightSeconds(metres, firefly.drift.flightSpeed);
}

void fireflies::FlyHome(Firefly& firefly, const map_coords::MapCoords& home, float metres)
{
	firefly.state = State::FlyingHome;
	firefly.home = home;
	firefly.progress = 0.0f;
	firefly.flightSeconds = FlightSeconds(metres, firefly.drift.flightSpeed);
}

Step fireflies::Advance(Firefly& firefly, float seconds)
{
	switch (firefly.state)
	{
	case State::Resting:
		firefly.progress = 0.0f;
		return {};
	case State::Hovering:
		firefly.progress = 1.0f;
		return {};
	case State::FlyingOut:
	case State::FlyingHome:
		break;
	}
	float progress = seconds / firefly.flightSeconds + firefly.progress;
	if (progress >= 1.0f)
	{
		progress = 1.0f;
	}
	firefly.progress = progress;
	firefly.previous = firefly.at;
	if (progress == 1.0f)
	{
		const bool out = firefly.state == State::FlyingOut;
		firefly.state = out ? State::Hovering : State::Resting;
		return {.moveTo = out ? firefly.hover : firefly.home, .along = std::nullopt};
	}
	return {.moveTo = std::nullopt, .along = Ease(progress)};
}

float fireflies::Amplitude(State state, float progress)
{
	switch (state)
	{
	case State::Resting:
		return 0.0f;
	case State::Hovering:
		return 1.0f;
	case State::FlyingHome:
		return progress > k_DriftDiesFrom ? 1.0f - ((progress - k_DriftDiesFrom) * k_DriftDying) : 1.0f;
	case State::FlyingOut:
		return progress < k_DriftGrowsUntil ? progress * k_DriftGrowth : 1.0f;
	}
	return 0.0f;
}

glm::vec3 fireflies::DriftOffset(const Drift& drift, float clock, float amplitude)
{
	const float slowTurned = clock * drift.flightSpeed;
	const float slowAround = std::fmod(slowTurned * k_SlowLoopRate + drift.phases[0], k_TwoPi);
	const float slowUp = std::fmod(slowTurned * k_SlowLoopRate + drift.phases[1], k_TwoPi);
	const float quickTurned = clock * drift.quickSpeed;
	const float quickAround = std::fmod(quickTurned * k_QuickLoopRateAround + drift.phases[3], k_TwoPi);
	const float quickUp = std::fmod(quickTurned * k_QuickLoopRateUp + drift.phases[4], k_TwoPi);
	const auto loop = [amplitude](float around, float up, float radius) {
		return glm::vec3(amplitude * (std::cos(around) * std::cos(up) * radius),
		                 amplitude * (std::sin(up) * k_LoopHeightShare * radius),
		                 amplitude * (std::sin(around) * std::cos(up) * radius));
	};
	return loop(slowAround, slowUp, k_SlowLoopRadius) + loop(quickAround, quickUp, k_QuickLoopRadius);
}

std::optional<uint8_t> fireflies::OpacityAt(float distanceSquared)
{
	if (distanceSquared > k_FarSquared)
	{
		return std::nullopt;
	}
	if (distanceSquared <= k_NearSquared)
	{
		return static_cast<uint8_t>(k_Opacity);
	}
	const float share = 1.0f - ((distanceSquared - k_NearSquared) / (k_FarSquared - k_NearSquared));
	return static_cast<uint8_t>(map_coords::FtoL(share * k_Opacity));
}

int32_t fireflies::SearchCells(float reach)
{
	const auto side = map_coords::FtoL(static_cast<float>(std::ceil(static_cast<double>((reach + reach) / k_SearchCellSize))));
	return side * side;
}

map_coords::MapCoords fireflies::NowhereFrom(const map_coords::MapCoords& from, float altitude)
{
	auto spot = from;
	spot.x = map_coords::ToFixedGUtils(map_coords::ToMetres(from.x) + k_NowhereOffset);
	spot.altitude = altitude;
	return spot;
}

void RewardTable::SetWeight(size_t kind, float weight)
{
	if (kind >= k_RewardKinds)
	{
		return;
	}
	_weights.at(kind) = weight;
	float total = 0.0f;
	for (size_t i = 0; i < k_RewardKinds; ++i)
	{
		total = total + _weights.at(i);
		_totals.at(i) = total;
	}
}

void RewardTable::ClearWeights()
{
	_weights.fill(0.0f);
}

std::optional<size_t> RewardTable::Pick(float roll) const
{
	if (roll == 0.0f)
	{
		return std::nullopt;
	}
	size_t kind = 0;
	while (_totals.at(kind) < roll)
	{
		if (++kind == k_RewardKinds)
		{
			return std::nullopt;
		}
	}
	if (kind == 0)
	{
		return std::nullopt;
	}
	return kind;
}
