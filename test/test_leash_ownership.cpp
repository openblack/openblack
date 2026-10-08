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
#include <array>

#include <SDL_events.h>
#include <gtest/gtest.h>

#include "Creature/CreatureHandRules.h"
#include "Creature/LeashKeys.h"
#include "Creature/LeashOwnership.h"
#include "Input/GameActionMap.h"

using namespace openblack;
using namespace openblack::creature_leash;
using input::BindableActionMap;
using Kind = KeyCommand::Kind;

namespace
{
/// The player's own creature, leashable and knowing every leash
Candidate Own()
{
	return {.owner = PlayerNames::PLAYER_ONE, .leashable = true, .knowsLearningLeash = true, .knowsType = true};
}

std::bitset<k_Types.size()> Knowing(std::initializer_list<LeashType> types)
{
	std::bitset<k_Types.size()> known;
	for (const auto type : types)
	{
		known.set(*IndexOf(type));
	}
	return known;
}
} // namespace

TEST(LeashOwnership, ThePlayerLeadsTheirLeashableCreature)
{
	EXPECT_EQ(WhyNot(PlayerNames::PLAYER_ONE, Own()), Refusal::None);
	auto held = Own();
	held.heldBy = PlayerNames::PLAYER_ONE;
	EXPECT_EQ(WhyNot(PlayerNames::PLAYER_ONE, held), Refusal::None);
}

TEST(LeashOwnership, AnotherPlayersCreatureIsRefused)
{
	auto theirs = Own();
	theirs.owner = PlayerNames::PLAYER_TWO;
	EXPECT_EQ(WhyNot(PlayerNames::PLAYER_ONE, theirs), Refusal::OwnedByAnother);
	EXPECT_EQ(WhyNot(PlayerNames::PLAYER_TWO, theirs), Refusal::None);
	auto nobodys = Own();
	nobodys.owner = PlayerNames::NEUTRAL;
	EXPECT_EQ(WhyNot(PlayerNames::PLAYER_ONE, nobodys), Refusal::OwnedByAnother);
	EXPECT_EQ(WhyNot(PlayerNames::NEUTRAL, nobodys), Refusal::NoPlayer);
}

TEST(LeashOwnership, OnlyTheLeashableOneOfThePlayers)
{
	auto extra = Own();
	extra.leashable = false;
	EXPECT_EQ(WhyNot(PlayerNames::PLAYER_ONE, extra), Refusal::NotLeashable);
}

TEST(LeashOwnership, ItMustKnowTheLeashes)
{
	auto untaught = Own();
	untaught.knowsLearningLeash = false;
	EXPECT_EQ(WhyNot(PlayerNames::PLAYER_ONE, untaught), Refusal::DoesNotKnowLearningLeash);
	auto other = Own();
	other.knowsType = false;
	EXPECT_EQ(WhyNot(PlayerNames::PLAYER_ONE, other), Refusal::DoesNotKnowThatLeash);
}

TEST(LeashOwnership, AnotherHolderIsRefused)
{
	auto held = Own();
	held.heldBy = PlayerNames::PLAYER_TWO;
	EXPECT_EQ(WhyNot(PlayerNames::PLAYER_ONE, held), Refusal::HeldByAnother);
}

TEST(LeashOwnership, EveryRefusalIsDescribed)
{
	for (const auto refusal :
	     {Refusal::None, Refusal::NotACreature, Refusal::NoPlayer, Refusal::OwnedByAnother, Refusal::NotLeashable,
	      Refusal::DoesNotKnowLearningLeash, Refusal::DoesNotKnowThatLeash, Refusal::HeldByAnother})
	{
		EXPECT_STRNE(Describe(refusal), "unknown");
	}
}

TEST(LeashOwnership, MakingOneLeashableDisplacesTheOwnersOther)
{
	const std::array<Claim, 4> creatures {{
	    {.creature = 1, .owner = PlayerNames::PLAYER_ONE, .leashable = true},
	    {.creature = 2, .owner = PlayerNames::PLAYER_ONE, .leashable = false},
	    {.creature = 3, .owner = PlayerNames::PLAYER_TWO, .leashable = true},
	    {.creature = 4, .owner = PlayerNames::PLAYER_ONE, .leashable = false},
	}};
	EXPECT_EQ(Displaced(creatures, 2, PlayerNames::PLAYER_ONE), std::vector<uint32_t> {1});
	// Choosing the one already leashable displaces nobody, and other players' creatures are left alone
	EXPECT_TRUE(Displaced(creatures, 1, PlayerNames::PLAYER_ONE).empty());
	EXPECT_TRUE(Displaced(creatures, 4, PlayerNames::PLAYER_THREE).empty());
}

