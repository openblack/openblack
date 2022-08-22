/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Registry.h"

#include "Locator.h"
#include "Systems/RenderingSystemInterface.h"

namespace openblack::ecs
{

Registry::Registry()
{
	_registry.ctx().emplace<RegistryContext>();
}

RegistryContext& Registry::Context()
{
	return _registry.ctx().at<RegistryContext>();
}

const RegistryContext& Registry::Context() const
{
	return _registry.ctx().at<const RegistryContext>();
}

void Registry::SetDirty()
{
	Locator::rendereringSystem::value().SetDirty();
}
void Registry::Destroy(entt::entity entity)
{
	_registry.destroy(entity);
}
} // namespace openblack::ecs
