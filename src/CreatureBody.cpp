/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Entities/Components/Mesh.h"
#include "Entities/Components/Creature.h"
#include "Game.h"

using namespace openblack;

using S = CreatureBody::Species;
std::map<std::string, CreatureBody::Species> MeshNameToSpecies =
{
	{"c_cow", S::Cow},
	{"a_tiger2", S::Tiger},
	{"c_leopard", S::Leopard},
	{"c_wolf", S::Wolf},
	{"c_lion", S::Lion},
	{"a_horse", S::Horse},
	{"c_tortoise", S::Tortoise},
	{"c_zebra", S::Zebra},
	{"a_bear", S::BrownBear},
	{"c_polar_bear", S::PolarBear},
	{"c_sheep", S::Sheep},
	{"c_chimp", S::Chimp},
	{"a_greek", S::Ogre},
	{"c_mandrill", S::Mandrill},
	{"c_rhino", S::Rhino},
	{"c_gorilla", S::Gorilla},
	{"c_ape", S::GiantApe}
};

using A = CreatureBody::Appearance;
std::map<std::string, CreatureBody::Appearance> MeshNameToAppearance =
{
	{"boned", A::Base},
	{"base", A::Base},
	{"good", A::Good},
	{"evil", A::Evil},
	{"strong", A::Strong},
	{"weak", A::Weak},
	{"fat", A::Fat},
	{"thin", A::Thin}
};

CreatureBody::Species GetSpeciesFromMeshName(std::string name)
{
	for (auto& [creatureTerm, species] : MeshNameToSpecies)
	{
		if (BeginsWith(name, creatureTerm))
		{
			return species;
		}
	}

	return CreatureBody::Species::Unknown;
}

CreatureBody::Appearance GetAppearanceFromMeshName(std::string name)
{
	for (auto& [creatureTerm, appearance] : MeshNameToAppearance)
	{
		if (EndsWith(name, creatureTerm))
		{
			return appearance;
		}
	}

	return CreatureBody::Appearance::Unknown;
};

bool CreatureBody::LoadCreatureMeshes()
{
	MeshLocator locator;
	auto meshDir = Game::instance()->GetFileSystem().DataPath() / "CreatureMesh";

	if (!locator.LoadLooseMeshFiles(meshDir))
	{
		return false;
	}

	auto& meshes = locator.GetMeshes();

	if (meshes.size() == 0)
	{
		return false;
	}

	// Meshes are split into base, good, evil, fat, thin, strong and weak
	for (auto& [meshName, meshId] : meshes)
	{
		auto species = GetSpeciesFromMeshName(meshName);

		if (species == Species::Unknown)
			continue;

		auto appearance = GetAppearanceFromMeshName(meshName);

		if (appearance == Appearance::Unknown)
		{
			continue;
		}

		if (appearance == Appearance::Base)
		{
			_baseMeshes.emplace(species, meshId);
		}
	}

	return true;
}

MeshId CreatureBody::GetMeshId(entities::components::Creature creature)
{
	// Given the characteristics of a creature, return a mesh
	const auto species = creature.species;
	return _baseMeshes.find(species)->second;
}
