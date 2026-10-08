/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

#include "ECS/ScriptObjectTable.h"
#include "ECS/Systems/ScriptObjectsSystemInterface.h"

namespace openblack::ecs::systems
{

class ScriptObjectsSystem final: public ScriptObjectsSystemInterface
{
public:
	bool Register(entt::entity object, bool createdByScript) override;
	void EnterNative(uint32_t native) override;
	entt::entity Fetch(entt::entity object) override;
	void AddReference(entt::entity object) override;
	void RemoveReference(entt::entity object) override;
	void ReleaseFromScript(entt::entity object) override;
	void Replace(entt::entity from, entt::entity to) override;
	[[nodiscard]] bool IsCreatedByScript(entt::entity object) const override;
	void Reset() override;

private:
	script_objects::Table _table;
	/// Whether the native running takes control of what it is given
	bool _takesControl {false};
};

} // namespace openblack::ecs::systems
