/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>

namespace openblack::particles
{
struct CarriedObject;
}

namespace openblack::ecs::components
{

/// Something a tornado has picked up: it is off the ground, its own doings stop, and it follows the particle carrying
/// it until the particle has gone and it is let go where it was flung
struct CarriedByTornado
{
	std::shared_ptr<particles::CarriedObject> carried;
};

/// A creature a tornado has caught: too big to be picked up, it stands helpless and is let off the leash, once
struct CaughtByTornado
{
};

} // namespace openblack::ecs::components
