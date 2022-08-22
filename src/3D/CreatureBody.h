/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <map>

#include <entt/entity/registry.hpp>

#include "Enums.h"

namespace openblack
{
namespace ecs::components
{
struct Creature;
}

namespace creature
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

entt::id_type GetIdFromMeshName(const std::string& fileName);
entt::id_type GetIdFromType(CreatureType species);
CreatureType GetSpeciesFromMeshName(const std::string& name);
CreatureBody::Appearance GetAppearanceFromMeshName(const std::string& name);
} // namespace creature
} // namespace openblack
