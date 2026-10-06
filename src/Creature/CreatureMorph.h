/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <span>

#include <entt/core/fwd.hpp>
#include <glm/mat4x4.hpp>

#include "3D/CreatureBody.h"

/// How a creature's body shows what it has become. Each species has a base mesh and a mesh at either end of three axes:
/// evil to good, thin to fat and weak to strong. The body is the base pulled towards one mesh on each axis, by how far
/// along it the creature is.
namespace openblack::creature_morph
{
/// How far along each axis the body is, -1 to 1: evil, thin and weak below 0, good, fat and strong above
struct Morph
{
	float evilGood {0.0f};
	float thinFat {0.0f};
	float weakStrong {0.0f};
};

/// The smallest and largest a creature can be drawn, and the largest it grows to by itself
constexpr float k_MinScale = 0.05f;
constexpr float k_MaxScale = 4.0f;
constexpr float k_MaxGrownScale = 2.0f;

/// How much a species' own strength counts towards how strong its body looks, against the creature's strength
constexpr float k_SpeciesStrengthWeight = 0.2f;
/// Neither weak nor strong, for a species whose own strength isn't known
constexpr float k_UnknownSpeciesStrength = 0.5f;

/// The body of a creature of alignment -1 (evil) to 1 (good), fatness 0 to 1 and strength 0 to 1, of a species whose
/// own strength is speciesStrength, 0 to 1
[[nodiscard]] Morph FromAttributes(float alignment, float fatness, float strength, float speciesStrength);

[[nodiscard]] float ClampScale(float scale);

/// The game draws a creature of size 1 this tall, by the height of its rest pose's bones
constexpr float k_HeightAtSizeOne = 15.0f;
/// How far the bones of a rest pose reach up and down from the mesh's origin, in the form L3DMesh::GetBoneMatrices gives
/// them
[[nodiscard]] float RestHeight(std::span<const glm::mat4> rest);
/// The scale a creature's mesh is drawn at for its size
[[nodiscard]] float DrawnScale(float size, float restHeight);

/// The mesh an axis pulls towards at a value: evil, thin or weak below 0, otherwise good, fat or strong
[[nodiscard]] creature::CreatureBody::Appearance EvilGoodMesh(float value);
[[nodiscard]] creature::CreatureBody::Appearance ThinFatMesh(float value);
[[nodiscard]] creature::CreatureBody::Appearance WeakStrongMesh(float value);

/// A body vertex, or normal before it is normalised again: the base moved towards each axis' mesh by how far along the
/// axis the body is
template <typename Vec>
[[nodiscard]] Vec Blend(const Vec& base, const Vec& evilGood, const Vec& thinFat, const Vec& weakStrong, const Morph& morph)
{
	const auto pull = [&base](const Vec& towards, float value) { return (towards - base) * (value < 0.0f ? -value : value); };
	return base + pull(evilGood, morph.evilGood) + pull(thinFat, morph.thinFat) + pull(weakStrong, morph.weakStrong);
}

/// A body is drawn again once one of its axes has moved this far from how it was last drawn
constexpr float k_RefreshThreshold = 0.03f;
/// How far a creature's fatness, as its body shows it, follows its fatness in a game turn
constexpr float k_MaxFatnessStep = 0.01f;

/// The fatness a body shows, a turn on: towards the creature's fatness, but by no more than k_MaxFatnessStep
[[nodiscard]] float EaseFatness(float shown, float fatness);

/// What changes when the body is brought up to date with what the creature has become
struct Refresh
{
	/// The body as it is now drawn
	Morph drawn;
	/// The animations and rest pose follow the evil to good and thin to fat axes
	bool animations;
	/// The vertices follow all three axes
	bool vertices;
};

/// The body is drawn anew once an axis has moved by k_RefreshThreshold. A new alignment brings all three axes up to
/// date, a new fatness the fatness and strength, a new strength the strength alone.
[[nodiscard]] Refresh RefreshDrawn(const Morph& drawn, const Morph& target);

/// The meshes a body is drawn from: its base, and the mesh each axis pulls it towards, or the base where the species
/// has no such mesh
struct Meshes
{
	entt::id_type base;
	entt::id_type evilGood;
	entt::id_type thinFat;
	entt::id_type weakStrong;
};

/// The meshes of a species' body, of those hasMesh(id) says are loaded
template <typename HasMesh>
[[nodiscard]] Meshes MeshesOf(CreatureType species, const Morph& morph, HasMesh&& hasMesh)
{
	const auto base = creature::GetIdFromType(species, creature::CreatureBody::Appearance::Base);
	const auto orBase = [&](creature::CreatureBody::Appearance appearance) {
		const auto id = creature::GetIdFromType(species, appearance);
		return hasMesh(id) ? id : base;
	};
	return {
	    .base = base,
	    .evilGood = orBase(EvilGoodMesh(morph.evilGood)),
	    .thinFat = orBase(ThinFatMesh(morph.thinFat)),
	    .weakStrong = orBase(WeakStrongMesh(morph.weakStrong)),
	};
}
} // namespace openblack::creature_morph
