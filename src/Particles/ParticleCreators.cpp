/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleCreators.h"

#include <cmath>

#include <algorithm>
#include <memory>

#include <ParticleFile.h>

#include "Common/GameRandom.h"
#include "ParticleClassRegistry.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
/// The sheets these creators draw from are 256 texels across
constexpr float k_SheetTexels = 256.0f;
/// A file names no file or model with this
constexpr std::string_view k_NoName = "NULL_STRING";
/// A mist without a shape of its own is between 2.5 and 5 times wider than tall seen level
constexpr float k_RandomRatioBase = 2.5f;
constexpr float k_RandomRatioRange = 2.5f;
/// A mist's animation starts at one of its first sixteen counts
constexpr int32_t k_MistStartCounts = 16;
/// The C library's random numbers run to this
constexpr float k_CrtRandRange = 32768.0f;
/// A light map's texels are colours, three bytes each
constexpr int k_LightMapChannels = 3;

bool Named(std::string_view name)
{
	return !name.empty() && name != k_NoName;
}

std::unique_ptr<Creator> MakeMesh(const psys::ParticleObject& object, CreatorResourcesInterface* resources)
{
	auto creator = std::make_unique<MeshCreator>();
	ReadCreatorProperties(object, *creator);
	creator->kind = Creator::Kind::Mesh;
	creator->type = object.className == "ParticleAnimCreator"               ? MeshCreator::Type::Animated
	                : object.className == "ParticleMeshCreatorAnimTextured" ? MeshCreator::Type::AnimTextured
	                                                                        : MeshCreator::Type::Plain;
	// A model of the game's list when the files name one, else a model file
	if (resources != nullptr)
	{
		const auto name = object.String("MeshEnum");
		if (Named(name))
		{
			creator->mesh = resources->MeshByName(name);
		}
		if (const auto file = object.String("MeshFileName"); !creator->mesh.has_value() && Named(file))
		{
			creator->mesh = resources->MeshByFile(file);
		}
	}
	creator->faceCamera = object.Bool("FaceCamera", false);
	creator->heightStretch = object.Float("HeightStretch", 1.0f);
	// The animated models always take the creator's materials, and their atom's alpha unless told otherwise
	const bool animated = creator->type == MeshCreator::Type::Animated;
	creator->changeMaterial = animated || object.Bool("MeshChangeMaterialProps", true);
	creator->additiveMaterial = creator->changeMaterial && object.Bool("UseAdditiveAlpha", false);
	creator->writeDepthMaterial = object.Bool("MaterialUpdateZBuffer", false);
	creator->doubleSided = object.Bool("MaterialSetDoubleSided", true);
	// The plain kind never draws in its atom's alpha, whatever its file says
	creator->useGlobalAlpha = creator->type != MeshCreator::Type::Plain && object.Bool("UseGlobalAlpha", animated);
	creator->landscapeColour = object.Bool("DrawWithLandscapeColor", false);
	creator->numFrames = 1;
	if (creator->type == MeshCreator::Type::AnimTextured)
	{
		creator->textureWidth = std::max(1, object.Int("TextureWidth", 64));
		creator->textureHeight = std::max(1, object.Int("TextureHeight", 64));
		creator->slideU = object.Bool("SlideU", false);
		creator->slideV = object.Bool("SlideV", false);
		creator->randomiseInitFrame = object.Bool("RandomiseInitFrame", false);
		creator->randomiseFrameRate = object.Bool("RandomiseFrameRate", false);
		creator->frameRate = object.Float("FrameRate", 1.0f);
		creator->frameRateMax = object.Float("FrameRateMax", 10.0f);
		creator->numFrames = std::max(1, object.Int("NumFrames", 1));
		creator->playAnim = object.Bool("PlayAnim", false);
		creator->initialOffsetFraction = object.Float("InitialOffsetFrac", 0.0f);
		creator->stretchY = object.Float("StretchY", 1.0f);
	}
	else if (animated)
	{
		creator->randomiseInitFrame = object.Bool("RandomiseInitFrame", false);
	}
	creator->numFrames = creator->FramesPerAtom();
	return creator;
}

