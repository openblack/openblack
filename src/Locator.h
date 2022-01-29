/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Resources/Loaders.h"
#include "Resources/ResourceManager.h"

#include <entt/locator/locator.hpp>

namespace openblack
{

namespace resources
{
using MeshManager = ResourceManager<resources::L3DLoader>;
using TextureManager = ResourceManager<resources::Texture2DLoader>;
using AnimationManager = ResourceManager<resources::L3DAnimLoader>;

class ResourcesInterface
{
public:
	ResourcesInterface() {}
	virtual ~ResourcesInterface() {}
	virtual resources::MeshManager& GetMeshes() = 0;
	virtual resources::TextureManager& GetTextures() = 0;
	virtual resources::AnimationManager& GetAnimations() = 0;
};

class Resources final : public ResourcesInterface
{
public:
	Resources() {}
	resources::MeshManager& GetMeshes() override { return _meshes; }
	resources::TextureManager& GetTextures() override { return _textures; }
	resources::AnimationManager& GetAnimations() override { return _animations; }

private:
	resources::MeshManager _meshes;
	resources::TextureManager _textures;
	resources::AnimationManager _animations;
};
} // namespace resources

struct Locator
{
	using resources = entt::service_locator<resources::ResourcesInterface>;
};
} // namespace openblack