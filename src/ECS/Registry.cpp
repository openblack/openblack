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
	_registry.ctx().emplace<RegistryContext>();
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

RegistryContext& Registry::Context()
{
	return _registry.ctx().get<RegistryContext>();
}

const RegistryContext& Registry::Context() const
{
	return _registry.ctx().get<const RegistryContext>();
}

void Registry::Reset()
{
	SetDirty();
	_registry.clear();
	_registry.ctx().erase<RegistryContext>();
	_registry.ctx().emplace<RegistryContext>();
};

void Registry::SetDirty()
{
	Locator::rendereringSystem::value().SetDirty();
}
} // namespace openblack::ecs