std::unique_ptr<Creator> MakeChain(const psys::ParticleObject& object)
{
	auto creator = std::make_unique<ChainCreator>();
	ReadCreatorProperties(object, *creator);
	creator->kind = Creator::Kind::Chain;
	creator->texture = TextureBaseName(object.String("TextureFileName"));
	creator->additive = object.Bool("UseAdditiveAlpha", true);
	creator->writeDepth = object.Bool("MaterialUpdateZBuffer", false);
	creator->fileOffset = object.Int("FileOffset", 0);
	creator->frameOfHead = object.Int("FrameOfHead", 0);
	creator->frameOfTail = object.Int("FrameOfTail", 0);
	creator->texturesForWholeChain = object.Int("NumTexturesForWholeChain", -1);
	creator->frameWidth = std::max(1, object.Int("FrameWidth", 32));
	creator->frameHeight = std::max(1, object.Int("FrameHeight", 64));
	return creator;
}

std::unique_ptr<Creator> MakeMist(const psys::ParticleObject& object)
{
	auto creator = std::make_unique<MistCreator>();
	ReadCreatorProperties(object, *creator);
	creator->kind = Creator::Kind::Mist;
	creator->ratio = object.Float("Ratio", 0.0f);
	creator->ratioFromMatrix = object.Bool("TakeRatioFromMatrix", false);
	creator->initialScaleMin = object.Float("InitialScaleMin", 1.0f);
	creator->numFrames = 1;
	return creator;
}

std::unique_ptr<Creator> MakeLightMap(const psys::ParticleObject& object, CreatorResourcesInterface* resources)
{
	auto creator = std::make_unique<LightMapCreator>();
	ReadCreatorProperties(object, *creator);
	creator->kind = Creator::Kind::LightMap;
	creator->pitch = std::max(1, object.Int("Pitch", 1));
	const int framesInFile = std::max(1, object.Int("NumFramesInFile", 1));
	const int framesInUse = std::clamp(object.Int("NumFramesInUse", 1), 1, framesInFile);
	creator->useJitter = object.Bool("UseRandJitter", false);
	creator->jitter = object.Float("RandJitter", 0.0f);
	creator->numFrames = framesInUse;
	creator->frameRate = object.Float("FrameRate", 1.0f);
	creator->playAnim = object.Bool("PlayAnim", false);
	creator->loopAnim = object.Bool("LoopAnim", false);
	if (const auto file = object.String("TextureFileName"); resources != nullptr && Named(file))
	{
		creator->bitmap = resources->LightMap(file, creator->pitch, k_LightMapChannels, framesInFile, framesInUse);
	}
	return creator;
}
} // namespace

glm::vec2 MeshCreator::UvOffset(int frame) const
{
	if (!slideU && !slideV)
	{
		// Cells of the sheet in rows, as many to a row as fit
		const auto columns = static_cast<uint32_t>(std::max(1, static_cast<int>(k_SheetTexels) / textureWidth));
		const auto f = static_cast<uint32_t>(frame);
		return {static_cast<float>(textureWidth) / k_SheetTexels * static_cast<float>(f % columns),
		        static_cast<float>(textureHeight) / k_SheetTexels * static_cast<float>(f / columns)};
	}
	// A slide moves one cell over the whole run of frames
	const float run = static_cast<float>(FramesPerAtom()) * k_SheetTexels;
	return {slideU ? static_cast<float>(textureWidth * frame) / run : 0.0f,
	        slideV ? static_cast<float>(textureHeight * frame) / run : 0.0f};
}

void MeshCreator::InitAtom(Effect& effect, Atom& atom) const
{
	if (type == Type::Animated)
	{
		// The animation is not played: the model is drawn as it stands. A random first frame is still drawn, so the
		// effect's random numbers stay in step with the game's.
		atom.frameRate = 0.0f;
		atom.playAnim = false;
		if (randomiseInitFrame)
		{
			atom.frame = static_cast<float>(effect.Rand(k_SlideFrames));
		}
		return;
	}
	if (type != Type::AnimTextured)
	{
		return;
	}
	float rate = frameRate;
	if (randomiseFrameRate && frameRateMax != frameRate)
	{
		rate = frameRate + effect.Random(frameRateMax - frameRate);
	}
	atom.frame = 0.0f;
	if (slideU || slideV)
	{
		// A slide runs over its thousand frames, from a fraction of the way along
		rate *= static_cast<float>(k_SlideFrames);
		if (initialOffsetFraction != 0.0f)
		{
			atom.frame = std::trunc(std::clamp(initialOffsetFraction * static_cast<float>(k_SlideFrames), 0.0f,
			                                   static_cast<float>(k_SlideFrames - 1)));
		}
	}
	if (randomiseInitFrame)
	{
		atom.frame = static_cast<float>(effect.Rand(FramesPerAtom()));
	}
	atom.frameRate = rate;
	atom.playAnim = playAnim;
	atom.stretch = stretchY;
}

