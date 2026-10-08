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

#include <memory>

#include "ECS/ScriptObjectTable.h"
#include "ECS/ScriptObjectsWorld.h"
#include "ECS/Systems/ScriptObjectsSystemInterface.h"

namespace openblack::ecs::systems
{

class ScriptObjectsSystem final: public ScriptObjectsSystemInterface
{
public:
	/// Works on the game's own entities
	ScriptObjectsSystem();
	explicit ScriptObjectsSystem(std::unique_ptr<script_objects::World> world);

	bool Register(entt::entity object, bool createdByScript) override;
	void EnterNative(uint32_t native) override;
	entt::entity Fetch(entt::entity object) override;
	void AddReference(entt::entity object) override;
	void RemoveReference(entt::entity object) override;
	void ReleaseFromScript(entt::entity object) override;
	void Replace(entt::entity from, entt::entity to) override;
	void Reset() override;

private:
	/// A controlled object goes back into the game: control is cleared, and a living thing takes up what it does when
	/// no script holds it
	void ReleaseIntoGame(entt::entity object);

	std::unique_ptr<script_objects::World> _world;
	script_objects::Table _table;
	/// Whether the native running takes control of what it is given
	bool _takesControl {false};
};

} // namespace openblack::ecs::systems
