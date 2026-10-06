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
#include <cstdint>

#include <deque>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <entt/entity/entity.hpp>

#include "BenchmarkRecorder.h"
#include "TestbedScenarioRegistry.h"

namespace openblack::testbed_scenarios
{

/// How far spawning a scenario's crowd has got
struct CrowdProgress
{
	size_t spawned {0};
	size_t total {0};
	/// The time spent spawning, apart from the rest of the frames it took
	double spawnMs {0.0};
	uint32_t spawnFrames {0};

	[[nodiscard]] bool Done() const { return spawned >= total; }
};

/// How a crowd's frames are measured once it has all spawned
struct BenchmarkSettings
{
	/// Frames left to settle first, as the crowd's minds and routes start
	uint32_t warmUpFrames {120};
	/// The last so many frames are measured
	uint32_t frames {600};
	/// Where the results are written, with .json and .csv after it, once that many frames are measured, after which the
	/// game quits; for running a benchmark from the command line
	std::optional<std::filesystem::path> autoSave;
};

/// Plays a scenario on the testbed: loads the testbed afresh, which clears away everything on it, sets the time of
/// day, the weather and the creatures' body time, puts down the objects and creatures as the scenario has them, frames
/// the camera, then gives the commands in turn as their time comes. Everything goes through the game's systems.
class Runner
{
public:
	/// Loads the testbed and starts the scenario on it, in place of any other
	void Start(const Scenario& scenario);
	/// Stops giving commands and puts back the settings the scenario changed: the body time, fainting, the footprints'
	/// smileys, the clock and whether anger starts fights. What is on the land stays, to look at or carry on with by hand.
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

	/// The scenario's crowd as it spawns, if it has one
	[[nodiscard]] std::optional<CrowdProgress> GetCrowdProgress() const;
	void SetBenchmarkSettings(BenchmarkSettings settings) { _benchmark = std::move(settings); }
	[[nodiscard]] const BenchmarkSettings& GetBenchmarkSettings() const { return _benchmark; }
	/// The frames still to settle before the crowd's frames are measured, and how many have been
	[[nodiscard]] uint32_t GetWarmUpLeft() const;
	[[nodiscard]] size_t GetMeasuredFrames() const { return _recorder ? _recorder->Count() : 0; }
	/// The frames measured so far, summed up every so often as they are
	[[nodiscard]] const benchmark::Results& GetLiveResults() const { return _liveResults; }
	[[nodiscard]] std::span<const benchmark::StageInfo> GetStages() const;
	/// How many entities there are of each kind
	[[nodiscard]] std::vector<std::pair<std::string, size_t>> EntityCounts() const;
	/// Writes the results of the frames measured so far to the path with .json and .csv after it, or to the benchmarks
	/// folder under the scenario's id and the time when none is given; the JSON file's path, or none if they couldn't be
	/// written
	std::optional<std::filesystem::path> SaveResults(std::optional<std::filesystem::path> base = std::nullopt);

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
	/// Spawns the next batch of the crowd
	void SpawnCrowd();
	void SpawnCrowdMember(size_t index);
	/// Takes the last frame's times from the profiler, once the crowd has spawned and settled
	void Measure();
	/// Starts the scenario's particle effect of that index
	[[nodiscard]] uint32_t StartParticle(size_t index) const;
	/// The scenario's dispensers and lone bubbles, after the testbed's grid stays or goes as it asks
	void PlaceDispensers(const Scenario& scenario);
	/// Casts the scenario's miracles as their times come, and lets go of held ones when theirs are up
	void UpdateMiracles();
	/// Casts the scenario's miracle of that index; the running miracle, or none
	entt::entity CastMiracle(size_t index);
	void UpdateParticles(float seconds);
	/// The needs and desires go on once the body and mind have started, and every frame for those that hold them
	void ApplyStates();
	void Give(const Command& command);
	/// The commands on things, of the player's hand and of the leashes; each returns what came of it
	std::string GiveObjectCommand(entt::entity creature, const Command& command);
	std::string GiveHandCommand(entt::entity creature, const Command& command);
	std::string GiveLeashCommand(entt::entity creature, const Command& command);
	/// The commands of fights, and of being knocked out and brought round
	std::string GiveFightCommand(entt::entity creature, const Command& command);
	/// Creature Mode's and the Creature Cave's commands, as the player's keys and clicks give them
	std::string GiveCreatureModeCommand(entt::entity creature, const Command& command);
	/// Sets a desire or the stage of growing up, or rewards what the creature last did by the kind of thing it was to
	std::string TeachMind(entt::entity entity, const Command& command);
	/// Loads a mind file named as a scenario names it into a creature
	void LoadMindFile(entt::entity entity, std::string_view name);
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
	/// The scenario's particle effects, and the seconds since each was last started
	struct RunningParticle
	{
		uint32_t effect;
		float seconds;
	};
	std::vector<RunningParticle> _particles;
	/// The scenario's miracles: when each is next cast, and the one held in its hand until when
	struct RunningMiracle
	{
		std::optional<float> nextAt;
		std::optional<float> letGoAt;
		entt::entity spell {entt::null};
	};
	std::vector<RunningMiracle> _miracles;
	/// Whether each creature's needs and desires have been set as it started
	std::vector<bool> _started;

	std::deque<std::string> _log;

	/// The crowd laid out, the next of it to spawn, its homes and towns as they have spawned, and how long it took
	std::vector<CrowdCreature> _crowdCreatures;
	VillageLayout _village;
	size_t _crowdNext {0};
	std::vector<entt::entity> _crowdAbodes;
	std::vector<entt::entity> _crowdEntities;
	CrowdProgress _crowdProgress;

	BenchmarkSettings _benchmark;
	uint32_t _settledFrames {0};
	std::unique_ptr<benchmark::Recorder> _recorder;
	benchmark::Results _liveResults;
	uint32_t _framesSinceSummary {0};
	bool _saved {false};

	std::optional<Shot> _shot;
	size_t _shotCreature {0};
	float _shotDistance {1.0f};
};

} // namespace openblack::testbed_scenarios
