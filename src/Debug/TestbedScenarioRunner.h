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

#include <deque>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <entt/entity/entity.hpp>

#include "TestbedScenarioRegistry.h"

namespace openblack::testbed_scenarios
{

/// Plays a scenario on the testbed: loads the testbed afresh, which clears away everything on it, sets the time of
/// day, the weather and the creatures' body time, puts down the objects and creatures as the scenario has them, frames
/// the camera, then gives the commands in turn as their time comes. Everything goes through the game's systems.
class Runner
{
public:
	/// Loads the testbed and starts the scenario on it, in place of any other
	void Start(const Scenario& scenario);
	/// Stops giving commands and puts back the settings the scenario changed: the body time, fainting, the footprints'
	/// smileys and the clock. What is on the land stays, to look at or carry on with by hand.
	void Stop();
	/// Once a frame
	void Update(float seconds);

	[[nodiscard]] bool IsRunning() const { return _running; }
	/// The scenario running, or last run
	[[nodiscard]] const Scenario* GetScenario() const { return _scenario; }
	[[nodiscard]] float GetSeconds() const { return _seconds; }
	[[nodiscard]] const Timeline& GetTimeline() const { return _timeline; }
	/// The scenario's creatures by their place in it, null where one is gone
	[[nodiscard]] std::span<const entt::entity> GetCreatures() const { return _creatures; }
	/// What became of the last commands, the newest last
	[[nodiscard]] const std::deque<std::string>& GetLog() const { return _log; }

	/// Puts the camera on a shot of one of the scenario's creatures, or of all of them; following and close ups keep
	/// up with the creature until the camera is let go
	void Frame(Shot shot, size_t creature, float distance = 1.0f);
	/// The camera is the player's again
	void ReleaseCamera() { _shot.reset(); }
	[[nodiscard]] std::optional<Shot> GetShot() const { return _shot; }

private:
	void SetUpEnvironment(const Environment& environment);
	void PlaceObjects(const Scenario& scenario, glm::vec2 middle);
	void PlaceCreatures(const Scenario& scenario, glm::vec2 middle);
	/// The needs and desires go on once the body and mind have started, and every frame for those that hold them
	void ApplyStates();
	void Give(const Command& command);
	/// The commands on things, of the player's hand and of the leashes; each returns what came of it
	std::string GiveObjectCommand(entt::entity creature, const Command& command);
	std::string GiveHandCommand(entt::entity creature, const Command& command);
	std::string GiveLeashCommand(entt::entity creature, const Command& command);
	void UpdateCamera();
	[[nodiscard]] bool IsFree(size_t creature) const;
	[[nodiscard]] std::optional<entt::entity> CreatureAt(size_t index) const;
	/// The scenario's objects by their place in it, while they are still about
	[[nodiscard]] std::optional<entt::entity> ObjectAt(size_t index) const;
	void Log(std::string line);

	const Scenario* _scenario {nullptr};
	bool _running {false};
	float _seconds {0.0f};
	Timeline _timeline;
	glm::vec2 _middle {0.0f};
	std::vector<entt::entity> _creatures;
	std::vector<entt::entity> _objects;
	/// Whether each creature's needs and desires have been set as it started
	std::vector<bool> _started;
	std::deque<std::string> _log;

	std::optional<Shot> _shot;
	size_t _shotCreature {0};
	float _shotDistance {1.0f};
};

} // namespace openblack::testbed_scenarios
