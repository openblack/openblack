/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <vector>

#include <entt/fwd.hpp>

namespace openblack::audio
{

struct SoundGroup
{
	std::vector<entt::id_type> sounds;
};

} // namespace openblack::audio
