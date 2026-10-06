/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureFootprints.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

#include "3D/CreatureBody.h"

namespace openblack::creature_footprints
{
namespace
{
/// A print's corners about its middle, for a size of 1, before it is turned. The picture points along +x.
constexpr std::array<glm::vec2, 4> k_Corners = {glm::vec2(-k_HalfSide, k_HalfSide), glm::vec2(k_HalfSide, k_HalfSide),
                                                glm::vec2(k_HalfSide, -k_HalfSide), glm::vec2(-k_HalfSide, -k_HalfSide)};
/// Where the corners fall in the print's cell, for the right foot and for the left, flipped top to bottom
constexpr std::array<std::array<glm::vec2, 4>, 2> k_CellUvs = {{
    {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)},
    {glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
}};
} // namespace

SpeciesPrint PrintOf(CreatureType species, bool aprilFools)
{
	const auto row = std::min(creature::InfoRow(species), k_SpeciesPrints.size() - 1);
	auto print = k_SpeciesPrints.at(row);
	if (aprilFools)
	{
		print.cell = k_SmileyCell;
	}
	return print;
}

Foot FootOf(const glm::mat4& world)
{
	// The game's matrices are the transposes of these. Its yaw, of a turn about y then x then z, comes from what the
	// bone's z axis has become.
	const auto z = glm::vec3(world[2]);
	const auto length = glm::length(z);
	const auto axis = length > 0.0f ? z / length : glm::vec3(0.0f, 0.0f, 1.0f);
	return {.position = glm::vec3(world[3]), .yaw = std::atan2(-axis.x, axis.z)};
}

float Side(float creatureSize, const SpeciesPrint& print)
{
	return 2.0f * k_HalfSide * creatureSize * k_SizePerCreatureSize * print.scale;
}

Footprint MakeFootprint(const glm::vec3& foot, float angle, float side, uint8_t cell, bool left, const AltitudeAt& altitudeAt)
{
	const auto size = side / (2.0f * k_HalfSide);
	const auto cosine = std::cos(angle);
	const auto sine = std::sin(angle);
	const auto origin = glm::vec2(static_cast<float>(cell) / static_cast<float>(k_CellsPerRow), 0.0f);
	const auto& cellUvs = k_CellUvs.at(left ? 1 : 0);
	Footprint print;
	for (size_t i = 0; i < print.corners.size(); ++i)
	{
		const auto offset = k_Corners.at(i) * size;
		const auto x = foot.x + (cosine * offset.x) - (sine * offset.y);
		const auto z = foot.z + (sine * offset.x) + (cosine * offset.y);
		print.corners.at(i) = glm::vec3(x, altitudeAt(x, z) + k_Lift, z);
		print.uvs.at(i) = origin + (cellUvs.at(i) / static_cast<float>(k_CellsPerRow));
	}
	return print;
}

bool Add(Trail& trail, const Footprint& print)
{
	if (trail.prints.size() >= k_Capacity)
	{
		return false;
	}
	trail.prints.push_back(print);
	return true;
}

void Fade(Trail& trail, float elapsedMs)
{
	trail.sinceFadeMs += elapsedMs;
	if (trail.sinceFadeMs < k_FadeIntervalMs)
	{
		return;
	}
	const auto fade = trail.sinceFadeMs * k_FadePerMs;
	trail.sinceFadeMs = 0.0f;
	for (auto& print : trail.prints)
	{
		// Kept in whole steps, the fraction dropped
		print.alpha = static_cast<uint8_t>(std::max(static_cast<float>(print.alpha) - fade, 0.0f));
	}
	std::erase_if(trail.prints, [](const Footprint& print) { return print.alpha == 0; });
}
} // namespace openblack::creature_footprints