std::array<glm::vec2, 4> ChainCreator::SegmentUv(int segment, int segments, int repeatsOverride) const
{
	// The ribbon is cut into repeats of the frame; the segment falls in repeat k, which holds n segments from its first,
	// b. The last repeat shows the head's frame and the first the tail's.
	const int s = std::max(1, segments);
	const int wanted = repeatsOverride != -1 ? repeatsOverride : texturesForWholeChain;
	const int repeats = std::max(1, wanted == -1 ? s : wanted);
	const int k = ((segment + 1) * repeats - 1) / s;
	const int b = k * s / repeats;
	const int n = std::max(1, ((k + 1) * s / repeats) - b);
	const int j = segment - b;
	const int frame = fileOffset + (k == repeats - 1 ? frameOfHead : (k == 0 ? frameOfTail : 0));
	const float v0 = static_cast<float>(frameHeight) * (static_cast<float>(j) / static_cast<float>(n)) / k_SheetTexels;
	const float v1 = static_cast<float>(frameHeight) * (static_cast<float>(j + 1) / static_cast<float>(n)) / k_SheetTexels;
	const float u0 = static_cast<float>(frame * frameWidth) / k_SheetTexels;
	const float u1 = u0 + (static_cast<float>(frameWidth) / k_SheetTexels);
	return {glm::vec2(u0, v0), glm::vec2(u1, v0), glm::vec2(u0, v1), glm::vec2(u1, v1)};
}

void MistCreator::InitAtom(Effect& effect, Atom& atom) const
{
	// Its animation's start comes from the C library's numbers, then its shape from this machine's own
	auto& random = effect.Services().random;
	const float start = static_cast<float>(random.CrtRand()) / k_CrtRandRange * static_cast<float>(k_MistStartCounts);
	const auto counter = static_cast<int32_t>(start) & (k_MistStartCounts - 1);
	float shape = ratio;
	if (shape == 0.0f || std::isnan(shape))
	{
		shape = random.LocalFloatRand(k_RandomRatioRange) + k_RandomRatioBase;
	}
	atom.creatorValue = {shape, static_cast<float>(counter)};
	atom.baseScale = randomiseScale ? effect.Random(initialScaleMin, initialScale) : initialScale;
	// One frame, which never moves on
	atom.frame = 0.0f;
	atom.frameRate = 1.0f;
	atom.playAnim = false;
}

void LightMapCreator::InitAtom(Effect& /*effect*/, Atom& atom) const
{
	atom.frame = 0.0f;
	atom.frameRate = frameRate;
	atom.playAnim = playAnim;
}

void openblack::particles::RegisterDrawnCreators(ParticleClassRegistry& registry, CreatorResourcesInterface* resources)
{
	const auto mesh = [resources](const psys::ParticleObject& object) { return MakeMesh(object, resources); };
	registry.AddCreator("ParticleMeshCreator", mesh);
	registry.AddCreator("ParticleMeshCreatorAnimTextured", mesh);
	registry.AddCreator("ParticleAnimCreator", mesh);
	registry.AddCreator("ParticleChainCreator", MakeChain);
	registry.AddCreator("ParticleMistCreator", MakeMist);
	registry.AddCreator("ParticleLightMapCreator",
	                    [resources](const psys::ParticleObject& object) { return MakeLightMap(object, resources); });
	registry.AddCreator("ParticleSymbolSpriteCreator", [](const psys::ParticleObject& object) {
		auto creator = std::make_unique<SymbolCreator>();
		ReadCreatorProperties(object, *creator);
		creator->kind = Creator::Kind::Symbol;
		return creator;
	});
}
