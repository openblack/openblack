/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include "ECS/Components/HandGrab.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/Implementations/HandGrabSystem.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// Flat land everywhere, things standing where they are put, and a record of what the hand did
class FakeWorld final: public hand_grab::HandGrabWorldInterface
{
public:
	FakeWorld()
	{
		hand = registry.Create();
		registry.Assign<Transform>(hand, glm::vec3(0.0f, 10.0f, 0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
	}

	entt::entity AddRock(glm::vec3 position, float radius = 1.0f)
	{
		const auto rock = registry.Create();
		registry.Assign<Transform>(rock, position, glm::mat3(1.0f), glm::vec3(1.0f));
		registry.Assign<MobileStatic>(rock, MobileStaticInfo::Rock);
		sizes[rock] = {.radius = radius, .height = 2.0f};
		return rock;
	}

	entt::entity AddTree(glm::vec3 position, float weight)
	{
		const auto tree = registry.Create();
		registry.Assign<Transform>(tree, position, glm::mat3(1.0f), glm::vec3(1.0f));
		registry.Assign<Tree>(tree);
		sizes[tree] = {.radius = 1.0f, .height = 10.0f};
		weights[tree] = weight;
		return tree;
	}

	Registry& Entities() override { return registry; }
	[[nodiscard]] entt::entity Hand() const override { return hand; }
	[[nodiscard]] PlayerNames HandPlayer() const override { return PlayerNames::PLAYER_ONE; }
	[[nodiscard]] std::optional<entt::entity> ObjectUnderCursor() const override { return underCursor; }
	[[nodiscard]] bool InInfluence(PlayerNames, glm::vec3) const override { return influence; }
	[[nodiscard]] bool InBounds(glm::vec3) const override { return true; }
	[[nodiscard]] glm::vec3 LandNormalAt(glm::vec3) const override { return {0.0f, 1.0f, 0.0f}; }
	[[nodiscard]] Size SizeOf(entt::entity object) const override
	{
		const auto found = sizes.find(object);
		return found != sizes.end() ? found->second : Size {};
	}
	[[nodiscard]] float WeightOf(entt::entity object) const override
	{
		const auto found = weights.find(object);
		return found != weights.end() ? found->second : 1.0f;
	}
	[[nodiscard]] float LifeOf(entt::entity) const override { return 1.0f; }
	[[nodiscard]] bool IsFlying(entt::entity object) const override { return flying.contains(object); }
	[[nodiscard]] bool SpeciesAllowsPickUp(entt::entity) const override { return true; }
	[[nodiscard]] MobileStaticInfo StaticKindOf(MobileStaticInfo) const override { return MobileStaticInfo::Rock; }
	[[nodiscard]] MeshId StaticMeshOf(MobileStaticInfo) const override { return MeshId::Dummy; }
	[[nodiscard]] bool IsLoosePot(PotInfo type) const override { return type == PotInfo::FoodPot; }
	[[nodiscard]] bool IsOfRockMaterial(entt::entity object) const override { return registry.AllOf<MobileStatic>(object); }
	[[nodiscard]] bool IsSexuallyActive(entt::entity) const override { return true; }
	[[nodiscard]] std::optional<PlayerNames> PlayerOf(entt::entity) const override { return PlayerNames::PLAYER_ONE; }

	void LeavePhysicsAndMap(entt::entity object) override
	{
		leftWorld.push_back(object);
		flying.erase(object);
	}
	void CreateReaction(const ReactionRequest& request) override { reactions.push_back(request); }
	void RemoveReactions(entt::entity, Reaction) override {}
	void FireStartedMoving(entt::entity, bool) override {}
	void HeatHeld(entt::entity object) override { heated.push_back(object); }
	void ArtefactTaken(entt::entity object, PlayerNames) override { artefactsTaken.push_back(object); }
	void PlaySample(uint32_t sample, glm::vec3) override { samples.push_back(sample); }
	bool TapThing(entt::entity object, glm::vec3, PlayerNames) override
	{
		tapped.push_back(object);
		return true;
	}
	[[nodiscard]] uint32_t LocalRandom(uint32_t) override { return 0; }
	void VillagerIntoHand(entt::entity villager) override { villagersInHand.push_back(villager); }
	void AnimalIntoOwnFlock(entt::entity) override {}
	void TreeUprooted(PlayerNames, entt::entity tree) override { uprooted.push_back(tree); }
	void LeaveRootsHole(entt::entity tree) override { holes.push_back(tree); }
	void RemovePotReaction(entt::entity) override {}
	void SetUpPotReaction(entt::entity pot, PlayerNames) override { potReactionsSetUp.push_back(pot); }
	[[nodiscard]] Pose PoseOf(entt::entity object) const override
	{
		const auto& transform = registry.Get<const Transform>(object);
		auto axes = transform.rotation;
		for (glm::length_t column = 0; column < 3; ++column)
		{
			axes[column] *= transform.scale[column];
		}
		return {.axes = axes, .origin = transform.position};
	}
	void SetPose(entt::entity object, const Pose& pose) override
	{
		auto& transform = registry.Get<Transform>(object);
		for (glm::length_t column = 0; column < 3; ++column)
		{
			const float length = glm::length(pose.axes[column]);
			transform.rotation[column] = pose.axes[column] / length;
			transform.scale[column] = length;
		}
		transform.position = pose.origin;
	}
	[[nodiscard]] glm::mat3 UnstretchedAxes(entt::entity, const glm::mat3& axes) const override
	{
		glm::mat3 result = axes;
		for (glm::length_t column = 0; column < 3; ++column)
		{
			result[column] = glm::normalize(axes[column]);
		}
		return result;
	}
	[[nodiscard]] std::optional<Pose> ReleasePose(entt::entity, bool) override { return std::nullopt; }
	std::optional<uint32_t> PourPot(entt::entity pot, PlayerNames) override
	{
		poured.push_back(pot);
		return ++streams;
	}
	[[nodiscard]] std::optional<FieldFacts> FieldFactsOf(entt::entity field) const override
	{
		const auto found = fields.find(field);
		return found != fields.end() ? std::optional(found->second) : std::nullopt;
	}
	void ResizePot(entt::entity pot) override { resized.push_back(pot); }
	void TakeFromField(entt::entity field, uint32_t amount) override
	{
		auto& facts = fields.at(field);
		facts.food -= std::min(facts.food, amount);
	}
	[[nodiscard]] std::optional<PotFacts> PotFactsOf(entt::entity pot) const override
	{
		const auto* data = registry.TryGet<const Pot>(pot);
		if (data == nullptr)
		{
			return std::nullopt;
		}
		const bool handful = data->type == PotInfo::HandFood;
		return PotFacts {.potType = handful ? PotType::Pot : PotType::PileFood,
		                 .resource = ResourceType::Food,
		                 .handful = PotInfo::HandFood,
		                 .amount = data->amount,
		                 .poisoned = data->poisoned};
	}
	[[nodiscard]] hand_grab::ScoopFacts ScoopFactsOf(PotInfo) const override
	{
		return {.initial = 25, .perTurn = 8, .perTurnEnd = 70, .maxPickedUp = 20000, .rampSeconds = 6.0f};
	}
	uint32_t TakeFromPile(entt::entity pile, uint32_t amount) override
	{
		auto& pot = registry.Get<Pot>(pile);
		const auto taken = std::min(amount, pot.amount);
		pot.amount -= taken;
		if (pot.amount == 0)
		{
			registry.Destroy(pile);
		}
		return taken;
	}
	[[nodiscard]] entt::entity MakeHandful(PotInfo type, glm::vec3 position, uint32_t amount, bool poisoned) override
	{
		const auto handful = registry.Create();
		registry.Assign<Transform>(handful, position, glm::mat3(1.0f), glm::vec3(1.0f));
		registry.Assign<Pot>(handful, Pot {.amount = amount, .maxAmount = 20000, .type = type, .poisoned = poisoned});
		sizes[handful] = {.radius = 0.5f, .height = 1.0f};
		return handful;
	}
	[[nodiscard]] std::optional<uint32_t> StartScoopStream(ResourceType, glm::vec3, bool) override
	{
		++streams;
		return streams;
	}
	void StopScoopStream(uint32_t stream) override { stopped.push_back(stream); }
	void MoveScoopStream(uint32_t, glm::vec3 hand) override { streamPoints.push_back(hand); }
	void PinCursor(bool pinned) override { cursorPinned = pinned; }
	void PlayScoopSound(ResourceType, glm::vec3, float ramp) override { scoopSounds.push_back(ramp); }
	[[nodiscard]] float LandHeightAt(glm::vec3) const override { return 0.0f; }
	[[nodiscard]] bool StoresResource(entt::entity store, ResourceType) const override { return stores.contains(store); }
	uint32_t AddToStore(entt::entity store, ResourceType, uint32_t amount, bool /*poisoned*/) override
	{
		stored[store] += amount;
		return amount;
	}
	bool TakeIntoStore(entt::entity store, entt::entity object) override
	{
		if (!stores.contains(store))
		{
			return false;
		}
		takenWhole.emplace_back(store, object);
		registry.Destroy(object);
		return true;
	}
	void PourAt(ResourceType, glm::vec3, uint32_t amount, PlayerNames, bool /*poisoned*/) override
	{
		pouredAmounts.push_back(amount);
	}
	void UseUp(entt::entity object) override
	{
		usedUp.push_back(object);
		registry.Destroy(object);
	}
	FromHandResult LetGoFromHand(entt::entity object, const FromHand& release) override
	{
		released.emplace_back(object, release.velocity);
		return {.accepted = true};
	}
	[[nodiscard]] bool PlayerHasNoWindResistance(PlayerNames) const override { return false; }
	[[nodiscard]] std::optional<BodyFacts> BodyOf(entt::entity object) const override
	{
		return bodies.contains(object) ? std::optional(BodyFacts {.mass = 2.0f, .speed = 3.0f}) : std::nullopt;
	}
	void TwistBody(entt::entity object, glm::vec3 torque) override { twists.emplace_back(object, torque); }
	void DropDrag(entt::entity) override {}

	Registry registry;
	entt::entity hand {entt::null};
	std::optional<entt::entity> underCursor;
	bool influence {true};
	std::map<entt::entity, Size> sizes;
	std::map<entt::entity, float> weights;
	std::set<entt::entity> flying;
	std::set<entt::entity> bodies;
	std::vector<entt::entity> leftWorld;
	std::vector<ReactionRequest> reactions;
	std::vector<entt::entity> heated;
	std::vector<entt::entity> artefactsTaken;
	std::vector<uint32_t> samples;
	std::vector<entt::entity> tapped;
	std::vector<entt::entity> villagersInHand;
	std::vector<entt::entity> uprooted;
	std::vector<entt::entity> holes;
	std::vector<entt::entity> poured;
	std::vector<entt::entity> potReactionsSetUp;
	std::map<entt::entity, FieldFacts> fields;
	std::vector<std::pair<entt::entity, entt::entity>> takenWhole;
	std::vector<std::pair<entt::entity, glm::vec3>> released;
	std::vector<std::pair<entt::entity, glm::vec3>> twists;
	uint32_t streams {0};
	std::vector<uint32_t> stopped;
	std::vector<glm::vec3> streamPoints;
	std::vector<entt::entity> resized;
	std::vector<float> scoopSounds;
	bool cursorPinned {false};
	std::set<entt::entity> stores;
	std::map<entt::entity, uint32_t> stored;
	std::vector<uint32_t> pouredAmounts;
	std::vector<entt::entity> usedUp;
};

} // namespace

class HandGrabSystemWithWorld: public ::testing::Test
{
protected:
	HandGrabSystemWithWorld()
	{
		auto owned = std::make_unique<FakeWorld>();
		world = owned.get();
		system = std::make_unique<HandGrabSystem>(std::move(owned));
		// The hand has been over the land a frame before anything is pressed
		Frame(0);
	}

