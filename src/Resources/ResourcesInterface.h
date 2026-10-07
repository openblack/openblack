/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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
using L3DFileManager = ResourceManager<L3DFileLoader>;
using Bitmap16BManager = ResourceManager<Bitmap16BLoader>;
using LandLightPaletteManager = ResourceManager<LandLightPaletteLoader>;
using TextureManager = ResourceManager<Texture2DLoader>;
using AnimationManager = ResourceManager<L3DAnimLoader>;
using LevelManager = ResourceManager<LevelLoader>;
using CreatureMindManager = ResourceManager<CreatureMindLoader>;
using CreatureRigManager = ResourceManager<CreatureRigLoader>;
using CreatureSkinArtManager = ResourceManager<CreatureSkinArtLoader>;
using SoundManager = ResourceManager<SoundLoader>;
using GlowManager = ResourceManager<LightLoader>;
using CameraPathManager = ResourceManager<CameraPathLoader>;
using ParticleFileManager = ResourceManager<ParticleFileLoader>;
using GestureTemplatesManager = ResourceManager<GestureTemplatesLoader>;
using ParticleBitmapManager = ResourceManager<ParticleBitmapLoader>;

class ResourcesInterface
{
public:
	virtual MeshManager& GetMeshes() = 0;
	/// Meshes' data, for meshes changed on the CPU
	virtual L3DFileManager& GetL3DFiles() = 0;
	virtual Bitmap16BManager& GetBitmaps() = 0;
	virtual LandLightPaletteManager& GetLandLightPalettes() = 0;
	virtual TextureManager& GetTextures() = 0;
	virtual AnimationManager& GetAnimations() = 0;
	virtual LevelManager& GetLevels() = 0;
	virtual CreatureMindManager& GetCreatureMinds() = 0;
	/// What moves each species' body, by creature::GetRigId
	virtual CreatureRigManager& GetCreatureRigs() = 0;
	/// What creatures' tattoos and marks are painted with, by creature_skin::k_ArtId
	virtual CreatureSkinArtManager& GetCreatureSkinArt() = 0;
	virtual SoundManager& GetSounds() = 0;
	virtual GlowManager& GetGlows() = 0;
	virtual CameraPathManager& GetCameraPaths() = 0;
	/// The particle effect files, by particles::ParticleFileId of their names
	virtual ParticleFileManager& GetParticleFiles() = 0;
	/// The light maps the particle effects stamp on the land, by their paths
	virtual ParticleBitmapManager& GetParticleBitmaps() = 0;
	/// The templates the hand's drawn gestures are matched against, by gesture::k_TemplatesId
	virtual GestureTemplatesManager& GetGestureTemplates() = 0;
};

} // namespace openblack::resources
