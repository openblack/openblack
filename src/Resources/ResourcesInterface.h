/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

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
using SoundManager = ResourceManager<SoundLoader>;
using GlowManager = ResourceManager<LightLoader>;
using CameraPathManager = ResourceManager<CameraPathLoader>;

class ResourcesInterface
{
public:
	virtual MeshManager& GetMeshes() = 0;
	virtual TextureManager& GetTextures() = 0;
	virtual AnimationManager& GetAnimations() = 0;
	virtual LevelManager& GetLevels() = 0;
	virtual CreatureMindManager& GetCreatureMinds() = 0;
	virtual SoundManager& GetSounds() = 0;
	virtual GlowManager& GetGlows() = 0;
	virtual CameraPathManager& GetCameraPaths() = 0;
};

} // namespace openblack::resources
