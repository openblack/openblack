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

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>

namespace openblack::ecs::components
{

/// A light a village keeps at night, on a street lantern or a country lantern: it lights the land around it (see
/// village_lights), flickering a little, and burns with two flames in a glow (VillageLightSprite). Its Transform holds
/// where it stands.
struct VillageLight
{
	enum class Kind : uint8_t
	{
		Town,
		Country,
	};

	Kind kind;
	/// Game time since it last flickered, in milliseconds
	float flickerTimer;
	/// How far it has flickered from where it stands along x and z
	glm::vec2 flicker;
	/// The size of the glow about its flames
	float glowSize;
};

/// One of a village light's flames or its glow, a Sprite above the light
struct VillageLightSprite
{
	entt::entity light;
	/// The flames come first, then the glow
	uint8_t index;
};

/// The flames' loop, which every village light keeps in time with
struct VillageLightFlames
{
	/// Game time into the loop, in whole milliseconds
	int32_t clock;
	/// Where each sprite starts in the loop
	std::array<int32_t, 3> starts;
};

} // namespace openblack::ecs::components
