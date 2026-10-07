/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <random>

#include "ECS/Systems/CreatureFightSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CreatureFightSystem final: public CreatureFightSystemInterface
{
public:
	void ProcessTurn() override;
	void Update(std::chrono::duration<float, std::milli> gameTime) override;

	StartResult StartFight(entt::entity creature, entt::entity opponent) override;
	void AbortFight(entt::entity creature) override;
	[[nodiscard]] bool IsFighting(entt::entity creature) const override;
	[[nodiscard]] std::optional<entt::entity> OpponentOf(entt::entity creature) const override;

	bool QueueMove(entt::entity creature, const creature_fight::Move& move, bool replace) override;
	void ReleaseCharge(entt::entity creature, float heldMs) override;
	void SetAutoFighting(entt::entity creature, bool autoFight) override;
	[[nodiscard]] bool IsAutoFighting(entt::entity creature) const override;

	bool Press(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) override;
	void Release() override;
	[[nodiscard]] bool IsPressed() const override { return _pressed.has_value(); }

	void KnockOut(entt::entity creature) override;
	void KillPermanently(entt::entity creature) override;
	void Resurrect(entt::entity creature) override;
	[[nodiscard]] bool IsKnockedOut(entt::entity creature) const override;

	[[nodiscard]] std::optional<creature_fight_hud::Values> GetPanel() const override;

	void SetAngerStartsFights(bool enabled) override { _angerStartsFights = enabled; }
	[[nodiscard]] bool GetAngerStartsFights() const override { return _angerStartsFights; }
	void SetCameraWatches(bool enabled) override { _cameraWatches = enabled; }
	[[nodiscard]] bool GetCameraWatches() const override { return _cameraWatches; }
	[[nodiscard]] bool IsCameraOnFight() const final { return _watched.has_value(); }

private:
	/// The turn's parts: fights picked by angry creatures and started by the leash, the stages before and after the
	/// duel, the duel's moves, and the creatures knocked out
	void StartFightsFromMinds();
	void ProcessStages();
	void ProcessDuels();
	void ProcessKnockedOut();
	/// Makes the move at the front of a fighter's queue, if it can
	void CheckQueue(entt::entity creature);
	/// A blow at a band: struck, or a step taken towards where it would land
	void AttemptBlow(entt::entity creature, creature_fight::Band band, float speed);
	/// A fighter's action landing on its opponent this frame, if it does
	void TestHit(entt::entity creature);
	/// One creature beat the other: the loser faints and the winner shows off
	void Win(entt::entity winner, entt::entity loser);
	/// The fight ends for a creature: its life pays for it and it learns from it
	void EndFightFor(entt::entity creature, bool won);
	void BeginDuel(entt::entity creature);
	void MeasureBlows(entt::entity creature);
	/// Faints and lies out cold, to be taken home later, or back to where it started fighting
	void Faint(entt::entity creature, std::optional<glm::vec3> start);
	/// The camera watches a fight, from the side of its arena
	void Watch(const creature_fight::Arena& arena, glm::vec2 side);
	void FollowDuel();
	/// Leaves the fight for good, its mind taking over again
	void Leave(entt::entity creature);

	bool _angerStartsFights {true};
	bool _cameraWatches {true};
	/// The player's creature a press is charging a blow for, and how long it has been held
	struct Pressed
	{
		entt::entity creature;
		float heldMs;
	};
	std::optional<Pressed> _pressed;
	/// Where the camera was last sent to look at a fight
	std::optional<glm::vec2> _watched;
	std::mt19937 _random {std::random_device {}()};
};

} // namespace openblack::ecs::systems
