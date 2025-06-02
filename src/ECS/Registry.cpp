/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Registry.h"

#include "Locator.h"
#include "Systems/RenderingSystemInterface.h"

namespace openblack::ecs
{

Registry::Registry()
{
	_registry.ctx().emplace<MainGameContext>();
}

void Registry::Release(entt::entity entity)
{
	ENTT_ASSERT(_registry.orphan(entity), "Non-orphan entity");
	_registry.storage<entt::entity>().erase(entity);
}

void Registry::Destroy(entt::entity entity)
{
	_registry.destroy(entity);
}

MainMapContext& Registry::InitMapContext()
{
	return _registry.ctx().emplace<MainMapContext>();
}

MainMapContext& Registry::MapContext()
{
	return _registry.ctx().get<MainMapContext>();
}

const MainMapContext& Registry::MapContext() const
{
	return _registry.ctx().get<const MainMapContext>();
}

MainGameContext& Registry::GameContext()
{
	return _registry.ctx().get<MainGameContext>();
}

const MainGameContext& Registry::GameContext() const
{
	return _registry.ctx().get<const MainGameContext>();
}

void Registry::ClearMap()
{
	SetDirty();
	_registry.clear(); // TODO: clean only map entites
	_registry.ctx().erase<MainMapContext>();
	_registry.ctx().emplace<MainMapContext>();
};

void Registry::SetDirty()
{
	if (Locator::rendereringSystem::has_value())
	{
		Locator::rendereringSystem::value().SetDirty();
	}
}
} // namespace openblack::ecs
