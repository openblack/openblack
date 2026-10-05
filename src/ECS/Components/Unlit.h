/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::ecs::components
{

/// The entity's mesh isn't shaded by the sun: it is drawn in the colour of the land's light where it stands, hazed with
/// distance. Black & White draws its big forests and dead trees this way, as it does its trees.
struct Unlit
{
};

} // namespace openblack::ecs::components
