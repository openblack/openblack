/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

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

/// Until bodies are blended when drawn, the one whole mesh nearest the body: the mesh of the axis it is furthest along,
/// or the base when it is less than halfway along every axis
[[nodiscard]] creature::CreatureBody::Appearance NearestMesh(const Morph& morph);
} // namespace openblack::creature_morph
