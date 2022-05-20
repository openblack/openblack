/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/locator/locator.hpp>

#include "Common/RandomNumberManager.h"
#include "Resources/Resources.h"

namespace openblack
{

struct Locator
{
	using resources = entt::service_locator<resources::ResourcesInterface>;
	using rng = entt::service_locator<RandomNumberManagerInterface>;
};
} // namespace openblack
