/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>
#include <cstdint>

#include <array>
#include <deque>
#include <vector>

#include <gtest/gtest.h>

#include "Creature/CreatureFight.h"
#include "Creature/CreatureFightHud.h"

using namespace openblack;
using namespace openblack::creature_fight;

namespace
{
/// Random numbers given in turn, then 0
Random Sequence(std::deque<uint32_t> values)
{
	return [values = std::move(values)](uint32_t range) mutable {
		if (values.empty())
		{
			return 0u;
		}
		const auto value = values.front();
		values.pop_front();
		return range == 0 ? 0u : value % range;
	};
}

Random Always(uint32_t value)
{
	return [value](uint32_t range) { return range == 0 ? 0u : std::min(value, range - 1); };
}
} // namespace

TEST(CreatureFight, ArenaIsSizedByTheBiggerCreatureAndCapped)
{
	EXPECT_FLOAT_EQ(ArenaRadius(1.0f, 0.5f), 39.0f);
	EXPECT_FLOAT_EQ(ArenaRadius(0.5f, 1.2f), 39.0f * 1.2f);
	EXPECT_FLOAT_EQ(ArenaRadius(3.0f, 1.0f), 60.0f);
	const auto arena = MakeArena({0.0f, 0.0f}, {40.0f, 20.0f}, 1.0f, 1.0f);
	EXPECT_EQ(arena.centre, glm::vec2(20.0f, 10.0f));
	EXPECT_EQ(ArenaSpot(arena, 1.0f, true), glm::vec2(35.0f, 10.0f));
	EXPECT_EQ(ArenaSpot(arena, 2.0f, false), glm::vec2(-10.0f, 10.0f));
	EXPECT_FLOAT_EQ(ArrivalDistance(1.0f), 22.5f);
}

