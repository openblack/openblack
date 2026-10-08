/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "HandGrabRules.h"

namespace openblack::ecs
{
class Registry;
}

namespace openblack::ecs::systems
{
struct FromHand;
struct FromHandResult;
} // namespace openblack::ecs::systems

namespace openblack::hand_grab
{

/// What the god hand needs from the world to pick things up, hold them and let them go: what is under the cursor, the
/// facts of things the tables and models hold, and the other systems a pick-up or a release reaches. The game's world
/// answers from the game; tests answer with fakes.
class HandGrabWorldInterface
{
public:
	virtual ~HandGrabWorldInterface() = default;

	/// The things, and the hand's own state, which is kept on the hand
	[[nodiscard]] virtual ecs::Registry& Entities() = 0;
	[[nodiscard]] virtual entt::entity Hand() const = 0;
	/// The player whose hand it is
	[[nodiscard]] virtual PlayerNames HandPlayer() const = 0;

	/// The thing the interface picks under the cursor, none over the land or nothing
	[[nodiscard]] virtual std::optional<entt::entity> ObjectUnderCursor() const = 0;
	/// Whether a point is in a player's influence
	[[nodiscard]] virtual bool InInfluence(PlayerNames player, glm::vec3 point) const = 0;
	/// Whether a point is on the map
	[[nodiscard]] virtual bool InBounds(glm::vec3 point) const = 0;
	/// The flat normal of the land at a point
	[[nodiscard]] virtual glm::vec3 LandNormalAt(glm::vec3 point) const = 0;

	/// How big a thing stands at its scale
	struct Size
	{
		float radius {0.0f};
		float height {0.0f};
	};
	[[nodiscard]] virtual Size SizeOf(entt::entity object) const = 0;
	/// Its weight: its kind's weight at its scale
	[[nodiscard]] virtual float WeightOf(entt::entity object) const = 0;
	[[nodiscard]] virtual float LifeOf(entt::entity object) const = 0;
	/// Whether it flies in the physics
	[[nodiscard]] virtual bool IsFlying(entt::entity object) const = 0;
	/// Whether its kind of animal may be picked up by a player
	[[nodiscard]] virtual bool SpeciesAllowsPickUp(entt::entity animal) const = 0;
	/// A static's kind of thing (rock, toy, ...) and its model, from its row of the tables
	[[nodiscard]] virtual MobileStaticInfo StaticKindOf(MobileStaticInfo type) const = 0;
	[[nodiscard]] virtual MeshId StaticMeshOf(MobileStaticInfo type) const = 0;
	/// Whether a pot of its kind is a loose pot, rather than a pile or a handful
	[[nodiscard]] virtual bool IsLoosePot(PotInfo type) const = 0;
	/// Whether it is made of the physics' rock material, which doesn't lean or stretch as it is pulled
	[[nodiscard]] virtual bool IsOfRockMaterial(entt::entity object) const = 0;
	/// Whether a villager is old enough to make love and not yet too old
	[[nodiscard]] virtual bool IsSexuallyActive(entt::entity villager) const = 0;
	/// The player a thing belongs to, none for a thing of nobody's
	[[nodiscard]] virtual std::optional<PlayerNames> PlayerOf(entt::entity object) const = 0;

	/// Taking a thing: it leaves the physics without landing and the map's cells
	virtual void LeavePhysicsAndMap(entt::entity object) = 0;
	/// The people and animals about react to something
	struct ReactionRequest
	{
		entt::entity initiator {entt::null};
		Reaction type {Reaction::None};
		PlayerNames player {PlayerNames::NEUTRAL};
		glm::vec3 position {0.0f};
	};
	virtual void CreateReaction(const ReactionRequest& request) = 0;
	virtual void RemoveReactions(entt::entity initiator, Reaction type) = 0;
	/// A burning thing starts moving, into a hand (or it is a villager) or not
	virtual void FireStartedMoving(entt::entity object, bool inHand) = 0;
	/// A held thing catches from the fires in its cell
	virtual void HeatHeld(entt::entity object) = 0;
	/// A sound of the in-game bank where something is
	virtual void PlaySample(uint32_t sample, glm::vec3 position) = 0;
	/// A local random number below a count
	[[nodiscard]] virtual uint32_t LocalRandom(uint32_t count) = 0;
	/// A villager goes into the hand: it remembers what it was doing and is held
	virtual void VillagerIntoHand(entt::entity villager) = 0;
	/// An animal leaves its flock for one of its own, the old one going if it is left empty
	virtual void AnimalIntoOwnFlock(entt::entity animal) = 0;
	/// Pulling up a tree moves the player's alignment
	virtual void TreeUprooted(PlayerNames player, entt::entity tree) = 0;
	/// A tree pulled out of the ground leaves its roots in a hole, with a puff of smoke
	virtual void LeaveRootsHole(entt::entity tree) = 0;
	/// A pot taken stops calling the people to it
	virtual void RemovePotReaction(entt::entity pot) = 0;
	/// A pot let go over the land calls the people to it again, when it calls nobody yet, for the player letting it go
	virtual void SetUpPotReaction(entt::entity pot, PlayerNames player) = 0;
	/// Where a thing is drawn, its scaled axes and its origin
	struct Pose
	{
		glm::mat3 axes {1.0f};
		glm::vec3 origin {0.0f};
	};
	[[nodiscard]] virtual Pose PoseOf(entt::entity object) const = 0;
	virtual void SetPose(entt::entity object, const Pose& pose) = 0;
	/// A thing's axes at its own size, as they are once it stops being stretched
	[[nodiscard]] virtual glm::mat3 UnstretchedAxes(entt::entity object, const glm::mat3& axes) const = 0;

