/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Registry.h"

#include "Systems/RenderingSystem.h"

namespace openblack::ecs
{

Registry::Registry()
{
	_registry.set<RegistryContext>();
}

RegistryContext& Registry::Context()
{
	return _registry.ctx<RegistryContext>();
}

const RegistryContext& Registry::Context() const
{
	return _registry.ctx<const RegistryContext>();
}

void Registry::SetDirty()
{
	systems::RenderingSystem::instance().SetDirty();
}
} // namespace openblack::ecs
