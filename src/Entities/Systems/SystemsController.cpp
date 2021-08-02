/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "SystemsController.h"

using namespace openblack::entities::systems;

void SystemsController::Update(std::chrono::duration<float, std::milli> dt)
{
	for (auto& system : _systems)
	{
		system->Update(dt);
	}
}
