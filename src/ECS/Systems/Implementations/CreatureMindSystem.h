/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>
#include <random>
#include <vector>

#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureMindTables.h"
#include "Creature/CreaturePlanActions.h"
#include "Creature/CreaturePlanner.h"
#include "ECS/Components/CreatureCasting.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CreatureMindSystem final: public CreatureMindSystemInterface
{
public:
	void ProcessTurn() override;
	void PlanTurn() override;
	void LearnTurn() override;

	void LoadMind(entt::entity creature, std::shared_ptr<const creaturemind::MindFileData> mind) override;
	[[nodiscard]] std::optional<creaturemind::MindFileData> SaveMind(entt::entity creature) const override;
	void ClearLearning(entt::entity creature) override;
	void SeeSkill(const glm::vec3& point, size_t skill) override;
	void SeeMiracle(const glm::vec3& point, size_t miracle) override;
	void PlayerDid(size_t deed, const glm::vec3& point, std::optional<entt::entity> object,
	               std::optional<PlayerNames> player) override;
	[[nodiscard]] const creature_mind_tables::Tables* GetTables() override;

	bool PlayAction(entt::entity creature, size_t animation) override;
	bool PlayGesture(entt::entity creature, size_t animation) override;
	void PullFace(entt::entity creature, size_t animation) override;
	std::optional<creature_face::Request> ShowFeeling(entt::entity creature, creature_face::Cue cue) override;
	bool SitDown(entt::entity creature) override;
	void StandUp(entt::entity creature) override;
	void ReceiveFeedback(entt::entity creature, float feedback) override;
	void UpdateAttitudeFromFeedback(entt::entity creature, float feedback) override;
	void ChangeDesireSource(entt::entity creature, uint32_t type, float amount) override;
	bool ForceAction(entt::entity creature, size_t animation, bool mirrored, std::optional<creature_face::Request> face,
	                 float interruptsAfter) override;
	bool Sleep(entt::entity creature) override;
	bool Eat(entt::entity creature, std::optional<entt::entity> food) override;
	bool Drink(entt::entity creature) override;
	bool Poo(entt::entity creature) override;
	bool Puke(entt::entity creature) override;
	bool Faint(entt::entity creature) override;
	void Wake(entt::entity creature) override;
	void FoughtFight(entt::entity creature, bool won) override;
	void AbandonAction(entt::entity creature) override;
	void ForceCatch(entt::entity creature, entt::entity object) override;
	void ReactToNastyMagic(entt::entity creature, const glm::vec3& point, std::optional<size_t> learn) override;
	void ReactToNiceMagic(entt::entity creature, const glm::vec3& point, std::optional<size_t> learn) override;
	bool TryMiracle(entt::entity creature, MagicType type, entt::entity target) override;
	void KnowMiracle(entt::entity creature, size_t miracle) override;
	bool TellCast(entt::entity creature, MagicType type, entt::entity target) override;

private:
	/// Sets up what a creature has learnt the first time its mind thinks, from its mind file when it has one
	void SetUpLearning(entt::entity creature, components::CreatureMindState& mind);
	/// Takes up a mind file waiting to be loaded
	void TakeUpFile(entt::entity creature, components::CreatureMindState& mind);
	/// Remembers what the idle mind has started, for feedback to be credited to, and finishes plans that are done
	void FollowAgenda(entt::entity creature, components::CreatureMindState& mind);
	/// Carries out a plan in place of what the creature was doing; returns whether it could
	bool Adopt(entt::entity creature, components::CreatureMindState& mind, const creature_planner::Plan& plan,
	           const creature_plan_actions::Situation& situation);
	/// Gives up the plan carried out, if any
	static void Abandon(components::CreatureMindState& mind);
	/// Learns what feedback teaches, from what the creature did lately
	void LearnFromFeedback(entt::entity creature, components::CreatureMindState& mind, float feedback);
	/// Plans the desires due this turn for one creature
	/// Plans the desires due this turn for one creature, or all of them, at most once a turn
	void PlanCreature(entt::entity creature, components::CreatureMindState& mind, bool everyDesire = false);
	/// Uniform random numbers from 0 to n - 1, and from 0 to 1
	uint32_t Random(uint32_t range);
	float Chance();

	/// Plans an activity in place of what the creature was doing, getting it up and stopping it first
	bool Replan(entt::entity creature, creature_mind::Activity activity, std::vector<creature_mind::Step> agenda);
	/// The creature sees a miracle it reacted to, and learns from it
	void WatchMiracle(entt::entity creature, size_t miracle);

	// Casting miracles (CreatureMindCasting.cpp)
	/// The miracle an action of the game's table casts, if any
	static std::optional<uint32_t> CastMagicOf(uint32_t action);
	/// Whether the creature may try the miracle an action casts: seen often enough, able to pay for it, and for a
	/// power-up grown up enough
	bool MayCast(entt::entity creature, const components::CreatureMindState& mind, uint32_t action, bool powerUp);
	/// What the creature casts for an action, the gesture it draws first and its height
	std::optional<creature_plan_actions::CastInfo> CastInfoFor(entt::entity creature, uint32_t action);
	/// A fizzled try, shown as it next chooses what to do
	void ShowFizzle(entt::entity creature, components::CreatureMindState& mind);
	/// Going near, getting away from or turning to face an object begins, and each turn it goes on
	void StartSubMove(entt::entity creature, const creature_mind::Movement& movement, float seconds);
	void StepSubMove(entt::entity creature, bool animating);
	bool GoNear(entt::entity creature, components::CreatureCasting& casting, bool reissue);
	creature_mind::SubMove SubMoveOf(entt::entity creature);
	/// The minds choose at random, apart from the game's own random numbers
	std::mt19937 _random {std::random_device {}()};
	/// The game's tables for the minds, taken once the game's data is loaded
	std::optional<creature_mind_tables::Tables> _tables;
};

} // namespace openblack::ecs::systems
