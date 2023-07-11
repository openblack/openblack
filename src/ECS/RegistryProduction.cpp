/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS
#include "RegistryProduction.h"

#include "Locator.h"
#include "Systems/RenderingSystemInterface.h"

namespace openblack::ecs
{

RegistryProduction::RegistryProduction()
{
	_registry.ctx().emplace<RegistryContext>();
}

void RegistryProduction::Release(entt::entity entity)
{
	ENTT_ASSERT(_registry.orphan(entity), "Non-orphan entity");
	_registry.storage<entt::entity>().erase(entity);
}

void RegistryProduction::Destroy(entt::entity entity)
{
	_registry.destroy(entity);
}

RegistryContext& RegistryProduction::Context()
{
	return _registry.ctx().get<RegistryContext>();
}

const RegistryContext& RegistryProduction::Context() const
{
	return _registry.ctx().get<const RegistryContext>();
}

void RegistryProduction::Reset()
{
	SetDirty();
	_registry.clear();
	_registry.ctx().erase<RegistryContext>();
	_registry.ctx().emplace<RegistryContext>();
};

void RegistryProduction::SetDirty()
{
	Locator::rendereringSystem::value().SetDirty();
}
} // namespace openblack::ecs
