/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <vector>

#include <entt/core/hashed_string.hpp>

#include "Creature/CreatureHair.h"

namespace openblack::ecs::components
{

/// A creature's hair, for the species that have any: each tuft's strands as they hang this frame, and how they are
/// drawn
struct CreatureHair
{
	/// The texture, Data/C_Ape_Hair.raw, and its alpha, Data/C_Ape_Haira.raw, every species' hair is drawn with
	static constexpr entt::id_type k_TextureId = entt::hashed_string("creature/hair");
	static constexpr entt::id_type k_AlphaTextureId = entt::hashed_string("creature/hair_alpha");

	struct Group
	{
		std::vector<creature_hair::Strand> strands;
		/// 0 to 255 a channel
		glm::ivec3 colour {0};
		/// How wide each strand is drawn, either side of its line
		float halfWidth {0.0f};
		/// Drawn with the hair texture rather than in its colour alone
		bool textured {false};
	};
	std::vector<Group> groups;
	/// Whether the strands have been laid out on the body yet; they start straight out from their roots
	bool started {false};
};

} // namespace openblack::ecs::components
