/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#if !defined(LOCATOR_IMPLEMENTATIONS)
#warning "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

#include <entt/entt.hpp>

#include "ECS/RegistryContext.h"
#include "Registry.h"

namespace openblack
{
class Camera;
}

namespace openblack::graphics
{
class DebugLines;
class ShaderManager;
} // namespace openblack::graphics

namespace openblack::ecs
{
class RegistryProduction final: public RegistryInterface
{
public:
	RegistryProduction();

	void Release(entt::entity entity) override;
	void Destroy(entt::entity entity) override;
	void SetDirty() override;
	RegistryContext& Context() override;
	[[nodiscard]] const RegistryContext& Context() const override;
	void Reset() override;
};

} // namespace openblack::ecs
