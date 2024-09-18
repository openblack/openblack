/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ResourcesInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#pragma message("Locator interface implementations should only be included in Locator.cpp, use interface instead.")
#endif

namespace openblack::resources
{
class Resources final: public ResourcesInterface
{
public:
	MeshManager& GetMeshes() override { return _meshes; }
	TextureManager& GetTextures() override { return _textures; }
	AnimationManager& GetAnimations() override { return _animations; }
	LevelManager& GetLevels() override { return _levels; }
	CreatureMindManager& GetCreatureMinds() override { return _creatureMinds; }
	SoundManager& GetSounds() override { return _sounds; }
	GlowManager& GetGlows() override { return _glows; }
	CameraPathManager& GetCameraPaths() override { return _cameraPaths; }

private:
	MeshManager _meshes;
	TextureManager _textures;
	AnimationManager _animations;
	LevelManager _levels;
	CreatureMindManager _creatureMinds;
	SoundManager _sounds;
	GlowManager _glows;
	CameraPathManager _cameraPaths;
};
} // namespace openblack::resources