	/// Letting go: where it starts from as it leaves the hand, lifted out of the land and laid along it or not
	[[nodiscard]] virtual std::optional<Pose> ReleasePose(entt::entity object, bool alignToSlope) = 0;
	/// A pot let go slowly is poured out where it is: what it holds goes to what takes it there or into a pile, and the
	/// pot goes
	virtual std::optional<uint32_t> PourPot(entt::entity pot, PlayerNames player) = 0;

	/// What a pile or pot holds, and what its handful's table says of scooping it
	struct PotFacts
	{
		PotType potType {PotType::Pot};
		ResourceType resource {ResourceType::Food};
		/// The handful the hand scoops out of it
		PotInfo handful {PotInfo::HandFood};
		uint32_t amount {0};
	};
	[[nodiscard]] virtual std::optional<PotFacts> PotFactsOf(entt::entity pot) const = 0;
	/// What a field has for the hand to scoop: its food, and whether its crop is ripe (a ripe crop gives half)
	struct FieldFacts
	{
		uint32_t food {0};
		bool ripe {false};
	};
	[[nodiscard]] virtual std::optional<FieldFacts> FieldFactsOf(entt::entity field) const = 0;
	/// Food is taken out of a field by the hand
	virtual void TakeFromField(entt::entity field, uint32_t amount) = 0;
	/// A pot, a handful among them, is drawn at the size of what it holds
	virtual void ResizePot(entt::entity pot) = 0;
	[[nodiscard]] virtual ScoopFacts ScoopFactsOf(PotInfo handful) const = 0;
	/// Some of what a pile holds is taken out of it: no more than it has. The pile goes once it has nothing left, unless
	/// it is a store's. What was taken.
	virtual uint32_t TakeFromPile(entt::entity pile, uint32_t amount) = 0;
	/// A handful the hand holds is made, holding so much, where the hand is
	[[nodiscard]] virtual entt::entity MakeHandful(PotInfo type, glm::vec3 position, uint32_t amount) = 0;
	/// The stream of what is scooped flowing from its source into the hand starts, and stops; its effect
	[[nodiscard]] virtual std::optional<uint32_t> StartScoopStream(ResourceType resource, glm::vec3 source) = 0;
	virtual void StopScoopStream(uint32_t stream) = 0;
	/// The stream flows to where the hand now is
	virtual void MoveScoopStream(uint32_t stream, glm::vec3 hand) = 0;
	/// The scooping sound of a game turn, rising as the scoop ramps up
	virtual void PlayScoopSound(ResourceType resource, glm::vec3 hand, float ramp) = 0;
	[[nodiscard]] virtual float LandHeightAt(glm::vec3 point) const = 0;
	/// Whether something stores a resource, as a village store stores food and wood
	[[nodiscard]] virtual bool StoresResource(entt::entity store, ResourceType resource) const = 0;
	/// A store takes what it will of some resource; what it took
	virtual uint32_t AddToStore(entt::entity store, ResourceType resource, uint32_t amount) = 0;
	/// A store takes a thing whole, for the resource it is worth, and the thing goes; whether it took it
	virtual bool TakeIntoStore(entt::entity store, entt::entity object) = 0;
	/// Some resource poured onto a point: to the stores and piles of it about the point, or a new pile
	virtual void PourAt(ResourceType resource, glm::vec3 point, uint32_t amount, PlayerNames player) = 0;
	/// A thing is used up: a ghost of it flickers out where it was, and it goes
	virtual void UseUp(entt::entity object) = 0;
	/// It goes into the physics from the hand, put down or thrown
	virtual ecs::systems::FromHandResult LetGoFromHand(entt::entity object, const ecs::systems::FromHand& release) = 0;
	/// Whether the player's hand throws things without the air's drag, as a script can set
	[[nodiscard]] virtual bool PlayerHasNoWindResistance(PlayerNames player) const = 0;
	/// A body's mass and speed, none for a thing not in the physics
	struct BodyFacts
	{
		float mass {0.0f};
		float speed {0.0f};
	};
	[[nodiscard]] virtual std::optional<BodyFacts> BodyOf(entt::entity object) const = 0;
	/// A turning force on a body for the rest of the game turn
	virtual void TwistBody(entt::entity object, glm::vec3 torque) = 0;
	/// The body of what the hand let go flies without the air's drag
	virtual void DropDrag(entt::entity object) = 0;
};

} // namespace openblack::hand_grab