TEST(LeashOwnership, EachPlayerHasOneLeashableCreature)
{
	const std::array<Claim, 3> creatures {{
	    {.creature = 7, .owner = PlayerNames::PLAYER_ONE, .leashable = false},
	    {.creature = 8, .owner = PlayerNames::PLAYER_ONE, .leashable = true},
	    {.creature = 9, .owner = PlayerNames::PLAYER_TWO, .leashable = true},
	}};
	EXPECT_EQ(LeashableOf(creatures, PlayerNames::PLAYER_ONE), 8u);
	EXPECT_EQ(LeashableOf(creatures, PlayerNames::PLAYER_TWO), 9u);
	EXPECT_FALSE(LeashableOf(creatures, PlayerNames::PLAYER_THREE).has_value());
}

TEST(LeashOwnership, APlayersFirstCreatureClaimsTheLeash)
{
	EXPECT_TRUE(ClaimsOnArrival({}, PlayerNames::PLAYER_ONE));
	const std::array<Claim, 1> one {{{.creature = 1, .owner = PlayerNames::PLAYER_ONE, .leashable = true}}};
	EXPECT_FALSE(ClaimsOnArrival(one, PlayerNames::PLAYER_ONE));
	EXPECT_TRUE(ClaimsOnArrival(one, PlayerNames::PLAYER_TWO));
	// A creature that belongs to nobody is never leashable
	EXPECT_FALSE(ClaimsOnArrival({}, PlayerNames::NEUTRAL));
	// Extra creatures that aren't leashable don't count
	const std::array<Claim, 1> extra {{{.creature = 1, .owner = PlayerNames::PLAYER_ONE, .leashable = false}}};
	EXPECT_TRUE(ClaimsOnArrival(extra, PlayerNames::PLAYER_ONE));
}

TEST(LeashKeys, ControlsMapToTheShortcuts)
{
	EXPECT_EQ(KeyFor(BindableActionMap::LEASH_UNLEASH_CREATURE), LeashKey::Leash);
	EXPECT_EQ(KeyFor(BindableActionMap::PREVIOUS_LEASH), LeashKey::PreviousLeash);
	EXPECT_EQ(KeyFor(BindableActionMap::NEXT_LEASH), LeashKey::NextLeash);
	EXPECT_FALSE(KeyFor(BindableActionMap::ACTION).has_value());
	EXPECT_FALSE(KeyFor(BindableActionMap::QUICK_LOAD).has_value());
}

TEST(LeashKeys, TheGameBindsLVAndB)
{
	// Whether pressing the key, with the modifier held, sets off the action
	const auto sets = [](SDL_Scancode key, uint16_t mod, BindableActionMap action) {
		input::GameActionMap actions;
		SDL_Event event {};
		event.type = SDL_KEYDOWN;
		event.key.keysym.scancode = key;
		event.key.keysym.mod = mod;
		actions.ProcessEvent(event);
		return actions.Get(action);
	};
	EXPECT_TRUE(sets(SDL_SCANCODE_L, KMOD_NONE, BindableActionMap::LEASH_UNLEASH_CREATURE));
	EXPECT_TRUE(sets(SDL_SCANCODE_V, KMOD_NONE, BindableActionMap::PREVIOUS_LEASH));
	EXPECT_TRUE(sets(SDL_SCANCODE_B, KMOD_NONE, BindableActionMap::NEXT_LEASH));
	// Ctrl+L is the quick load, not the leash
	EXPECT_FALSE(sets(SDL_SCANCODE_L, KMOD_LCTRL, BindableActionMap::LEASH_UNLEASH_CREATURE));
	EXPECT_TRUE(sets(SDL_SCANCODE_L, KMOD_LCTRL, BindableActionMap::QUICK_LOAD));
}

TEST(LeashKeys, TheLeashKeyPutsOnUntiesAndTakesOff)
{
	const auto all = Knowing({LeashType::Evil, LeashType::Rope, LeashType::Good});
	EXPECT_EQ(CommandFor(LeashKey::Leash, {.known = all, .selected = LeashType::Good}),
	          (KeyCommand {.kind = Kind::PutOn, .type = LeashType::Good}));
	EXPECT_EQ(CommandFor(LeashKey::Leash, {.worn = true, .tied = true, .known = all}),
	          (KeyCommand {.kind = Kind::UntieToHand}));
	EXPECT_EQ(CommandFor(LeashKey::Leash, {.worn = true, .known = all}), (KeyCommand {.kind = Kind::TakeOff}));
}

TEST(LeashKeys, TheLeashKeyNeedsTheLearningLeash)
{
	EXPECT_EQ(CommandFor(LeashKey::Leash, {.known = Knowing({LeashType::Good})}), KeyCommand {});
	// A picked leash it doesn't know falls back to the learning leash
	EXPECT_EQ(CommandFor(LeashKey::Leash, {.known = Knowing({LeashType::Rope}), .selected = LeashType::Evil}),
	          (KeyCommand {.kind = Kind::PutOn, .type = LeashType::Rope}));
}