	glm::vec3 Frame(uint32_t gameMs, glm::vec3 target = {0.0f, 0.0f, 0.0f}, glm::vec3 ground = {0.0f, 0.0f, 0.0f})
	{
		now += gameMs;
		return system->UpdateFrame({.target = target,
		                            .rayOrigin = target + glm::vec3(0.0f, 50.0f, 0.0f),
		                            .rayDirection = {0.0f, -1.0f, 0.0f},
		                            .cursorGround = ground,
		                            .handSize = 1.0f,
		                            .seconds = static_cast<float>(gameMs) * 0.001f,
		                            .gameMs = gameMs,
		                            .nowMs = now,
		                            .turn = now / 100});
	}

	bool Press() { return system->Press(now, now / 100); }
	std::optional<entt::entity> Release() { return system->Release(now, now / 100); }

	FakeWorld* world {nullptr};
	std::unique_ptr<HandGrabSystem> system;
	uint32_t now {1000};
};

TEST_F(HandGrabSystemWithWorld, ARockIsTakenOnceTheHandHasFadedIntoItsPull)
{
	const auto rock = world->AddRock({0.0f, 0.0f, 0.0f});
	world->underCursor = rock;
	EXPECT_TRUE(Press());
	EXPECT_TRUE(system->IsBusy());
	// The pull waits for the 0.13 s fade, then anything but a tree comes free at once and is taken the frame after
	for (int i = 0; i < 20 && !system->GetHeld().has_value(); ++i)
	{
		Frame(10);
	}
	ASSERT_TRUE(system->GetHeld().has_value());
	EXPECT_EQ(*system->GetHeld(), rock);
	EXPECT_TRUE(world->registry.AllOf<InHand>(rock));
	EXPECT_EQ(world->leftWorld, std::vector<entt::entity> {rock});
	// It sounds as it is picked up, and the people about the hand see it
	ASSERT_FALSE(world->samples.empty());
	EXPECT_EQ(world->samples.front(), 10u);
	ASSERT_FALSE(world->reactions.empty());
	EXPECT_EQ(world->reactions.front().type, Reaction::ReactToHandPickUp);
	EXPECT_EQ(world->reactions.front().initiator, world->hand);
}

TEST_F(HandGrabSystemWithWorld, AShortPressIsATap)
{
	const auto rock = world->AddRock({0.0f, 0.0f, 0.0f});
	world->underCursor = rock;
	EXPECT_TRUE(Press());
	Frame(50);
	const auto tapped = Release();
	ASSERT_TRUE(tapped.has_value());
	EXPECT_EQ(*tapped, rock);
	EXPECT_FALSE(system->IsBusy());
	// The tap reaches the thing
	ASSERT_EQ(world->tapped.size(), 1u);
	EXPECT_EQ(world->tapped.front(), rock);
}

TEST_F(HandGrabSystemWithWorld, ThingsOutOfTheInfluenceOrHeldByAScriptAreLeft)
{
	const auto rock = world->AddRock({0.0f, 0.0f, 0.0f});
	world->underCursor = rock;
	world->influence = false;
	EXPECT_FALSE(Press());
	world->influence = true;
	world->registry.Assign<CannotBePickedUp>(rock);
	EXPECT_FALSE(Press());
	world->registry.Remove<CannotBePickedUp>(rock);
	// A rock wider than the hand can lift
	const auto boulder = world->AddRock({5.0f, 0.0f, 0.0f}, 4.0f);
	world->underCursor = boulder;
	EXPECT_FALSE(Press());
	// Out of the influence or held by a script nothing is tapped; a boulder the hand can't lift is tapped at once
	ASSERT_EQ(world->tapped.size(), 1u);
	EXPECT_EQ(world->tapped.front(), boulder);
}

TEST_F(HandGrabSystemWithWorld, AThingInFlightIsCaughtOnlyAfterTheWait)
{
	const auto rock = world->AddRock({0.0f, 5.0f, 0.0f});
	world->underCursor = rock;
	world->flying.insert(rock);
	EXPECT_TRUE(Press());
	Frame(100);
	EXPECT_FALSE(system->GetHeld().has_value());
	Frame(130);
	ASSERT_TRUE(system->GetHeld().has_value());
	EXPECT_EQ(*system->GetHeld(), rock);
}

TEST_F(HandGrabSystemWithWorld, AFlyingThingThatLandsDuringTheWaitIsTakenAtOnce)
{
	const auto rock = world->AddRock({0.0f, 5.0f, 0.0f});
	world->underCursor = rock;
	world->flying.insert(rock);
	EXPECT_TRUE(Press());
	Frame(50);
	world->flying.erase(rock);
	Frame(10);
	EXPECT_TRUE(system->GetHeld().has_value());
}

TEST_F(HandGrabSystemWithWorld, AHeavyTreeIsNeverPulledFree)
{
	const auto tree = world->AddTree({0.0f, 0.0f, 0.0f}, 1.0e6f);
	world->underCursor = tree;
	EXPECT_TRUE(Press());
	for (int i = 0; i < 100; ++i)
	{
		Frame(10, {0.0f, 50.0f, 0.0f});
	}
	EXPECT_FALSE(system->GetHeld().has_value());
	EXPECT_TRUE(world->holes.empty());
	// While it pulls the hand takes the pose a tree is held with
	const auto pose = system->GetPullPose();
	ASSERT_TRUE(pose.has_value());
	EXPECT_EQ(pose->hold, HoldType::Tree);
	// Let go before it came free, it stays where it is
	EXPECT_FALSE(Release().has_value());
	EXPECT_FALSE(system->GetPullPose().has_value());
	EXPECT_FALSE(world->registry.AllOf<InHand>(tree));
}

TEST_F(HandGrabSystemWithWorld, ALightTreeIsUprootedAndCreaks)
{
	const auto tree = world->AddTree({0.0f, 0.0f, 0.0f}, 10.0f);
	world->underCursor = tree;
	EXPECT_TRUE(Press());
	for (int i = 0; i < 30 && !system->GetHeld().has_value(); ++i)
	{
		Frame(10, {0.0f, 50.0f, 0.0f});
	}
	ASSERT_TRUE(system->GetHeld().has_value());
	EXPECT_EQ(world->holes, std::vector<entt::entity> {tree});
	EXPECT_EQ(world->uprooted, std::vector<entt::entity> {tree});
	// A tree pulled out of the ground creaks rather than making the pick-up sound
	ASSERT_EQ(world->samples.size(), 1u);
	EXPECT_EQ(world->samples.front(), 32u);
}

TEST_F(HandGrabSystemWithWorld, TheSpringTakesHoldTheFrameAfterThePressAndThrows)
{
	const auto rock = world->AddRock({0.0f, 0.0f, 0.0f});
	world->underCursor = rock;
	Press();
	for (int i = 0; i < 20 && !system->GetHeld().has_value(); ++i)
	{
		Frame(10);
	}
	Release();
	ASSERT_TRUE(system->GetHeld().has_value());
	// Held at rest, the hand rises for the rock without lagging
	const auto held = Frame(10, {0.0f, 0.0f, 0.0f});
	EXPECT_FLOAT_EQ(held.x, 0.0f);
	// Pressed again it makes ready to throw: this frame the hand is where it should be, then the spring drags it
	Press();
	const auto first = Frame(10, {5.0f, 0.0f, 0.0f});
	EXPECT_FLOAT_EQ(first.x, 5.0f);
	const auto dragged = Frame(10, {10.0f, 0.0f, 0.0f});
	EXPECT_GT(dragged.x, 5.0f);
	EXPECT_LT(dragged.x, 10.0f);
	Release();
	ASSERT_EQ(world->released.size(), 1u);
	EXPECT_EQ(world->released.front().first, rock);
	EXPECT_GT(world->released.front().second.x, 0.0f);
	EXPECT_FALSE(system->GetHeld().has_value());
}

TEST_F(HandGrabSystemWithWorld, WhatIsLetGoGetsItsTwistAFifthOfASecondLater)
{
	const auto rock = world->AddRock({0.0f, 0.0f, 0.0f});
	world->underCursor = rock;
	Press();
	for (int i = 0; i < 20 && !system->GetHeld().has_value(); ++i)
	{
		Frame(10);
	}
	Release();
	Frame(10);
	Press();
	Frame(10);
	Release();
	world->bodies.insert(rock);
	Frame(100, {}, {1.0f, 0.0f, 0.0f});
	EXPECT_TRUE(world->twists.empty());
	Frame(100, {}, {1.0f, 0.0f, 0.0f});
	ASSERT_EQ(world->twists.size(), 1u);
	EXPECT_EQ(world->twists.front().first, rock);
}

TEST_F(HandGrabSystemWithWorld, AVillagerOfTheHandsPlayerAlarmsThoseAboutIt)
{
	const auto villager = world->registry.Create();
	world->registry.Assign<Transform>(villager, glm::vec3(0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
	world->registry.Assign<Villager>(villager);
	world->sizes[villager] = {.radius = 0.5f, .height = 2.0f};
	world->underCursor = villager;
	Press();
	for (int i = 0; i < 20 && !system->GetHeld().has_value(); ++i)
	{
		Frame(10);
	}
	ASSERT_TRUE(system->GetHeld().has_value());
	EXPECT_EQ(world->villagersInHand, std::vector<entt::entity> {villager});
	const bool alarmed = std::ranges::any_of(world->reactions, [villager](const auto& reaction) {
		return reaction.type == Reaction::ReactToVillagerInHand && reaction.initiator == villager;
	});
	EXPECT_TRUE(alarmed);
}

TEST_F(HandGrabSystemWithWorld, WhatIsHeldIsHeatedEachTurnAndDroppedOnceGone)
{
	const auto rock = world->AddRock({0.0f, 0.0f, 0.0f});
	world->underCursor = rock;
	Press();
	for (int i = 0; i < 20 && !system->GetHeld().has_value(); ++i)
	{
		Frame(10);
	}
	// A rock taken stops being its town's artefact
	EXPECT_EQ(world->artefactsTaken, std::vector<entt::entity> {rock});
	system->ProcessTurn();
	EXPECT_EQ(world->heated, std::vector<entt::entity> {rock});
	world->registry.Destroy(rock);
	system->ProcessTurn();
	EXPECT_FALSE(system->IsBusy());
}

TEST_F(HandGrabSystemWithWorld, WhatIsHeldOutsideTheInfluenceIsNotHeated)
{
	const auto rock = world->AddRock({0.0f, 0.0f, 0.0f});
	world->underCursor = rock;
	Press();
	for (int i = 0; i < 20 && !system->GetHeld().has_value(); ++i)
	{
		Frame(10);
	}
	world->influence = false;
	system->ProcessTurn();
	EXPECT_TRUE(world->heated.empty());
}

TEST_F(HandGrabSystemWithWorld, AHandfulScoopedFromAPoisonedPileIsPoisoned)
{
	const auto pile = world->registry.Create();
	world->registry.Assign<Transform>(pile, glm::vec3(0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
	world->registry.Assign<Pot>(pile, Pot {.amount = 1000, .maxAmount = 2000, .type = PotInfo::FoodPile, .poisoned = true});
	world->sizes[pile] = {.radius = 2.0f, .height = 3.0f};
	world->underCursor = pile;
	EXPECT_TRUE(Press());
	const auto handful = system->GetHeld();
	ASSERT_TRUE(handful.has_value());
	EXPECT_TRUE(world->registry.Get<const Pot>(*handful).poisoned);
}

TEST_F(HandGrabSystemWithWorld, APileIsScoopedIntoAHandfulThatGrowsWhileTheButtonIsHeld)
{
	const auto pile = world->registry.Create();
	world->registry.Assign<Transform>(pile, glm::vec3(0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
	world->registry.Assign<Pot>(pile, Pot {.amount = 1000, .maxAmount = 2000, .type = PotInfo::FoodPile});
	world->sizes[pile] = {.radius = 2.0f, .height = 3.0f};
	world->underCursor = pile;
	EXPECT_TRUE(Press());
	// The first scoop is taken at once
	const auto handful = system->GetHeld();
	ASSERT_TRUE(handful.has_value());
	EXPECT_EQ(world->registry.Get<const Pot>(*handful).amount, 25u);
	EXPECT_EQ(world->registry.Get<const Pot>(pile).amount, 975u);
	EXPECT_EQ(world->streams, 1u);
	// The cursor is pinned while it scoops
	EXPECT_TRUE(world->cursorPinned);
	// Each game turn takes more, from 8 a turn
	system->ProcessTurn();
	EXPECT_EQ(world->registry.Get<const Pot>(*handful).amount, 25u + 8u);
	system->ProcessTurn();
	EXPECT_GT(world->registry.Get<const Pot>(*handful).amount, 33u);
	EXPECT_EQ(world->scoopSounds.size(), 2u);
	// The hand hovers over the pile, three above its height
	const auto hovering = Frame(10, {30.0f, 0.0f, 0.0f});
	EXPECT_FLOAT_EQ(hovering.x, 0.0f);
	EXPECT_FLOAT_EQ(hovering.y, 3.0f + 3.0f);
	// Let go, the scoop ends, its stream stops and the cursor is free
	Release();
	EXPECT_EQ(world->stopped, std::vector<uint32_t> {1u});
	EXPECT_FALSE(world->cursorPinned);
	system->ProcessTurn();
	EXPECT_EQ(world->scoopSounds.size(), 2u);
	EXPECT_TRUE(system->GetHeld().has_value());
}

TEST_F(HandGrabSystemWithWorld, AHandfulPressedOntoAStoreGoesIntoIt)
{
	const auto pile = world->registry.Create();
	world->registry.Assign<Transform>(pile, glm::vec3(0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
	world->registry.Assign<Pot>(pile, Pot {.amount = 100, .maxAmount = 2000, .type = PotInfo::FoodPile});
	world->underCursor = pile;
	Press();
	Release();
	const auto handful = *system->GetHeld();
	const auto store = world->registry.Create();
	world->stores.insert(store);
	world->underCursor = store;
	EXPECT_TRUE(Press());
	EXPECT_FALSE(system->IsBusy());
	EXPECT_EQ(world->stored[store], 25u);
	EXPECT_EQ(world->usedUp, std::vector<entt::entity> {handful});
	EXPECT_TRUE(world->released.empty());
}

TEST_F(HandGrabSystemWithWorld, ATreePressedOntoAStoreGoesIntoItWhole)
{
	const auto tree = world->AddTree({0.0f, 0.0f, 0.0f}, 10.0f);
	world->underCursor = tree;
	Press();
	for (int i = 0; i < 40 && !system->GetHeld().has_value(); ++i)
	{
		Frame(10);
	}
	ASSERT_TRUE(system->GetHeld().has_value());
	Release();
	const auto store = world->registry.Create();
	world->stores.insert(store);
	world->underCursor = store;
	EXPECT_TRUE(Press());
	ASSERT_EQ(world->takenWhole.size(), 1u);
	EXPECT_EQ(world->takenWhole.front().first, store);
	EXPECT_EQ(world->takenWhole.front().second, tree);
	EXPECT_FALSE(system->GetHeld().has_value());
}

TEST_F(HandGrabSystemWithWorld, APotLetGoOverTheLandCallsThePeopleAgain)
{
	const auto pile = world->registry.Create();
	world->registry.Assign<Transform>(pile, glm::vec3(0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
	world->registry.Assign<Pot>(pile, Pot {.amount = 100, .maxAmount = 2000, .type = PotInfo::FoodPile});
	world->underCursor = pile;
	Press();
	Release();
	const auto handful = *system->GetHeld();
	world->underCursor.reset();
	// Made ready and let go slowly, it is poured, having called the people to it first
	Press();
	Frame(10);
	Release();
	EXPECT_EQ(world->potReactionsSetUp, std::vector<entt::entity> {handful});
}

TEST_F(HandGrabSystemWithWorld, AThingLetGoBeforeItIsTakenIsStillTheLastLetGo)
{
	const auto rock = world->AddRock({0.0f, 0.0f, 0.0f});
	world->underCursor = rock;
	Press();
	// The first frame of the pull never pulls, so the rock is still being taken
	Frame(200);
	EXPECT_FALSE(system->GetHeld().has_value());
	Release();
	EXPECT_EQ(world->registry.Get<const HandGrab>(world->hand).released, rock);
}

TEST_F(HandGrabSystemWithWorld, ARipeFieldGivesHalfOfEachScoop)
{
	const auto field = world->registry.Create();
	world->registry.Assign<Transform>(field, glm::vec3(0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
	world->fields[field] = {.food = 1000, .ripe = true};
	world->underCursor = field;
	EXPECT_TRUE(Press());
	const auto handful = system->GetHeld();
	ASSERT_TRUE(handful.has_value());
	// The first scoop of 25 is halved, and so is each turn's 8
	EXPECT_EQ(world->registry.Get<const Pot>(*handful).amount, 12u);
	EXPECT_EQ(world->fields[field].food, 988u);
	system->ProcessTurn();
	EXPECT_EQ(world->registry.Get<const Pot>(*handful).amount, 16u);
}

TEST_F(HandGrabSystemWithWorld, AScoopsStreamFollowsTheHandAndAPourEndsAfterThreeQuartersOfASecond)
{
	const auto pile = world->registry.Create();
	world->registry.Assign<Transform>(pile, glm::vec3(0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
	world->registry.Assign<Pot>(pile, Pot {.amount = 1000, .maxAmount = 2000, .type = PotInfo::FoodPile});
	world->underCursor = pile;
	EXPECT_TRUE(Press());
	Frame(10);
	EXPECT_FALSE(world->streamPoints.empty());
	Release();
	world->underCursor.reset();
	// Made ready and let go slowly, the handful pours, and the pour stops after 0.75 s
	Press();
	Frame(10);
	Release();
	ASSERT_EQ(world->poured.size(), 1u);
	const auto stoppedBefore = world->stopped.size();
	Frame(700);
	EXPECT_EQ(world->stopped.size(), stoppedBefore);
	Frame(100);
	EXPECT_EQ(world->stopped.size(), stoppedBefore + 1);
}
