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

#include <glm/vec2.hpp>

namespace openblack::ecs::components
{

/// A light a village keeps at night, on a street lantern or a country lantern: it lights the land around it (see
/// village_lights), flickering a little. Its Transform holds where it stands.
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
	/// The size of the glow about its flame
	float glowSize;
};

} // namespace openblack::ecs::components
