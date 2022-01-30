/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
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

class ResourcesInterface
{
public:
	virtual resources::MeshManager& GetMeshes() = 0;
	virtual resources::TextureManager& GetTextures() = 0;
	virtual resources::AnimationManager& GetAnimations() = 0;
};

class Resources final: public ResourcesInterface
{
public:
	resources::MeshManager& GetMeshes() override { return _meshes; }
	resources::TextureManager& GetTextures() override { return _textures; }
	resources::AnimationManager& GetAnimations() override { return _animations; }

private:
	resources::MeshManager _meshes;
	resources::TextureManager _textures;
	resources::AnimationManager _animations;
};
} // namespace openblack::resources