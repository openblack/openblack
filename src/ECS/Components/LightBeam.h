/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "3D/Light.h"

namespace openblack::ecs::components
{
/// The beam of a spot light, drawn with its glow
struct LightBeam
{
	LightCone cone;
};
} // namespace openblack::ecs::components
