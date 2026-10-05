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

#include <entt/core/hashed_string.hpp>

namespace openblack::ecs::components
{

/// A puff of mist: the mist mesh turned to face the camera, in frames of the smoke texture. The land's scripts lay banks
/// of them. Its Transform holds where it is.
struct Mist
{
	static constexpr entt::id_type k_MeshId = entt::hashed_string("landscape/mist");
	static constexpr entt::id_type k_TextureId = entt::hashed_string("raw/smoke");
	static constexpr entt::id_type k_AlphaTextureId = entt::hashed_string("raw/smokea");

	/// How many times the mesh's size it is
	float size;
	/// 0xAARRGGBB
	uint32_t colour;
	/// A mist that shrinks edge on is round seen from straight above or below and edgeShrink times wider than tall
	/// seen level. It is lit from straight above rather than by the land. The others keep their size and take the
	/// land's light where they stand.
	bool shrinksEdgeOn;
	float edgeShrink;
	/// Its animation (see mists)
	int counter;
	float counterRemainder;
};

} // namespace openblack::ecs::components
