/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "ECS/Systems/PathfindingSystemInterface.h"

namespace openblack::ecs::systems
{
class PathfindingSystem final: public PathfindingSystemInterface
{
public:
	void Update() override;
};
} // namespace openblack::ecs::systems
