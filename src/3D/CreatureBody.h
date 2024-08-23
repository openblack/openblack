/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <string>

#include <entt/fwd.hpp>

#include "Enums.h"

namespace openblack::creature
{
class CreatureBody
{
public:
	enum class Appearance : uint32_t
	{
		Unknown,
		Base,
		Good,
		Evil,
		Strong,
		Weak,
		Fat,
		Thin
	};
};

/**
 * Returns an ECS ID representing the mesh resource for the given creature mesh file name.
 * For example, "A_Tiger2_Evil" returns the "evil" variant mesh resource ID and
 * "A_Tiger2_Base" returns the "base" variant.
 * @param fileName The creature mesh file name
 * @return A unique ID pointing to the mesh
 */
entt::id_type GetIdFromMeshName(const std::string& fileName);
entt::id_type GetIdFromType(CreatureType species, CreatureBody::Appearance appearance);
} // namespace openblack::creature
