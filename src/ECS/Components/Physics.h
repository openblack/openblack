/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

/// The object is in the physics: thrown, dropped, knocked or pushed, it flies, slides or floats there until it comes to
/// rest, and is out of the map's cells meanwhile
struct InPhysics
{
};

/// A script holds the object where it is: it is never thrown, dropped or knocked into flight, and the body of a model
/// never moves
struct Immovable
{
};

/// How much of a building stands built, 0 to 1: flying things hit a building only once more than a tenth of it is built.
/// openblack doesn't build buildings up yet, so a building without one counts as built.
struct BuildProgress
{
	float built {1.0f};
};

/// Where a moving body is drawn this frame, between where it was at the start of the last game turn and where it is now,
/// so that its motion looks smooth whatever the frame rate. The game itself sees the object's Transform, where the body
/// is at the end of the turn.
struct PhysicsDrawPose
{
	/// The model's axes, scaled by the object's scale
	glm::mat3 axes {1.0f};
	glm::vec3 origin {0.0f};
};

} // namespace openblack::ecs::components
