/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ResourcesInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::resources
{
class Resources final: public ResourcesInterface
{
public:
	MeshManager& GetMeshes() override { return _meshes; }
	L3DFileManager& GetL3DFiles() override { return _l3dFiles; }
	Bitmap16BManager& GetBitmaps() override { return _bitmaps; }
	LandLightPaletteManager& GetLandLightPalettes() override { return _landLightPalettes; }
	TextureManager& GetTextures() override { return _textures; }
	AnimationManager& GetAnimations() override { return _animations; }
	LevelManager& GetLevels() override { return _levels; }
	CreatureMindManager& GetCreatureMinds() override { return _creatureMinds; }
	SoundManager& GetSounds() override { return _sounds; }
	GlowManager& GetGlows() override { return _glows; }
	CameraPathManager& GetCameraPaths() override { return _cameraPaths; }

private:
	MeshManager _meshes;
	L3DFileManager _l3dFiles;
	Bitmap16BManager _bitmaps;
	LandLightPaletteManager _landLightPalettes;
	TextureManager _textures;
	AnimationManager _animations;
	LevelManager _levels;
	CreatureMindManager _creatureMinds;
	SoundManager _sounds;
	GlowManager _glows;
	CameraPathManager _cameraPaths;
};
} // namespace openblack::resources
