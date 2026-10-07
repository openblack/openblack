/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TurnRules.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

#include "Body.h"

using namespace openblack;
using namespace openblack::physics;

namespace
{
/// How far, as a share of its speed across the ground, a moving body's reach runs ahead of it
constexpr float k_WakeAhead = 0.1f;
/// Map coordinates are metres times this, truncated; their high word is the cell
constexpr float k_FixedPerMetre = 6553.6f;
constexpr int32_t k_MapCells = 512;
/// A softer knock than this many weights sounds soft, a harder one than the other hard
constexpr float k_SoftLoad = 1.25f;
constexpr float k_HardLoad = 3.0f;
/// The collision sound type of grain
constexpr int32_t k_Grain = 11;
/// The camera hears things pass within this distance, faster than this
constexpr float k_WhooshDistanceSquared = 100.0f;
constexpr float k_WhooshSpeedSquared = 400.0f;
/// The rings are laid a little above the sea; the hit's and the bob's textures
constexpr float k_RingHeight = 0.1f;
constexpr uint8_t k_HitRingCell = 0x3F;
constexpr uint8_t k_BobRingCell = 0x30;
/// A body has sunk when its centre is under this share of its radius
constexpr float k_SinkShare = 0.5f;

int32_t CellOfMetres(float metres)
{
	// Truncated to map coordinates, then the cell is the high word (rounding down)
	const auto fixed = static_cast<int32_t>(metres * k_FixedPerMetre);
	return fixed >> 16;
}

water_rings::Ring Ring(glm::vec3 centre, float radius, uint8_t cell)
{
	return {.position = glm::vec3(centre.x, k_RingHeight, centre.z),
	        .growth = radius + radius,
	        .rate = 1.0f / radius,
	        .cell = cell,
	        .argb = 0xFFFFFFFFu};
}
} // namespace

std::optional<float> turn::Impact(glm::vec3 forceSum)
{
	const float squared = glm::dot(forceSum, forceSum);
	if (squared <= k_LeastForceSquared)
	{
		return std::nullopt;
	}
	return std::sqrt(squared) * k_ImpactScale;
}

float turn::GLoad(float impact, float mass)
{
	return impact / (mass * k_Gravity);
}

bool turn::WantsCollisionSound(bool resting, bool hitByBody, float impact, float mass)
{
	return !resting && (hitByBody || impact > mass * (0.5f * k_Gravity));
}

turn::SoundLevel turn::CollisionLevel(float impact, float infoWeight, bool grain)
{
	const float load = impact / (infoWeight * k_Gravity);
	auto level = SoundLevel::Medium;
	if (load < k_SoftLoad)
	{
		level = SoundLevel::Soft;
	}
	else if (load > k_HardLoad)
	{
		level = SoundLevel::Hard;
	}
	if (grain && level == SoundLevel::Hard)
	{
		level = SoundLevel::Medium;
	}
	return level;
}

std::array<int32_t, 5> turn::CollisionKeys(SoundLevel level, int32_t hitterType, int32_t hitType)
{
	const auto code = [](const auto& table, int32_t type) {
		return type >= 0 && static_cast<size_t>(type) < table.size() ? table[static_cast<size_t>(type)] : 0;
	};
	return {static_cast<int32_t>(level), 0, code(k_HitterCodes, hitterType), code(k_HitCodes, hitType), k_CollisionAction};
}

bool turn::SoundPairs::MaySound(uint32_t a, uint32_t b) const
{
	if (_pairs.size() >= k_MostPairs)
	{
		return false;
	}
	return std::ranges::none_of(
	    _pairs, [a, b](const Pair& pair) { return (pair.a == a && pair.b == b) || (pair.a == b && pair.b == a); });
}

void turn::SoundPairs::Add(uint32_t a, uint32_t b)
{
	if (_pairs.size() < k_MostPairs)
	{
		_pairs.push_back({.a = a, .b = b, .turns = k_QuietTurns});
	}
}

void turn::SoundPairs::EndTurn()
{
	for (auto& pair : _pairs)
	{
		--pair.turns;
	}
	std::erase_if(_pairs, [](const Pair& pair) { return pair.turns <= 0; });
}

turn::CellRange turn::SquareCells(glm::vec3 centre, float half)
{
	const auto clamp = [](int32_t cell) { return std::clamp(cell, 0, k_MapCells - 1); };
	const glm::ivec2 first(clamp(CellOfMetres(centre.x - half)), clamp(CellOfMetres(centre.z - half)));
	const glm::ivec2 second(clamp(CellOfMetres(centre.x + half)), clamp(CellOfMetres(centre.z + half)));
	return {.low = glm::min(first, second), .high = glm::max(first, second)};
}

turn::CellRange turn::WakeCells(glm::vec3 centre, glm::vec3 velocity, float radius)
{
	return SquareCells(centre, glm::length(glm::vec2(velocity.x, velocity.z)) * k_WakeAhead + radius);
}

float turn::DustPuffSize(const DustPuff& puff)
{
	float size = puff.size * (k_PuffLife - puff.age) / k_PuffLife;
	if (puff.age < k_PuffLife * k_PuffGrowth)
	{
		size *= puff.age / (k_PuffLife * k_PuffGrowth);
	}
	return size;
}

int32_t turn::DustPuffFrame(const DustPuff& puff)
{
	const auto turns = static_cast<int32_t>(puff.age * 2.0f);
	return 16 + ((puff.variant + turns) & 15);
}

bool turn::AdvanceDustPuff(DustPuff& puff, float seconds)
{
	puff.age += seconds;
	if (puff.age >= k_PuffLife)
	{
		return false;
	}
	puff.position += puff.velocity * seconds;
	return true;
}

float turn::LandingPuffSize(float radius)
{
	return std::min(radius + radius, k_LargestPuff);
}

uint32_t turn::BlendColour(uint32_t argb, uint32_t towards, int32_t amount)
{
	const auto channel = [argb, towards, amount](uint32_t shift) {
		const auto from = static_cast<int32_t>((argb >> shift) & 0xFFu);
		const auto to = static_cast<int32_t>((towards >> shift) & 0xFFu);
		// Each channel moves towards the other by the amount in 256ths, rounding down
		const int32_t moved = from + static_cast<int32_t>(std::floor(static_cast<float>((to - from) * amount) / 256.0f));
		return static_cast<uint32_t>(moved & 0xFF) << shift;
	};
	return (argb & 0xFF000000u) | channel(16) | channel(8) | channel(0);
}

water_rings::Ring turn::ImpactRing(glm::vec3 centre, float radius)
{
	return Ring(centre, radius, k_HitRingCell);
}

water_rings::Ring turn::BobRing(glm::vec3 centre, float radius)
{
	return Ring(centre, radius, k_BobRingCell);
}

bool turn::PassesCamera(glm::vec3 before, glm::vec3 after, glm::vec3 camera, glm::vec3 velocity)
{
	const auto from = before - camera;
	const auto to = after - camera;
	return glm::dot(from, from) > k_WhooshDistanceSquared && glm::dot(to, to) < k_WhooshDistanceSquared &&
	       glm::dot(velocity, velocity) > k_WhooshSpeedSquared;
}

bool turn::NearSeaLevel(bool resting, float centreHeight, float radius)
{
	return !resting && centreHeight < radius * k_SinkShare;
}

bool turn::Bobbed(float upwardSpeedBefore, float upwardSpeedAfter)
{
	return upwardSpeedBefore * upwardSpeedAfter < 0.0f;
}

float turn::PushForce(float weight)
{
	return weight * k_Gravity;
}
