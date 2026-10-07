/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <glm/vec2.hpp>

#include "ParticleEffect.h"
#include "SurfaceOfRevolution.h"

namespace openblack::particles
{
class ParticleClassRegistry;

/// How an atom is drawn as a surface of revolution (see surface::Build): the swirl of light under a miracle dispenser,
/// the teleport's pool. The rule that makes the atom gives it this creator and a surface of its own. The atom's colour
/// and alpha tint the surface, its frame places, turns and sizes it, and its texture slides round and out.
struct SurfaceCreator final: Creator
{
	surface::Shape shape;
	/// It sways when either is set, unless it lies on the land
	float maxTwist {0.0f};
	float maxSlide {0.0f};
	/// Laid over the land under it once, as it is made
	bool drapeOverLand {false};
	/// Laid over the land again every time it is drawn
	bool clampToLand {false};
	/// Lit by the light, each point by its normal
	bool lit {true};
	bool doubleSided {false};
	/// The texture's offset wraps by this much each way: the sheet's width and height over 256
	glm::vec2 wrap {1.0f};
};

/// An atom's own surface: as built and as it is now (swayed, or laid over the land), and how far its texture has slid
/// at the last two steps and now
struct SurfaceInstance
{
	surface::Mesh still;
	surface::Mesh mesh;
	glm::vec2 uvPrevious {0.0f};
	glm::vec2 uvLast {0.0f};
	glm::vec2 uvCurrent {0.0f};
};

/// How far a surface's texture has slid when drawn a fraction of the way between its last two steps: back by the wrap
/// while beyond it
[[nodiscard]] glm::vec2 SurfaceUvOffset(const SurfaceInstance& instance, const SurfaceCreator& creator, float fraction);

/// The rules that draw an atom as a surface
void RegisterSurfaceRules(ParticleClassRegistry& registry);

} // namespace openblack::particles
