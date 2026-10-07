/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FlockMiracleRules.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack::magic;

int flock::NumberToCreate(uint32_t number, float tribalPower)
{
	// Rounded half to even, as the game's rounding does
	return static_cast<int>(std::nearbyint(static_cast<double>(number) * static_cast<double>(tribalPower)));
}

bool flock::IsEvil(float alignment, float alignmentSwitch)
{
	return alignment < alignmentSwitch;
}

float flock::EmitAfterTurn(float emitted, float turnSeconds)
{
	return emitted + (k_EmitPerSecond * turnSeconds);
}

float flock::SpawnFraction(int created, float emittedBefore, float emittedAfter)
{
	const float span = emittedAfter - emittedBefore;
	return span > 0.0f ? (static_cast<float>(created) - emittedBefore) / span : 1.0f;
}

glm::vec2 flock::SpawnPoint(glm::vec2 from, glm::vec2 to, float fraction)
{
	return from + ((to - from) * fraction);
}

glm::vec2 flock::Jitter(glm::vec2 point, float randomX, float randomZ)
{
	return {point.x + randomX - k_SpawnJitter, point.y + randomZ - k_SpawnJitter};
}

glm::vec2 flock::Direction(bool humanCaster, glm::vec3 cameraForward, glm::vec3 castPosition, glm::vec3 handPosition)
{
	const glm::vec2 way = humanCaster ? glm::vec2(cameraForward.x, cameraForward.z)
	                                  : glm::vec2(castPosition.x - handPosition.x, castPosition.z - handPosition.z);
	if (glm::dot(way, way) < k_TinyDirection)
	{
		return {1.0f, 0.0f};
	}
	return way;
}

float flock::Side(bool humanCaster, glm::vec2 direction, glm::vec2 spawn, glm::vec2 castPoint, int created)
{
	const float parity = created % 2 == 0 ? 1.0f : -1.0f;
	if (!humanCaster)
	{
		return parity;
	}
	auto released = spawn - castPoint;
	released = glm::dot(released, released) < k_TinyDirection ? glm::vec2(1.0f, 0.0f) : glm::normalize(released);
	const auto view = glm::dot(direction, direction) < k_TinyDirection ? glm::vec2(1.0f, 0.0f) : glm::normalize(direction);
	const float cross = (released.y * view.x) - (released.x * view.y);
	if (cross > k_SideThreshold)
	{
		return 1.0f;
	}
	if (cross < -k_SideThreshold)
	{
		return -1.0f;
	}
	return parity;
}

float flock::FanAngle(int created, float side, int numberToCreate)
{
	if (numberToCreate <= 0)
	{
		return 0.0f;
	}
	return k_AngleVariation * static_cast<float>(created) * side / static_cast<float>(numberToCreate);
}

glm::vec2 flock::Rotate(glm::vec2 direction, float angle)
{
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	return {(direction.x * c) - (direction.y * s), (direction.x * s) + (direction.y * c)};
}

glm::vec2 flock::StepByCells(glm::vec2 from, glm::vec2 travel)
{
	const auto step = [](float position, float distance) {
		const float cell = std::floor(position / k_CellSize);
		const float offset = position - (cell * k_CellSize);
		// The cell number moves, cut to a whole one towards zero and kept to the map's sixteen bits of cells
		const auto moved = static_cast<uint16_t>(static_cast<int32_t>(((cell * k_CellSize) + distance) / k_CellSize));
		return (static_cast<float>(moved) * k_CellSize) + offset;
	};
	return {step(from.x, travel.x), step(from.y, travel.y)};
}

std::optional<glm::vec2> flock::Destination(glm::vec2 spawn, glm::vec2 direction, float distance,
                                            const std::function<bool(glm::vec2)>& onMap)
{
	const float length = glm::length(direction);
	if (!(length > 0.0f))
	{
		return std::nullopt;
	}
	const auto unit = direction / length;
	glm::vec2 target;
	do
	{
		// Tried at the distance, which is then halved; it stops once on the map, or once the halved distance is short
		target = StepByCells(spawn, unit * distance);
		distance *= 0.5f;
	} while (!onMap(target) && distance > k_MinTravel);
	return onMap(target) ? std::optional(target) : std::nullopt;
}

float flock::SpawnScale(bool wolf, float random)
{
	return (wolf ? k_WolfScaleMin : k_BirdScaleMin) + random;
}

flock::Corridor flock::MakeCorridor(glm::vec2 start, glm::vec2 destination, float halfWidth)
{
	const auto way = destination - start;
	glm::vec2 normal(way.y, -way.x);
	glm::vec2 along = way;
	if (glm::dot(normal, normal) < k_TinyDirection)
	{
		normal = {1.0f, 0.0f};
		along = {0.0f, 1.0f};
	}
	normal = glm::normalize(normal);
	along = glm::normalize(along);
	return {.normal = normal, .offset = -glm::dot(normal, start), .halfWidth = halfWidth, .along = along};
}

bool flock::IsOnCorridor(const Corridor& corridor, glm::vec2 point, glm::vec2 wolf)
{
	if (std::abs(glm::dot(corridor.normal, point) + corridor.offset) > corridor.halfWidth)
	{
		return false;
	}
	// Behind is measured from the corner of the land's cell the wolf is in
	const auto corner = glm::floor(wolf / k_CellSize) * k_CellSize;
	return glm::dot(point - corner, corridor.along) >= -corridor.halfWidth;
}

bool flock::CloserThanRemembered(glm::ivec2 wolf, glm::ivec2 prey, glm::ivec2 remembered)
{
	const auto chebyshev = [](glm::ivec2 a, glm::ivec2 b) {
		const auto across = [](int32_t from, int32_t to) {
			const auto difference = static_cast<uint32_t>(from) - static_cast<uint32_t>(to);
			return static_cast<int32_t>(difference) < 0 ? 0U - difference : difference;
		};
		return std::max(across(a.x, b.x), across(a.y, b.y));
	};
	// Twice the way to the prey in map units, against the way from the number of the wolf's cell to the remembered point
	// in map units: the game mixes the two, so only a point remembered near the map's corner turns prey down
	const glm::ivec2 cell(static_cast<int32_t>(static_cast<uint32_t>(wolf.x) >> 16U),
	                      static_cast<int32_t>(static_cast<uint32_t>(wolf.y) >> 16U));
	return 2U * chebyshev(wolf, prey) < chebyshev(cell, remembered);
}

bool flock::WolfArrived(glm::vec2 wolf, glm::vec2 destination)
{
	return glm::distance(wolf, destination) < k_WolfArrival;
}

bool flock::IsPrey(const PreyFacts& prey)
{
	if (prey.hunterFading || prey.uneatable || prey.skeleton || prey.helpless || !prey.hasMeat || !prey.onCorridor)
	{
		return false;
	}
	if (prey.heightAboveLand > k_PreyMaxHeight)
	{
		return false;
	}
	return prey.isVillager || prey.isOtherAnimal;
}

int flock::TurnsToPlay(uint32_t playTime, uint32_t turnMilliseconds)
{
	int turns = 0;
	while (turnMilliseconds != 0 && static_cast<uint32_t>(turns) * turnMilliseconds < playTime)
	{
		++turns;
	}
	return turns;
}

glm::vec3 flock::EatingPosition(glm::vec3 prey, glm::vec3 wolf, float scale)
{
	auto way = prey - wolf;
	const float length = glm::length(way);
	if (length != 0.0f)
	{
		way *= scale / length;
	}
	return prey - way;
}
