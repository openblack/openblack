/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Game.h"
#include "EntitySystem.h"

namespace openblack::entities::systems
{

class SystemsController
{
public:
	template<typename T, typename... Args>
	void Assign([[maybe_unused]] Args&&... args)
	{
		auto system = std::make_unique<T>(std::forward<Args>(args)...);
		system->Connect(Game::instance()->GetEntityRegistry());
		_systems.emplace_back(std::move(system));
	};
	void Update(std::chrono::duration<float, std::milli> dt);

private:
	std::vector<std::unique_ptr<EntitySystem>> _systems;
};
} // namespace openblack::entities::systems
