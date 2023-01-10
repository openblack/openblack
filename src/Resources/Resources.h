/*****************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Loaders.h"
#include "ResourceManager.h"

namespace openblack::resources
{
using MeshManager = ResourceManager<L3DLoader>;
using TextureManager = ResourceManager<Texture2DLoader>;
using AnimationManager = ResourceManager<L3DAnimLoader>;
using LevelManager = ResourceManager<LevelLoader>;
using CreatureMindManager = ResourceManager<CreatureMindLoader>;

class ResourcesInterface
{
public:
	virtual MeshManager& GetMeshes() = 0;
	virtual TextureManager& GetTextures() = 0;
	virtual AnimationManager& GetAnimations() = 0;
	virtual LevelManager& GetLevels() = 0;
	virtual CreatureMindManager& GetCreatureMinds() = 0;
};

class Resources final: public ResourcesInterface
{
public:
	MeshManager& GetMeshes() override { return _meshes; }
	TextureManager& GetTextures() override { return _textures; }
	AnimationManager& GetAnimations() override { return _animations; }
	LevelManager& GetLevels() override { return _levels; }
	CreatureMindManager& GetCreatureMinds() override { return _creatureMinds; }

private:
	MeshManager _meshes;
	TextureManager _textures;
	AnimationManager _animations;
	LevelManager _levels;
	CreatureMindManager _creatureMinds;
};
} // namespace openblack::resources
