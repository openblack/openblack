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

namespace openblack::ecs::components
{

/// One of the sky's clouds (see clouds), a puff of mist (components::Mist) drifting with the wind. Its Mist's alpha is
/// how far it has faded at the track's ends; its colour follows the sky.
struct Cloud
{
	/// Where it is on the wind's track
	glm::vec3 track;
	/// The two huge clouds on the horizon stay where they are
	bool pinned;
};

} // namespace openblack::ecs::components
