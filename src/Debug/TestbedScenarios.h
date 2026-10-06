/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>

#include <optional>

#include <entt/entity/fwd.hpp>

#include "TestbedScenarioRegistry.h"
#include "TestbedScenarioRunner.h"
#include "Window.h"

namespace openblack::debug::gui
{
class CreatureSpawner;

/// Ready-made scenarios on the creature testbed, one facet of the creatures at a time: pick a facet and a scenario, read
/// what it sets up and what to look for, and run, restart or stop it. The game's speed and the creatures' body time
/// can be changed, the camera put on the scenario's creatures, and their activities, needs, desires and speed read as
/// they go, each to be picked in the creature spawner. A running scenario carries on with the window closed.
class TestbedScenarios final: public Window
{
public:
	explicit TestbedScenarios(CreatureSpawner& spawner) noexcept;

protected:
	void Draw() noexcept override;
	void Update() noexcept override {}
	void UpdateAlways() noexcept override;
	void ProcessEventOpen([[maybe_unused]] const SDL_Event& event) noexcept override {}
	void ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept override {}

private:
	void DrawPicker() noexcept;
	void DrawControls() noexcept;
	void DrawTime() noexcept;
	void DrawCamera() noexcept;
	void DrawCreatures() noexcept;

	CreatureSpawner& _spawner;
	testbed_scenarios::Runner _runner;
	/// The facet the list is narrowed to, or every facet
	std::optional<testbed_scenarios::Facet> _facet;
	size_t _picked {0};
	/// The scenario's creature the camera shots are of
	size_t _focus {0};
};

} // namespace openblack::debug::gui
