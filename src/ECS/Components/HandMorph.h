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
#include <optional>
#include <vector>

#include <entt/core/hashed_string.hpp>

#include "3D/HandMorph.h"

namespace openblack::ecs::components
{

/// How the god hand shows its player's alignment (see hand_morph): the alignment its shape is drawn at and its skins as
/// blended
struct HandMorph
{
	/// The meshes the hand is pulled towards, evil then good, of the same shape as its base mesh
	static constexpr std::array<entt::id_type, 2> k_LookMeshIds = {entt::hashed_string("hand/evil"),
	                                                               entt::hashed_string("hand/good")};
	/// The files of the base, evil and good meshes, which hold their skins' texels
	static constexpr std::array<entt::id_type, 3> k_SkinFileIds = {
	    entt::hashed_string("hand/file/base"), entt::hashed_string("hand/file/evil"), entt::hashed_string("hand/file/good")};

	struct Skin
	{
		/// The base mesh's skin it takes the place of
		uint32_t id;
		/// 256 by 256 texels, blue the lowest 4 bits and alpha the highest
		std::vector<uint16_t> texels;
	};

	hand_morph::State state;
	/// The point the influence was last tested at (see hand_morph::Pick), and whether it was in the player's: what the
	/// next frame goes by. Not known before the first test.
	map_coords::MapCoords point {};
	std::optional<bool> pointInInfluence;
	/// Empty until the skin is first blended
	std::vector<Skin> skins;
	/// Goes up each time the skins are blended, for the renderer to take them up
	uint32_t revision {0};
};

} // namespace openblack::ecs::components
