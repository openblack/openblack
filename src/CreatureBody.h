/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "3D/L3DMesh.h"
#include "3D/MeshLocator.h"

#include <entt/entity/registry.hpp>

namespace openblack
{
namespace entities::components
{
struct Creature;
}

class CreatureBody
{
public:
	enum class Species
	{
		Unknown = 0,
		Cow = 1,
		Tiger = 2,
		Leopard = 3,
		Wolf = 4,
		Lion = 5,
		Horse = 6,
		Tortoise = 7,
		Zebra = 8,
		BrownBear = 9,
		PolarBear = 10,
		Sheep = 11,
		Chimp = 12,
		Ogre = 13,
		Mandrill = 14,
		Rhino = 15,
		Gorilla = 16,
		GiantApe = 17
	};

	enum class Appearance
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

	bool LoadCreatureMeshes();
	MeshId GetMeshId(entities::components::Creature creature);

private:
	std::map<Species, MeshId> _baseMeshes;
};
} // namespace openblack
