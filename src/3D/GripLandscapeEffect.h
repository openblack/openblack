/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec3.hpp>

/// The puff of dust thrown up where the hand grips the land, Black & White's GRIP_LANDSCAPE spot visual
/// (Data/Spells/ZSpellFiles/SF_GripLandscape).
///
/// A point a little above the ground grows from nothing to full size over a second. Eight dust sprites placed at random
/// within a sphere around it grow and spread out with it, cycling through the dust frames of S_SpriteSheet3, and fade out
/// within the first second.
namespace openblack::GripLandscapeEffect
{
/// Throws up a puff of dust at a point on the ground
void Spawn(glm::vec3 groundPosition);
/// Grows and fades the puffs, and removes those that have died
void Update(float deltaSeconds);
} // namespace openblack::GripLandscapeEffect