TEST(CreatureFight, FightersNeverPassThroughEachOther)
{
	EXPECT_FLOAT_EQ(ClosestApart(6.0f, 5.0f), 8.0f);
	// Moving in too close it stops on the circle; moving away or staying out it goes on
	const auto stopped = KeepApart({20.0f, 0.0f}, {2.0f, 0.0f}, {0.0f, 0.0f}, 8.0f);
	EXPECT_NEAR(stopped.x, 8.0f, 1e-4f);
	EXPECT_EQ(KeepApart({20.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 0.0f}, 8.0f), glm::vec2(10.0f, 0.0f));
	EXPECT_EQ(KeepApart({4.0f, 0.0f}, {6.0f, 0.0f}, {0.0f, 0.0f}, 8.0f), glm::vec2(6.0f, 0.0f));
	// Passing right through, it stays on its own side
	EXPECT_NEAR(KeepApart({20.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, 8.0f).x, 8.0f, 1e-4f);
}

TEST(CreatureFight, StepsBackOrSidewaysOnlyWithinRange)
{
	const Arena arena {.centre = {0.0f, 0.0f}, .radius = 50.0f};
	EXPECT_TRUE(WithinRange(arena, {39.0f, 0.0f}));
	EXPECT_FALSE(WithinRange(arena, {41.0f, 0.0f}));
	EXPECT_TRUE(CanStep(arena, {45.0f, 0.0f}, Step::Forward));
	EXPECT_FALSE(CanStep(arena, {45.0f, 0.0f}, Step::Back));
	EXPECT_FALSE(CanStep(arena, {45.0f, 0.0f}, Step::Left));
	EXPECT_TRUE(CanStep(arena, {10.0f, 0.0f}, Step::Right));
	const auto clamped = ClampToRange(arena, {0.0f, 100.0f});
	EXPECT_NEAR(clamped.y, 40.0f, 1e-4f);
}

TEST(CreatureFight, ClickHeightPicksTheBand)
{
	EXPECT_EQ(BandOf(0.1f), Band::Low);
	EXPECT_EQ(BandOf(0.32f), Band::Low);
	EXPECT_EQ(BandOf(0.33f), Band::Mid);
	EXPECT_EQ(BandOf(0.5f), Band::Mid);
	EXPECT_EQ(BandOf(0.66f), Band::Mid);
	EXPECT_EQ(BandOf(0.67f), Band::High);
	EXPECT_EQ(AttackMove(Band::High).kind, Move::Kind::High);
	EXPECT_EQ(AttackMove(Band::Low).kind, Move::Kind::Low);
}

TEST(CreatureFight, GroundClickStepsAlongTheLongerAxis)
{
	EXPECT_EQ(StepTowards({1.0f, 5.0f}), Step::Forward);
	EXPECT_EQ(StepTowards({1.0f, -5.0f}), Step::Back);
	EXPECT_EQ(StepTowards({5.0f, 1.0f}), Step::Right);
	EXPECT_EQ(StepTowards({-5.0f, -1.0f}), Step::Left);
	EXPECT_EQ(StepMove(Step::Left).value, animations::k_StepLeft);
	EXPECT_EQ(StepOf(animations::k_StepBack), Step::Back);
	EXPECT_FALSE(StepOf(animations::k_AttackHigh).has_value());
}

TEST(CreatureFight, ChargeRunsToOnePointTwoSeconds)
{
	EXPECT_FALSE(InitialCharge(Move::Kind::Mid).has_value());
	EXPECT_EQ(InitialCharge(Move::Kind::Block), 0.0f);
	EXPECT_EQ(InitialCharge(Move::Kind::Animation), 0.0f);
	EXPECT_EQ(InitialCharge(Move::Kind::Special), k_MaxChargeMs);
	EXPECT_EQ(InitialCharge(Move::Kind::Spell), k_MaxChargeMs);
	EXPECT_FLOAT_EQ(ReleasedCharge(5000.0f), 1200.0f);
	EXPECT_FLOAT_EQ(ReleasedCharge(-3.0f), 0.0f);
	EXPECT_FLOAT_EQ(BlowSpeed(0.0f), 0.5f);
	EXPECT_FLOAT_EQ(BlowSpeed(600.0f), 1.0f);
	EXPECT_FLOAT_EQ(BlowSpeed(3000.0f), 1.5f);
}

TEST(CreatureFight, QueueHoldsTwelveAndAClickReplacesIt)
{
	MoveQueue queue;
	for (size_t i = 0; i < MoveQueue::k_Capacity; ++i)
	{
		EXPECT_TRUE(queue.Push(BlockMove(), false));
	}
	EXPECT_FALSE(queue.Push(BlockMove(), false));
	EXPECT_EQ(queue.Size(), 12u);
	EXPECT_TRUE(queue.Push(AttackMove(Band::High), true));
	EXPECT_EQ(queue.Size(), 1u);
	EXPECT_TRUE(queue.HasWaiting());
	queue.Push(StepMove(Step::Back), false);
	EXPECT_TRUE(queue.Release(700.0f));
	EXPECT_EQ(queue.Front()->chargeMs, 700.0f);
	EXPECT_FALSE(queue.Release(100.0f));
	queue.Pop();
	EXPECT_EQ(queue.Front()->move, StepMove(Step::Back));
	queue.Pop();
	EXPECT_TRUE(queue.Empty());
	queue.Pop();
	EXPECT_TRUE(queue.Empty());
}

TEST(CreatureFight, GettingHitTakesAWaitingBlowAway)
{
	MoveQueue queue;
	queue.Push(BlockMove(), false);
	queue.Push(AttackMove(Band::Mid), false);
	queue.Push(StepMove(Step::Forward), false);
	queue.CancelWaiting();
	ASSERT_EQ(queue.Size(), 2u);
	EXPECT_EQ(queue.Moves()[0].move, BlockMove());
	EXPECT_EQ(queue.Moves()[1].move, StepMove(Step::Forward));
}

TEST(CreatureFight, OrdersAreTakenInRangeAndOnceCharged)
{
	Fighter fighter;
	Enter(fighter, State::Stance);
	fighter.queue.Push(AttackMove(Band::Low), false);
	EXPECT_FALSE(TakeOrder(fighter, true).has_value());
	fighter.queue.Release(1200.0f);
	EXPECT_FALSE(TakeOrder(fighter, false).has_value());
	const auto order = TakeOrder(fighter, true);
	ASSERT_TRUE(order.has_value());
	EXPECT_EQ(order->kind, Order::Kind::Blow);
	EXPECT_EQ(order->band, Band::Low);
	EXPECT_FLOAT_EQ(order->speed, 1.5f);
	EXPECT_TRUE(fighter.queue.Empty());

	// Blocking, anything ends the block and is made after it
	Enter(fighter, State::Block);
	fighter.queue.Push(StepMove(Step::Back), false);
	EXPECT_EQ(TakeOrder(fighter, true)->kind, Order::Kind::EndBlock);
	EXPECT_EQ(fighter.queue.Size(), 1u);
	Enter(fighter, State::Action, animations::k_AttackHigh);
	EXPECT_FALSE(TakeOrder(fighter, true).has_value());
}

TEST(CreatureFight, PlayerMovesTakeBackControlAndTeach)
{
	Fighter fighter;
	fighter.control = Control::Computer;
	fighter.computerWaitMs = 0.0f;
	EXPECT_TRUE(PlayerMove(fighter, AttackMove(Band::High), true));
	EXPECT_EQ(fighter.control, Control::Player);
	EXPECT_FLOAT_EQ(fighter.computerWaitMs, k_ComputerWaitsMs);
	EXPECT_FLOAT_EQ(fighter.tendency, 0.02f);
	PlayerMove(fighter, BlockMove(), false);
	EXPECT_FLOAT_EQ(fighter.tendency, (0.98f * 0.02f) - 0.02f);
	EXPECT_FLOAT_EQ(LearnTendency(1.0f, Move::Kind::Mid), 1.0f);
	EXPECT_FLOAT_EQ(LearnTendency(-1.0f, Move::Kind::Block), -1.0f);
	EXPECT_FLOAT_EQ(FirstTendency(-0.7f), 0.7f);
}

TEST(CreatureFight, HealthStaminaAndLife)
{
	EXPECT_FLOAT_EQ(FightHealthAtStart(1.0f), 1.0f);
	EXPECT_FLOAT_EQ(FightHealthAtStart(0.2f), 0.6f);
	EXPECT_FLOAT_EQ(StaminaAtStart(0.8f, 0.3f), 0.5f);
	EXPECT_FLOAT_EQ(StaminaAtStart(0.2f, 0.9f), 0.0f);
	EXPECT_FLOAT_EQ(RegainStamina(1.0f), 1.0f);
	EXPECT_NEAR(RegainStamina(0.5f), 0.5f + (1.0f / 600.0f), 1e-6f);
	// A knock out costs a quarter of the bar
	EXPECT_FLOAT_EQ(LifeAfterFight(1.0f, 0.0f), 0.75f);
	EXPECT_FLOAT_EQ(LifeAfterFight(0.8f, 0.6f), 0.7f);
	EXPECT_FLOAT_EQ(LifeAfterFight(0.1f, 0.0f), 0.0f);
	EXPECT_TRUE(HealthyEnoughToFight(0.11f));
	EXPECT_FALSE(HealthyEnoughToFight(0.1f));
	EXPECT_FLOAT_EQ(FaintSeconds(1.0f), 12.0f);
	EXPECT_FLOAT_EQ(FaintSeconds(2.0f), 16.0f);
	EXPECT_TRUE(NeedsRest(0.3f, 0.0f));
	EXPECT_TRUE(NeedsRest(0.9f, 0.5f));
	EXPECT_FALSE(NeedsRest(0.4f, 0.3f));
}

TEST(CreatureFight, DamageFollowsTheFormula)
{
	// Even creatures at normal speed: high 0.05, middle and low 0.03
	Blow even {.heightShare = 0.8f};
	EXPECT_FLOAT_EQ(DamageFactor(even), 1.0f);
	EXPECT_FLOAT_EQ(ResolveBlow(even, RecoilDirection::Plain).damage, 0.05f);
	even.heightShare = 0.5f;
	EXPECT_FLOAT_EQ(ResolveBlow(even, RecoilDirection::Plain).damage, 0.03f);
	even.heightShare = 0.1f;
	EXPECT_FLOAT_EQ(ResolveBlow(even, RecoilDirection::Plain).damage, 0.03f);

	// Size, strength and speed scale it
	const Blow strong {
	    .attackerSize = 1.0f, .defenderSize = 1.0f, .attackerStrength = 1.0f, .defenderStrength = 0.5f, .speed = 1.0f};
	EXPECT_FLOAT_EQ(DamageFactor(strong), 2.5f / 1.5f);
	EXPECT_FLOAT_EQ(DamageFactor({.attackerSize = 2.0f, .defenderSize = 1.0f, .speed = 0.5f}), 1.0f);
	// Kept from 0.04 to 2, doubled for the special move
	EXPECT_FLOAT_EQ(DamageFactor({.attackerSize = 4.0f, .defenderSize = 1.0f, .speed = 1.5f}), 2.0f);
	EXPECT_FLOAT_EQ(DamageFactor({.attackerSize = 4.0f, .defenderSize = 1.0f, .speed = 1.5f, .special = true}), 4.0f);
	EXPECT_FLOAT_EQ(DamageFactor({.attackerSize = 0.1f, .defenderSize = 2.0f, .speed = 0.5f}), 0.04f);
	// The most a blow does: 0.1, or 0.2 for the special move
	EXPECT_FLOAT_EQ(ResolveBlow({.attackerSize = 4.0f, .speed = 1.5f, .heightShare = 0.9f}, RecoilDirection::Plain).damage,
	                0.1f);
	EXPECT_FLOAT_EQ(
	    ResolveBlow({.attackerSize = 4.0f, .speed = 1.5f, .special = true, .heightShare = 0.9f}, RecoilDirection::Plain).damage,
	    0.2f);
}

TEST(CreatureFight, ABlockedBlowDoesATenth)
{
	const auto open = ResolveBlow({.heightShare = 0.8f}, RecoilDirection::Plain);
	const auto blocked = ResolveBlow({.heightShare = 0.8f, .blocked = true}, RecoilDirection::Plain);
	EXPECT_FLOAT_EQ(blocked.damage, open.damage * 0.1f);
}

TEST(CreatureFight, RecoilsByHeightAndDirection)
{
	EXPECT_EQ(ResolveBlow({.heightShare = 0.9f}, RecoilDirection::Plain).recoil, animations::k_RecoilHigh);
	EXPECT_EQ(ResolveBlow({.heightShare = 0.5f}, RecoilDirection::Right).recoil, animations::k_RecoilMid + 1);
	const auto low = ResolveBlow({.heightShare = 0.1f}, RecoilDirection::Bottom);
	EXPECT_EQ(low.recoil, animations::k_RecoilLow + 4);
	EXPECT_EQ(low.plainRecoil, animations::k_RecoilLow);
	EXPECT_EQ(RecoilDirectionOf({0.1f, 0.1f}), RecoilDirection::Plain);
	EXPECT_EQ(RecoilDirectionOf({0.8f, 0.2f}), RecoilDirection::Right);
	EXPECT_EQ(RecoilDirectionOf({-0.8f, 0.2f}), RecoilDirection::Left);
	EXPECT_EQ(RecoilDirectionOf({0.1f, 0.9f}), RecoilDirection::Top);
	EXPECT_EQ(RecoilDirectionOf({0.1f, -0.9f}), RecoilDirection::Bottom);
	EXPECT_EQ(WobbleAnimation(Band::High, true), animations::k_WobbleHighSide);
	EXPECT_EQ(WobbleAnimation(Band::Low, false), animations::k_WobbleLowFrontBack);
}

TEST(CreatureFight, WoundsByTheBlow)
{
	EXPECT_EQ(WoundKind(1, Sequence({0})), 0);
	EXPECT_EQ(WoundKind(0, Sequence({1})), 1);
	EXPECT_EQ(WoundKind(2, Sequence({1})), 2);
	EXPECT_EQ(WoundKind(1, Sequence({1, 2})), 5);
	EXPECT_EQ(BlowWoundType(animations::k_AttackEye), 1);
	EXPECT_EQ(BlowWoundType(animations::k_AttackKnee + 6), 2);
	EXPECT_EQ(BlowWoundType(animations::k_AttackBelly), 0);
	EXPECT_TRUE(Bleeds(3));
	EXPECT_FALSE(Bleeds(4));
}

TEST(CreatureFight, StatesFollowOneAnother)
{
	EXPECT_EQ(AfterAnimation(State::Start, true), State::Stance);
	EXPECT_EQ(AfterAnimation(State::Start, false), State::Finish);
	EXPECT_EQ(AfterAnimation(State::Stance, false), State::Finish);
	EXPECT_EQ(AfterAnimation(State::Action, true), State::Stance);
	EXPECT_EQ(AfterAnimation(State::Finish, true), State::Idle);
	EXPECT_EQ(AfterAnimation(State::BlockStart, true), State::Block);
	EXPECT_EQ(AfterAnimation(State::BlockRecoil, true), State::Block);
	EXPECT_EQ(AfterAnimation(State::BlockEnd, true), State::Stance);
	EXPECT_EQ(AfterAnimation(State::Faint, true), State::Lying);
	EXPECT_EQ(AfterAnimation(State::CastStart, true), State::Cast);
	EXPECT_EQ(AfterAnimation(State::Cast, true), State::CastEnd);
	EXPECT_EQ(AfterAnimation(State::CastEnd, true), State::Stance);
	EXPECT_EQ(AnimationOf(State::BlockRecoil), animations::k_RecoilBlock);
	EXPECT_TRUE(IsBlocking(State::Block, 0.0f, 1000.0f));
	EXPECT_FALSE(IsBlocking(State::BlockStart, 400.0f, 1000.0f));
	EXPECT_TRUE(IsBlocking(State::BlockStart, 600.0f, 1000.0f));
	EXPECT_TRUE(IsBlocking(State::BlockEnd, 400.0f, 1000.0f));
	EXPECT_FALSE(IsBlocking(State::BlockEnd, 600.0f, 1000.0f));
	EXPECT_FLOAT_EQ(PlaybackSpeed(animations::k_StepBack, 1.0f), 1.1f * 0.9f);
	EXPECT_FLOAT_EQ(PlaybackSpeed(animations::k_AttackHigh, 1.5f), 1.1f * 0.6f * 1.5f);

	Fighter fighter;
	Enter(fighter, State::Action, animations::k_AttackSpecial, 1.5f);
	EXPECT_TRUE(fighter.special);
	EXPECT_EQ(fighter.animation, animations::k_AttackSpecial);
	Enter(fighter, State::Stance);
	EXPECT_FALSE(fighter.special);
	EXPECT_EQ(fighter.animation, animations::k_Stance);
}

TEST(CreatureFight, ComputerKeepsTheGameTiers)
{
	EXPECT_EQ(TierOf(0.33f), 2u);
	EXPECT_EQ(TierOf(0.1f), 0u);
	EXPECT_EQ(TierOf(-0.9f), 0u);
}

TEST(CreatureFight, ComputerChoosesByWhatTheOpponentDoes)
{
	// A waiting opponent: the top tier always attacks, the bottom one turn in sixteen
	EXPECT_EQ(ChooseMove(2, Opponent::Stance, Always(5)).kind, Choice::Kind::Blow);
	EXPECT_EQ(ChooseMove(0, Opponent::Stance, Always(5)).kind, Choice::Kind::None);
	EXPECT_EQ(ChooseMove(0, Opponent::Stance, Sequence({0, 1})).kind, Choice::Kind::Blow);
	EXPECT_EQ(ChooseMove(0, Opponent::Stance, Sequence({0, 1})).band, Band::Mid);
	// Reeling in a block, or doing anything else, the opponent is attacked
	EXPECT_EQ(ChooseMove(0, Opponent::BlockRecoil, Always(0)).kind, Choice::Kind::Blow);
	EXPECT_EQ(ChooseMove(0, Opponent::OtherAction, Always(0)).kind, Choice::Kind::Blow);
	// Attacked, the bottom tier blocks a quarter of the time, else steps away
	EXPECT_EQ(ChooseMove(0, Opponent::Attacking, Sequence({0})).kind, Choice::Kind::Block);
	const auto away = ChooseMove(0, Opponent::Attacking, Sequence({1, 2}));
	EXPECT_EQ(away.kind, Choice::Kind::Step);
	EXPECT_EQ(away.step, Step::Back);
	// The top tier counter-attacks four times in five
	EXPECT_EQ(ChooseMove(2, Opponent::Attacking, Sequence({1, 0})).kind, Choice::Kind::Blow);
	EXPECT_EQ(ChooseMove(2, Opponent::Attacking, Sequence({0, 0})).kind, Choice::Kind::Block);
	// Stepping in: block, step, step or attack
	EXPECT_EQ(ChooseMove(0, Opponent::SteppingForward, Sequence({0})).kind, Choice::Kind::Block);
	EXPECT_EQ(ChooseMove(0, Opponent::SteppingForward, Sequence({1, 0})).kind, Choice::Kind::Step);
	EXPECT_EQ(ChooseMove(0, Opponent::SteppingForward, Sequence({3, 0})).kind, Choice::Kind::Blow);
	// Blocking: mostly waits, one turn in ten a step or a blow
	EXPECT_EQ(ChooseMove(0, Opponent::Blocking, Sequence({0})).kind, Choice::Kind::None);
	EXPECT_EQ(ChooseMove(0, Opponent::Blocking, Sequence({1, 0})).kind, Choice::Kind::Step);
	EXPECT_EQ(ChooseMove(0, Opponent::Blocking, Sequence({1, 1})).kind, Choice::Kind::Blow);
	EXPECT_EQ(ChooseMove(2, Opponent::Other, Always(0)).kind, Choice::Kind::None);
	EXPECT_TRUE(ComputerEndsBlock(Always(1)));
	EXPECT_FALSE(ComputerEndsBlock(Always(0)));
}

TEST(CreatureFight, OpponentStatesAsTheComputerSeesThem)
{
	EXPECT_EQ(OpponentOf(State::Action, animations::k_AttackKnee), Opponent::Attacking);
	EXPECT_EQ(OpponentOf(State::Action, animations::k_StepForward), Opponent::SteppingForward);
	EXPECT_EQ(OpponentOf(State::Action, animations::k_RecoilMid), Opponent::OtherAction);
	EXPECT_EQ(OpponentOf(State::BlockEnd, animations::k_EndBlock), Opponent::Blocking);
	EXPECT_EQ(OpponentOf(State::Cast, animations::k_Cast), Opponent::Casting);
	EXPECT_EQ(OpponentOf(State::Start, animations::k_Start), Opponent::Other);
}

TEST(CreatureFight, ABlowIsChosenThatLandsAtTheBand)
{
	// Reaches 12 ahead at the head, 10 at the belly, 8 at the knee
	const std::array<Reach, 4> reaches {{
	    {.animation = animations::k_AttackHigh, .reach = 12.0f, .height = 12.0f},
	    {.animation = animations::k_AttackBelly, .reach = 10.0f, .height = 7.0f},
	    {.animation = animations::k_AttackKnee, .reach = 8.0f, .height = 3.0f},
	    {.animation = animations::k_AttackSpecial, .reach = 30.0f, .height = 7.0f},
	}};
	// In reach of everything, the band asked for wins
	EXPECT_EQ(ChooseAttack(reaches, Band::Mid, 6.0f, 4.0f, 1.0f, 15.0f).animation, animations::k_AttackBelly);
	EXPECT_EQ(ChooseAttack(reaches, Band::Low, 6.0f, 4.0f, 1.0f, 15.0f).animation, animations::k_AttackKnee);
	EXPECT_EQ(ChooseAttack(reaches, Band::High, 6.0f, 4.0f, 1.0f, 15.0f).animation, animations::k_AttackHigh);
	// Out of reach, it steps forward first
	const auto far = ChooseAttack(reaches, Band::Low, 20.0f, 4.0f, 1.0f, 15.0f);
	EXPECT_FALSE(far.animation.has_value());
	EXPECT_EQ(far.step, Step::Forward);
	// Too close for the knee to land well, it steps back
	EXPECT_EQ(ChooseAttack(reaches, Band::Low, -2.0f, 4.0f, 1.0f, 15.0f).step, Step::Back);
	// A blow too high for a small opponent isn't used, and nothing in reach of anything steps in
	EXPECT_EQ(ChooseAttack(reaches, Band::High, 6.0f, 4.0f, 1.0f, 10.0f).animation, animations::k_AttackBelly);
	EXPECT_EQ(ChooseAttack({}, Band::High, 6.0f, 4.0f, 1.0f, 10.0f).step, Step::Forward);
}

TEST(CreatureFight, OutcomesAndStarting)
{
	EXPECT_TRUE(WinnerPoos(-0.6f));
	EXPECT_FALSE(WinnerPoos(-0.4f));
	EXPECT_EQ(Response(0.8f, 0.5f), animations::k_Happy);
	EXPECT_EQ(Response(0.4f, 0.5f), animations::k_Sad);
	EXPECT_TRUE(WantsToFight(0.7f, 1.0f, 50.0f, 1.0f, 200.0f));
	EXPECT_FALSE(WantsToFight(0.5f, 1.0f, 50.0f, 1.0f, 200.0f));
	EXPECT_FALSE(WantsToFight(0.7f, 0.05f, 50.0f, 1.0f, 200.0f));
	EXPECT_FALSE(WantsToFight(0.7f, 1.0f, 70.0f, 1.0f, 200.0f));
	EXPECT_FALSE(WantsToFight(0.7f, 1.0f, 50.0f, 1.0f, 30.0f));
	const auto camera = CameraOrigin({.centre = {10.0f, 20.0f}, .radius = 40.0f}, 5.0f, {2.0f, 0.0f});
	EXPECT_EQ(camera, glm::vec3(50.0f, 25.0f, 20.0f));
	// Across the line between the fighters
	const auto side = CameraSide({0.0f, 0.0f}, {10.0f, 0.0f});
	EXPECT_NEAR(side.x, 0.0f, 1e-6f);
	EXPECT_NEAR(std::abs(side.y), 1.0f, 1e-6f);
}

TEST(CreatureFightHud, LayoutAndColours)
{
	const auto layout = creature_fight_hud::Compute({1400, 700});
	EXPECT_NEAR(layout.box.max.x - layout.box.min.x, 660.0f, 1e-3f);
	EXPECT_NEAR(layout.box.max.y - layout.box.min.y, 90.0f, 1e-3f);
	for (const auto& row : layout.rows)
	{
		EXPECT_GT(row.health.max.y - row.health.min.y, row.stamina.max.y - row.stamina.min.y);
		EXPECT_LE(row.stamina.max.y, layout.box.max.y);
		EXPECT_GE(row.name.x, layout.box.min.x);
	}
	EXPECT_LT(layout.rows[0].stamina.max.y, layout.rows[1].name.y);
	const creature_fight_hud::Rect bar {.min = {10.0f, 0.0f}, .max = {110.0f, 5.0f}};
	EXPECT_FLOAT_EQ(creature_fight_hud::Filled(bar, 0.25f).max.x, 35.0f);
	EXPECT_FLOAT_EQ(creature_fight_hud::Filled(bar, 2.0f).max.x, 110.0f);
	EXPECT_EQ(creature_fight_hud::BarColour(1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	EXPECT_EQ(creature_fight_hud::BarColour(0.5f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
	EXPECT_EQ(creature_fight_hud::BarColour(0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	EXPECT_EQ(creature_fight_hud::SpeciesName(CreatureType::Tiger), u"Tiger");
	EXPECT_EQ(creature_fight_hud::SpeciesName(CreatureType::Unknown), u"Creature");
}
