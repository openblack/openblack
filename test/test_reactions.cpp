/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstring>

#include <algorithm>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

#include "ECS/Components/Creature.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/LivingReaction.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/MiracleImpression.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/ReactionRules.h"
#include "Magic/ShieldRules.h"
#include "Magic/VillagerReactionRules.h"

#define LOCATOR_IMPLEMENTATIONS
#include "ECS/MapProduction.h"
#include "ECS/Systems/Implementations/LivingActionSystem.h"
#include "ECS/Systems/Implementations/ReactionSystem.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;

namespace
{
/// Every kind of living thing reacts to every kind of reaction
void ReactToEverything(GLivingInfo& info)
{
	std::memset(&info.isReacting, 1, sizeof(info.isReacting));
}

/// Where the miracles are cast, well inside the land's map
const glm::vec3 k_Here(500.0f, 0.0f, 500.0f);
} // namespace

/// Villagers of a town and a creature about a miracle, with made-up reaction tables
class Reactions: public ::testing::Test
{
protected:
	void SetUp() override
	{
		for (const auto* name : {"ai", "game", "scripting"})
		{
			if (spdlog::get(name) == nullptr)
			{
				spdlog::create<spdlog::sinks::null_sink_st>(name);
			}
		}
		Locator::entitiesRegistry::emplace<Registry>();
		Locator::entitiesMap::emplace<MapProduction>();
		Locator::livingActionSystem::emplace<ecs::systems::LivingActionSystem>();
		_info = std::make_unique<InfoConstants>();
		for (const auto type : {Reaction::LookAtNiceSpell, Reaction::FleeFromSpell, Reaction::ReactToMagicShield})
		{
			auto& reaction = _info->reaction.at(static_cast<size_t>(type));
			reaction.priority = type == Reaction::FleeFromSpell ? 150 : 120;
			reaction.whetherItImpresses = 1;
			reaction.maxReactionDistance = 60.0f;
			reaction.howImportantIsDistance = 0.25f;
			reaction.numGameTurnsForNormalThingsToReact = 80;
			reaction.numGameTurnsForNormalThingsBeforeReactingAgain = 15;
			reaction.numGameTurnsForCreatureToReact = 80;
			reaction.numGameTurnsForCreatureBeforeReactingAgain = 0;
			reaction.defaultReactionImpressiveMultiplier = 1.0f;
			reaction.additionToTownBoredomMultipliers = 0.25f;
			reaction.minDistanceToRunAwayFromObject = 50.0f;
			reaction.maxDistanceToRunAwayFromObject = 100.0f;
		}
		for (auto& villager : _info->villager)
		{
			ReactToEverything(villager);
		}
		for (auto& creature : _info->creature)
		{
			ReactToEverything(creature);
		}
		// Idling villagers may react; reacting is a state of its own
		auto& idle = _info->villagerStateTable.at(static_cast<size_t>(VillagerStates::DecideWhatToDo));
		idle.availableForReaction = 1;
		idle.isFinalState = 1;
		idle.resumeState = static_cast<int>(VillagerStates::DecideWhatToDo);
		for (const auto state : {VillagerStates::FleeingFromObjectReaction, VillagerStates::LookingAtObjectReaction,
		                         VillagerStates::FleeingAndLookingAtObjectReaction})
		{
			_info->villagerStateTable.at(static_cast<size_t>(state)).availableForReaction = 1;
			_info->villagerStateTable.at(static_cast<size_t>(state)).isReactionState = 1;
		}
		_info->town.populationForUnmodifiedBelief = 30.0f;
		_info->belief.defaultBoredomOfMe = -0.02f;
		Locator::infoConstants::emplace(*_info);

		auto& registry = Locator::entitiesRegistry::value();
		_town = registry.Create();
		for (const float x : {0.0f, 5.0f, 200.0f})
		{
			const auto villager = registry.Create();
			registry.Assign<Transform>(villager, k_Here + glm::vec3(x, 0.0f, 0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
			registry.Assign<Mobile>(villager);
			auto& component = registry.Assign<Villager>(villager);
			component.town = _town;
			component.health = 100;
			registry.Assign<WallHug>(villager, glm::vec2(0.0f), glm::vec2(0.0f), 0.0f, 1.0f);
			registry.Assign<LivingAction>(villager, VillagerStates::DecideWhatToDo, static_cast<uint16_t>(0));
			_villagers.push_back(villager);
		}
		_creature = registry.Create();
		registry.Assign<Transform>(_creature, k_Here, glm::mat3(1.0f), glm::vec3(1.0f));
		registry.Assign<Creature>(_creature).owner = PlayerNames::PLAYER_ONE;
		Locator::entitiesMap::value().Sync();
	}
	void TearDown() override
	{
		Locator::infoConstants::reset();
		Locator::livingActionSystem::reset();
		Locator::entitiesMap::reset();
		Locator::entitiesRegistry::reset();
	}

	[[nodiscard]] static ecs::systems::ReactionSystemInterface::Source Miracle(Reaction type = Reaction::LookAtNiceSpell)
	{
		return {.initiator = static_cast<entt::entity>(1000),
		        .type = type,
		        .player = PlayerNames::PLAYER_ONE,
		        .position = k_Here,
		        .impressiveValue = 2.0f,
		        .power = 1.0f};
	}

	[[nodiscard]] static VillagerStates StateOf(entt::entity villager)
	{
		return static_cast<VillagerStates>(Locator::entitiesRegistry::value().Get<const LivingAction>(villager).states.at(0));
	}

	std::unique_ptr<InfoConstants> _info;
	entt::entity _town {entt::null};
	std::vector<entt::entity> _villagers;
	entt::entity _creature {entt::null};
	ecs::systems::ReactionSystem _reactions;
};

TEST(ReactionRules, MoreUrgentCloserAndToFleeingMore)
{
	using namespace openblack::magic;
	// The kind's priority, the caster taking no notice of its own nice miracle
	EXPECT_EQ(KindPriority(Reaction::LookAtNiceSpell, 120, 0.0f, false), 120u);
	EXPECT_EQ(KindPriority(Reaction::LookAtNiceSpell, 120, 0.0f, true), 0u);
	// Fleeing gains up to a hundred within the urgent reach, by whole steps
	EXPECT_EQ(KindPriority(Reaction::FleeFromSpell, 150, k_FleeUrgencyUnits / 2.0f, false), 200u);
	EXPECT_EQ(KindPriority(Reaction::FleeFromSpell, 150, k_FleeUrgencyUnits, false), 150u);
	EXPECT_EQ(KindPriority(Reaction::FleeFromSpell, 150, 0.0f, true), 0u);
	EXPECT_FLOAT_EQ(FastMapDistance({0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 4.0f}), 12.0f * k_MapUnitsPerMetre);
	// The distance then counts as for any reaction: close by up to an eighth more
	const villager_reaction::Distance reach {.maxDistance = 60.0f, .importance = 0.25f};
	EXPECT_EQ(villager_reaction::Priority(120, true, reach, 0.0f), 135u);
	EXPECT_EQ(villager_reaction::Priority(120, true, reach, 60.0f), 120u);
	EXPECT_EQ(villager_reaction::Priority(120, true, reach, 61.0f), 0u);
}

TEST(ReactionRules, ADifferentMoreUrgentKindTakesOverOnlyAfterAWhile)
{
	using namespace openblack::magic;
	EXPECT_FALSE(ChangesReaction(Reaction::LookAtNiceSpell, 120, 200, 9));
	EXPECT_TRUE(ChangesReaction(Reaction::LookAtNiceSpell, 120, 200, 10));
	EXPECT_FALSE(ChangesReaction(Reaction::LookAtNiceSpell, 200, 200, 20));
	// Reacting to the hand picking something up, a second is enough
	EXPECT_FALSE(ChangesReaction(Reaction::ReactToHandPickUp, 120, 200, 0));
	EXPECT_TRUE(ChangesReaction(Reaction::ReactToHandPickUp, 120, 200, 1));
	EXPECT_FALSE(ChangesReaction(Reaction::LookAtNiceSpell, 120, 200, 1));
}

TEST(ReactionRules, VillagersFleeAwayOrAcrossTheWay)
{
	using namespace openblack::magic;
	const auto still = FleePointFromStill({10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	EXPECT_FLOAT_EQ(still.x, 20.0f);
	EXPECT_FLOAT_EQ(still.z, 0.0f);
	// Something moving north past a villager to its east: the villager goes on east
	const auto moving = FleePointFromMoving({5.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 4.0f, 4.0f);
	EXPECT_FLOAT_EQ(moving.x, 5.0f + 10.0f);
	EXPECT_FLOAT_EQ(moving.z, 0.0f);
	EXPECT_TRUE(ComingTowards({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}));
	EXPECT_FALSE(ComingTowards({10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}));
}

TEST_F(Reactions, VillagersInReachTakeUpANiceMiracleAndBelieveOnce)
{
	_reactions.Create(Miracle());
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* impression = registry.TryGet<const TownImpression>(_town);
	ASSERT_NE(impression, nullptr);
	// The two villagers in reach, the first right by it, the other a little away; the third is too far. Each impression
	// is shared by the town's three people against the thirty its belief is unchanged for.
	// It waits as the town's pending belief until the town's turn
	const float belief = impression->belief.pending.at(static_cast<size_t>(PlayerNames::PLAYER_ONE));
	EXPECT_FLOAT_EQ(impression->belief.belief.at(static_cast<size_t>(PlayerNames::PLAYER_ONE)), 0.0f);
	const float share = (30.0f + 0.001f) / (3.0f + 0.001f);
	EXPECT_GT(belief, 2.0f * share);
	EXPECT_LT(belief, 4.0f * share);
	// Each villager's impression tires the town by the belief table's boredom times its share of the town
	EXPECT_FLOAT_EQ(impression->boredom.at(Reaction::LookAtNiceSpell), 1.0f - (2.0f * 0.02f * share));
	EXPECT_EQ(StateOf(_villagers[0]), VillagerStates::LookingAtObjectReaction);
	EXPECT_EQ(StateOf(_villagers[2]), VillagerStates::DecideWhatToDo);
	// Spread again, those reacting already aren't impressed again
	_reactions.ProcessTurn();
	const auto& town = registry.Get<const TownImpression>(_town).belief;
	// Believed at the town's turn, no more than its cap
	EXPECT_FLOAT_EQ(town.belief.at(static_cast<size_t>(PlayerNames::PLAYER_ONE)),
	                std::min(belief, openblack::magic::town_belief::k_DefaultCap));
	EXPECT_FLOAT_EQ(town.pending.at(static_cast<size_t>(PlayerNames::PLAYER_ONE)), 0.0f);
}

TEST_F(Reactions, VillagersStopAfterTheirTimeAndGoBack)
{
	_reactions.Create(Miracle());
	for (int turn = 0; turn < 100; ++turn)
	{
		_reactions.ProcessTurn();
	}
	EXPECT_EQ(StateOf(_villagers[0]), VillagerStates::DecideWhatToDo);
	EXPECT_EQ(Locator::entitiesRegistry::value().Get<const LivingReaction>(_villagers[0]).reaction, 0u);
}

TEST_F(Reactions, VillagersFleeAFrighteningMiracle)
{
	// A turn in, as a turn of 0 is remembered as not lately
	_reactions.ProcessTurn();
	_reactions.Create(Miracle(Reaction::FleeFromSpell));
	EXPECT_EQ(StateOf(_villagers[0]), VillagerStates::FleeingFromObjectReaction);
	// Remembered with the kinds the shields' reactions see
	const auto& memory = Locator::entitiesRegistry::value().Get<const VillagerReactionMemory>(_villagers[0]).memory;
	EXPECT_NE(memory.LastReacted(static_cast<uint32_t>(Reaction::FleeFromSpell)), 0u);
	// Its going takes everyone off it
	_reactions.RemoveFrom(static_cast<entt::entity>(1000));
	EXPECT_EQ(StateOf(_villagers[0]), VillagerStates::DecideWhatToDo);
}

TEST_F(Reactions, TheCreatureIsImpressedByItsOwnPlayersMiracle)
{
	_reactions.Create(Miracle());
	const auto* impression = Locator::entitiesRegistry::value().TryGet<const CreatureImpression>(_creature);
	ASSERT_NE(impression, nullptr);
	EXPECT_FLOAT_EQ(impression->byOwnPlayer, 2.0f * 4.0f);
	EXPECT_FLOAT_EQ(impression->byOtherCreatures, 0.0f);
}

TEST_F(Reactions, AnotherPlayersMiracleImpressesACreatureNotAtAllWhoeverCastIt)
{
	// Another player's creature cast it: a miracle isn't a creature, so it impresses nothing
	auto& registry = Locator::entitiesRegistry::value();
	const auto other = registry.Create();
	registry.Assign<Creature>(other).owner = PlayerNames::PLAYER_TWO;
	auto source = Miracle(Reaction::FleeFromSpell);
	source.player = PlayerNames::PLAYER_TWO;
	source.casterCreature = other;
	_reactions.Create(source);
	const auto* impression = registry.TryGet<const CreatureImpression>(_creature);
	ASSERT_NE(impression, nullptr);
	EXPECT_FLOAT_EQ(impression->byOwnPlayer, 0.0f);
	EXPECT_FLOAT_EQ(impression->byOtherCreatures, 0.0f);
}

TEST_F(Reactions, AnotherCreatureOfItsPlayerImpressesItAsItsPlayer)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto other = registry.Create();
	registry.Assign<Creature>(other).owner = PlayerNames::PLAYER_ONE;
	auto source = Miracle(Reaction::FleeFromSpell);
	source.casterCreature = other;
	_reactions.Create(source);
	const auto* impression = registry.TryGet<const CreatureImpression>(_creature);
	ASSERT_NE(impression, nullptr);
	EXPECT_GT(impression->byOwnPlayer, 0.0f);
	EXPECT_FLOAT_EQ(impression->byOtherCreatures, 0.0f);
}

TEST_F(Reactions, ACreatureIsNotImpressedByItsOwnMiracle)
{
	auto source = Miracle();
	source.casterCreature = _creature;
	_reactions.Create(source);
	_reactions.ProcessTurn();
	EXPECT_EQ(Locator::entitiesRegistry::value().TryGet<const CreatureImpression>(_creature), nullptr);
}

TEST_F(Reactions, AReactionGoesWithWhatMadeItAndIsFoundWhereItReaches)
{
	const auto id = _reactions.Create(Miracle());
	EXPECT_TRUE(_reactions.HasReaction(static_cast<entt::entity>(1000)));
	EXPECT_EQ(_reactions.ReactionsAt(k_Here + glm::vec3(30.0f, 0.0f, 0.0f)).size(), 1u);
	EXPECT_TRUE(_reactions.ReactionsAt(k_Here + glm::vec3(100.0f, 0.0f, 0.0f)).empty());
	_reactions.Move(id, k_Here + glm::vec3(100.0f, 0.0f, 0.0f), glm::vec3(0.0f), 1.0f);
	EXPECT_EQ(_reactions.ReactionsAt(k_Here + glm::vec3(100.0f, 0.0f, 0.0f)).size(), 1u);
	_reactions.RemoveFrom(static_cast<entt::entity>(1000));
	EXPECT_FALSE(_reactions.HasReaction(static_cast<entt::entity>(1000)));
	EXPECT_TRUE(_reactions.GetReactions().empty());
}

TEST_F(Reactions, TheLandsBalanceScalesHowImpressiveMiraclesAre)
{
	_reactions.SetLandBalance(2.0f);
	_reactions.Create(Miracle());
	EXPECT_FLOAT_EQ(Locator::entitiesRegistry::value().Get<const CreatureImpression>(_creature).byOwnPlayer,
	                2.0f * 2.0f * 4.0f);
	_reactions.Reset();
	EXPECT_FLOAT_EQ(_reactions.GetLandBalance(), 1.0f);
}

TEST_F(Reactions, AShieldIsLeftToTheShieldsForVillagersButACreatureTakesItUp)
{
	_reactions.Create(Miracle(Reaction::ReactToMagicShield));
	const auto& registry = Locator::entitiesRegistry::value();
	EXPECT_EQ(StateOf(_villagers[0]), VillagerStates::DecideWhatToDo);
	EXPECT_EQ(registry.TryGet<const LivingReaction>(_villagers[0]), nullptr);
	EXPECT_EQ(registry.Get<const LivingReaction>(_creature).type, Reaction::ReactToMagicShield);
}

TEST_F(Reactions, ACreatureIgnoresAnotherPlayersShield)
{
	auto source = Miracle(Reaction::ReactToMagicShield);
	source.player = PlayerNames::PLAYER_TWO;
	_reactions.Create(source);
	const auto* state = Locator::entitiesRegistry::value().TryGet<const LivingReaction>(_creature);
	EXPECT_TRUE(state == nullptr || state->reaction == 0);
}

TEST_F(Reactions, VillagersBusyWithAShieldAreLeftToIt)
{
	Locator::entitiesRegistry::value().Assign<VillagerShieldReaction>(_villagers[0]);
	_reactions.Create(Miracle(Reaction::FleeFromSpell));
	EXPECT_EQ(StateOf(_villagers[0]), VillagerStates::DecideWhatToDo);
	EXPECT_EQ(StateOf(_villagers[1]), VillagerStates::FleeingFromObjectReaction);
}

TEST(ReactionRules, MostReactionsTimeOutButNotFleeingNorImpressedByMiracles)
{
	using namespace openblack::magic;
	EXPECT_TRUE(TimesOut(Reaction::LookAtNiceSpell));
	EXPECT_TRUE(TimesOut(Reaction::ReactToObjectCrushed));
	EXPECT_FALSE(TimesOut(Reaction::FleeFromSpell));
	EXPECT_FALSE(TimesOut(Reaction::ReactToImpressiveSpell));
	// Its time starts with its stamp: none, never
	EXPECT_FALSE(TimedOut(Reaction::LookAtNiceSpell, std::nullopt, 500, 80));
	EXPECT_FALSE(TimedOut(Reaction::LookAtNiceSpell, 10, 90, 80));
	EXPECT_TRUE(TimedOut(Reaction::LookAtNiceSpell, 10, 91, 80));
	EXPECT_FLOAT_EQ(StartingReach(true, 60.0f), 1.0f);
	EXPECT_FLOAT_EQ(StartingReach(false, 60.0f), 60.0f);
}

TEST(ShieldRules, WhatIsSurelyWithinAShield)
{
	using namespace openblack::magic::shield;
	EXPECT_TRUE(WithinSpiritualShield({0.0f, 0.0f, 0.0f}, 10.0f, {6.0f, 0.0f, 7.9f}));
	EXPECT_FALSE(WithinSpiritualShield({0.0f, 0.0f, 0.0f}, 10.0f, {6.0f, 0.0f, 8.0f}));
	const DomeVolume dome {.radius = 20.0f, .height = 10.0f};
	EXPECT_TRUE(WithinPhysicalShield(dome, 10.0f, 4.9f));
	EXPECT_FALSE(WithinPhysicalShield(dome, 10.0f, 5.0f));
	EXPECT_FALSE(WithinPhysicalShield(dome, 20.0f, 0.0f));
}
