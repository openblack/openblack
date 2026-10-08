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
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

#include "Hand/HandGrabWorld.h"

namespace openblack::ecs::systems
{

/// The god hand's world in the game: the things on the map and the systems a pick-up or a release reaches
class GameHandGrabWorld final: public hand_grab::HandGrabWorldInterface
{
public:
	[[nodiscard]] Registry& Entities() override;
	[[nodiscard]] entt::entity Hand() const override;
	[[nodiscard]] PlayerNames HandPlayer() const override;

	[[nodiscard]] std::optional<entt::entity> ObjectUnderCursor() const override;
	[[nodiscard]] bool InInfluence(PlayerNames player, glm::vec3 point) const override;
	[[nodiscard]] bool InBounds(glm::vec3 point) const override;
	[[nodiscard]] glm::vec3 LandNormalAt(glm::vec3 point) const override;

	[[nodiscard]] Size SizeOf(entt::entity object) const override;
	[[nodiscard]] float WeightOf(entt::entity object) const override;
	[[nodiscard]] float LifeOf(entt::entity object) const override;
	[[nodiscard]] bool IsFlying(entt::entity object) const override;
	[[nodiscard]] bool SpeciesAllowsPickUp(entt::entity animal) const override;
	[[nodiscard]] MobileStaticInfo StaticKindOf(MobileStaticInfo type) const override;
	[[nodiscard]] MeshId StaticMeshOf(MobileStaticInfo type) const override;
	[[nodiscard]] bool IsLoosePot(PotInfo type) const override;
	[[nodiscard]] bool IsOfRockMaterial(entt::entity object) const override;
	[[nodiscard]] bool IsSexuallyActive(entt::entity villager) const override;
	[[nodiscard]] std::optional<PlayerNames> PlayerOf(entt::entity object) const override;

	void LeavePhysicsAndMap(entt::entity object) override;
	void CreateReaction(const ReactionRequest& request) override;
	void RemoveReactions(entt::entity initiator, Reaction type) override;
	void FireStartedMoving(entt::entity object, bool inHand) override;
	void HeatHeld(entt::entity object) override;
	void PlaySample(uint32_t sample, glm::vec3 position) override;
	bool TapThing(entt::entity object, glm::vec3 handPoint, PlayerNames player) override;
	[[nodiscard]] uint32_t LocalRandom(uint32_t count) override;
	void VillagerIntoHand(entt::entity villager) override;
	void AnimalIntoOwnFlock(entt::entity animal) override;
	void ArtefactTaken(entt::entity object, PlayerNames player) override;
	void TreeUprooted(PlayerNames player, entt::entity tree) override;
	void LeaveRootsHole(entt::entity tree) override;
	void RemovePotReaction(entt::entity pot) override;
	void SetUpPotReaction(entt::entity pot, PlayerNames player) override;
	[[nodiscard]] Pose PoseOf(entt::entity object) const override;
	void SetPose(entt::entity object, const Pose& pose) override;
	[[nodiscard]] glm::mat3 UnstretchedAxes(entt::entity object, const glm::mat3& axes) const override;

	[[nodiscard]] std::optional<Pose> ReleasePose(entt::entity object, bool alignToSlope) override;
	std::optional<uint32_t> PourPot(entt::entity pot, PlayerNames player) override;
	[[nodiscard]] std::optional<PotFacts> PotFactsOf(entt::entity pot) const override;
	[[nodiscard]] hand_grab::ScoopFacts ScoopFactsOf(PotInfo handful) const override;
	uint32_t TakeFromPile(entt::entity pile, uint32_t amount) override;
	[[nodiscard]] std::optional<FieldFacts> FieldFactsOf(entt::entity field) const override;
	void TakeFromField(entt::entity field, uint32_t amount) override;
	void ResizePot(entt::entity pot) override;
	[[nodiscard]] entt::entity MakeHandful(PotInfo type, glm::vec3 position, uint32_t amount, bool poisoned) override;
	[[nodiscard]] std::optional<uint32_t> StartScoopStream(ResourceType resource, glm::vec3 source) override;
	void StopScoopStream(uint32_t stream) override;
	void MoveScoopStream(uint32_t stream, glm::vec3 hand) override;
	void PlayScoopSound(ResourceType resource, glm::vec3 hand, float ramp) override;
	[[nodiscard]] float LandHeightAt(glm::vec3 point) const override;
	[[nodiscard]] bool StoresResource(entt::entity store, ResourceType resource) const override;
	uint32_t AddToStore(entt::entity store, ResourceType resource, uint32_t amount, bool poisoned) override;
	bool TakeIntoStore(entt::entity store, entt::entity object) override;
	void PourAt(ResourceType resource, glm::vec3 point, uint32_t amount, PlayerNames player, bool poisoned) override;
	void UseUp(entt::entity object) override;
	FromHandResult LetGoFromHand(entt::entity object, const FromHand& release) override;
	[[nodiscard]] bool PlayerHasNoWindResistance(PlayerNames player) const override;
	[[nodiscard]] std::optional<BodyFacts> BodyOf(entt::entity object) const override;
	void TwistBody(entt::entity object, glm::vec3 torque) override;
	void DropDrag(entt::entity object) override;

private:
	/// The piles and stores what is poured out of a pot goes into, as the miracles' food and wood do
};

} // namespace openblack::ecs::systems
