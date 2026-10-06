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

#include <optional>
#include <vector>

#include <entt/core/fwd.hpp>

#include "Creature/CreatureMarks.h"
#include "Creature/CreatureTattoo.h"

namespace openblack::ecs::components
{

/// The tattoos a creature wears, in its eight slots
struct CreatureTattoos
{
	creature_tattoo::Slots slots {};
	/// Goes up with each change, which paints the skins again
	uint32_t revision {0};
};

/// The wounds, burns and blood on a creature's skins
struct CreatureMarks
{
	creature_marks::Marks marks {};
	/// Goes up with each change, which paints the skins again
	uint32_t revision {0};
};

/// A creature's skins as painted: each of its base mesh's skins blended towards its evil or good mesh's, with its
/// tattoos and marks over them, 4 bits a channel
struct CreatureSkin
{
	struct Skin
	{
		uint32_t id;
		/// 256 by 256 texels, blue the lowest 4 bits and alpha the highest
		std::vector<uint16_t> texels;
	};
	std::vector<Skin> skins;

	/// What the skins were last painted with
	struct Painted
	{
		entt::id_type baseMesh;
		entt::id_type variantMesh;
		uint8_t weight;
		uint32_t tattoos;
		uint32_t marks;
		bool art;

		bool operator==(const Painted&) const = default;
	};
	std::optional<Painted> painted;
	/// Goes up each time the skins are painted, for the renderer to take them up
	uint32_t revision {0};
};

} // namespace openblack::ecs::components