TEST(LeashKeys, VStepsUpAndBDownThroughTheLeashNumbers)
{
	const auto all = Knowing({LeashType::Evil, LeashType::Rope, LeashType::Good});
	const auto change = [](LeashType type) { return KeyCommand {.kind = Kind::ChangeType, .type = type}; };
	// V: aggression 1, learning 2, compassion 3, and round again
	EXPECT_EQ(CommandFor(LeashKey::PreviousLeash, {.known = all, .selected = LeashType::Evil}), change(LeashType::Rope));
	EXPECT_EQ(CommandFor(LeashKey::PreviousLeash, {.known = all, .selected = LeashType::Rope}), change(LeashType::Good));
	EXPECT_EQ(CommandFor(LeashKey::PreviousLeash, {.known = all, .selected = LeashType::Good}), change(LeashType::Evil));
	// B: 1, 3, 2, and round again
	EXPECT_EQ(CommandFor(LeashKey::NextLeash, {.known = all, .selected = LeashType::Evil}), change(LeashType::Good));
	EXPECT_EQ(CommandFor(LeashKey::NextLeash, {.known = all, .selected = LeashType::Good}), change(LeashType::Rope));
	EXPECT_EQ(CommandFor(LeashKey::NextLeash, {.known = all, .selected = LeashType::Rope}), change(LeashType::Evil));
}

TEST(LeashKeys, AStepOntoAnUnknownLeashDoesNothing)
{
	// Knowing aggression and learning, V from learning would reach compassion, which it doesn't know, so it stays
	const auto two = Knowing({LeashType::Evil, LeashType::Rope});
	EXPECT_FALSE(StepType(LeashType::Rope, two, true).has_value());
	EXPECT_EQ(StepType(LeashType::Rope, two, false), LeashType::Evil);
	EXPECT_EQ(CommandFor(LeashKey::PreviousLeash, {.known = two, .selected = LeashType::Rope}), KeyCommand {});
	EXPECT_EQ(CommandFor(LeashKey::NextLeash, {.known = Knowing({LeashType::Rope}), .selected = LeashType::Rope}),
	          KeyCommand {});
}

TEST(LeashKeys, TheLeashKeyTakesTheLeashOffHoweverManyItKnows)
{
	const auto all = Knowing({LeashType::Evil, LeashType::Rope, LeashType::Good});
	EXPECT_EQ(CommandFor(LeashKey::Leash, {.worn = true, .known = all, .selected = LeashType::Good}),
	          (KeyCommand {.kind = Kind::TakeOff}));
}

TEST(CreatureHand, TheHandHoldsAnyPlayersCreature)
{
	using creature_hand::Holdable;
	using creature_hand::MayHold;
	EXPECT_TRUE(MayHold({.owner = PlayerNames::PLAYER_ONE, .species = CreatureType::Tiger}));
	// Khazar's, Lethys's or any other god's
	EXPECT_TRUE(MayHold({.owner = PlayerNames::PLAYER_THREE, .species = CreatureType::Tortoise}));
	EXPECT_TRUE(MayHold({.owner = PlayerNames::PLAYER_FOUR, .species = CreatureType::Wolf}));
	// Not nobody's, nor an ogre, nor one asleep or frozen
	EXPECT_FALSE(MayHold({.owner = PlayerNames::NEUTRAL, .species = CreatureType::Tiger}));
	EXPECT_FALSE(MayHold({.owner = PlayerNames::PLAYER_ONE, .species = CreatureType::Ogre}));
	EXPECT_FALSE(MayHold({.owner = PlayerNames::PLAYER_ONE, .species = CreatureType::Tiger, .asleep = true}));
	EXPECT_FALSE(MayHold({.owner = PlayerNames::PLAYER_ONE, .species = CreatureType::Tiger, .frozen = true}));
}

TEST(CreatureHand, TheInteractTipIsForTheOwnCreatureOnly)
{
	using creature_hand::ShowsInteractTip;
	EXPECT_TRUE(ShowsInteractTip(PlayerNames::PLAYER_ONE, PlayerNames::PLAYER_ONE, false));
	EXPECT_FALSE(ShowsInteractTip(PlayerNames::PLAYER_ONE, PlayerNames::PLAYER_THREE, false));
	EXPECT_FALSE(ShowsInteractTip(PlayerNames::PLAYER_ONE, PlayerNames::PLAYER_ONE, true));
}

TEST(CreatureHand, AQuickPressIsAClickAndALongOneAHold)
{
	using creature_hand::IsClick;
	EXPECT_TRUE(IsClick(150.0f, false));
	EXPECT_FALSE(IsClick(creature_hand::k_ClickMaxMs, false));
	// Stroking or slapping makes it a hold however quick
	EXPECT_FALSE(IsClick(150.0f, true));
	// A click can never last long enough to stroke
	EXPECT_LE(creature_hand::k_ClickMaxMs, creature_feedback::k_StrokeHoldMs);
}
