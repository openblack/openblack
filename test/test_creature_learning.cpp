/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <array>
#include <vector>

#include <gtest/gtest.h>

#include "Creature/CreatureDecisionTree.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureLearning.h"
#include "Creature/CreatureMindModel.h"
#include "Creature/CreaturePlanActions.h"
#include "Creature/CreaturePlanner.h"
#include "Creature/CreatureWatching.h"

using namespace openblack;
using creature_desires::Desire;
using creature_tree::Attribute;

namespace
{
creature_tree::Belief Thing(uint32_t type, uint32_t animate)
{
	creature_tree::Belief belief {.type = type};
	belief.Set(Attribute::Type, type);
	belief.Set(Attribute::Animate, animate);
	belief.Set(Attribute::Allegiance, 1);
	return belief;
}

constexpr uint32_t k_Villager = creature_tree::belief_types::k_Villager;
constexpr uint32_t k_Food = creature_tree::belief_types::k_Other;
const std::array k_HungerAttributes {Attribute::Allegiance, Attribute::Animate, Attribute::Type};

creature_desires::Desires TwoDesires()
{
	creature_desires::Desires desires;
	for (auto& desire : desires.desires)
	{
		desire.activated = false;
	}
	auto& hunger = desires[Desire::Hunger];
	hunger.activated = true;
	hunger.value = 0.5f;
	hunger.max = 2.0f;
	hunger.decay = 0.9f;
	hunger.increaseSeconds = 20.0f;
	hunger.sources = {{.type = 14, .value = 0.8f, .threshold = 0.5f}, {.type = 15, .value = 0.1f, .threshold = 0.5f}};
	auto& anger = desires[Desire::Anger];
	anger.activated = true;
	anger.value = 0.2f;
	anger.max = 0.8f;
	anger.increaseSeconds = 5.0f;
	return desires;
}
} // namespace

// Decision trees

TEST(CreatureDecisionTree, BucketsFeedbackByTheFirstStepWithinAQuarter)
{
	EXPECT_EQ(creature_tree::BucketOf(-1.0f), 0u);
	EXPECT_EQ(creature_tree::BucketOf(1.0f), 9u);
	EXPECT_EQ(creature_tree::BucketOf(0.0f), 4u);
	EXPECT_EQ(creature_tree::BucketOf(0.5f), 7u);
}

TEST(CreatureDecisionTree, EntropyOfAgreeingExamplesIsZero)
{
	const std::vector<creature_tree::Episode> same {{.belief = Thing(k_Food, 0), .feedback = 1.0f},
	                                                {.belief = Thing(k_Food, 0), .feedback = 1.0f}};
	EXPECT_FLOAT_EQ(creature_tree::Entropy(same), 0.0f);
	const std::vector<creature_tree::Episode> mixed {{.belief = Thing(k_Food, 0), .feedback = 1.0f},
	                                                 {.belief = Thing(k_Villager, 1), .feedback = -1.0f}};
	// One bit for the signs, one for the steps
	EXPECT_NEAR(creature_tree::Entropy(mixed), 1.0f, 1e-5f);
}

TEST(CreatureDecisionTree, GainPicksTheAttributeThatSeparatesFeedback)
{
	std::vector<creature_tree::Episode> episodes;
	for (int i = 0; i < 4; ++i)
	{
		episodes.push_back({.belief = Thing(k_Food, 0), .feedback = -1.0f});
		episodes.push_back({.belief = Thing(k_Villager, 1), .feedback = 1.0f});
	}
	EXPECT_GT(creature_tree::Gain(episodes, Attribute::Type), 0.0f);
	EXPECT_FLOAT_EQ(creature_tree::Gain(episodes, Attribute::Allegiance), 0.0f);
	// An example without the attribute gives no gain
	episodes.push_back({.belief = {.type = k_Food}, .feedback = 1.0f});
	EXPECT_FLOAT_EQ(creature_tree::Gain(episodes, Attribute::Type), 0.0f);
}

TEST(CreatureDecisionTree, BuildsAndEvaluates)
{
	std::vector<creature_tree::Episode> episodes {{.belief = Thing(k_Food, 0), .feedback = -1.0f},
	                                              {.belief = Thing(k_Villager, 1), .feedback = 1.0f},
	                                              {.belief = Thing(k_Food, 0), .feedback = -1.0f}};
	const auto tree = creature_tree::Build(episodes, k_HungerAttributes);
	ASSERT_FALSE(tree.nodes.empty());
	EXPECT_TRUE(tree.nodes[0].test.has_value());
	EXPECT_FLOAT_EQ(creature_tree::Evaluate(tree, Thing(k_Food, 0)), -1.0f);
	EXPECT_FLOAT_EQ(creature_tree::Evaluate(tree, Thing(k_Villager, 1)), 0.8f);
	// A thing whose values no example had is neither good nor bad
	EXPECT_FLOAT_EQ(creature_tree::Evaluate(tree, Thing(creature_tree::belief_types::k_Tree, 2)), 0.0f);
	EXPECT_FALSE(creature_tree::Describe(tree).empty());
}

TEST(CreatureDecisionTree, EmptyTreeKnowsNothing)
{
	const auto tree = creature_tree::Build({}, k_HungerAttributes);
	EXPECT_FLOAT_EQ(creature_tree::Evaluate(tree, Thing(k_Food, 0)), 0.0f);
	EXPECT_FLOAT_EQ(creature_tree::Usefulness(0.0f), 0.1f);
}

TEST(CreatureDecisionTree, UsefulnessFromUtility)
{
	EXPECT_FLOAT_EQ(creature_tree::Usefulness(1.0f), 1.0f);
	EXPECT_FLOAT_EQ(creature_tree::Usefulness(-1.0f), 0.0f);
	EXPECT_FLOAT_EQ(creature_tree::Usefulness(-0.5f), 0.05f);
	EXPECT_FLOAT_EQ(creature_tree::Usefulness(0.5f), 0.55f);
}

TEST(CreatureDecisionTree, KeepsTheNewestSixteenExamples)
{
	std::vector<creature_tree::Episode> episodes;
	for (int i = 0; i < 20; ++i)
	{
		creature_tree::AddEpisode(episodes, {.belief = Thing(k_Food, 0), .feedback = static_cast<float>(i) / 20.0f});
	}
	ASSERT_EQ(episodes.size(), creature_tree::k_MaxEpisodes);
	EXPECT_FLOAT_EQ(episodes.back().feedback, 19.0f / 20.0f);
	EXPECT_FLOAT_EQ(episodes.front().feedback, 4.0f / 20.0f);
}

TEST(CreatureDecisionTree, SlotsFollowTheKindOfThing)
{
	EXPECT_EQ(creature_tree::SlotsFor(creature_tree::belief_types::k_Villager).size(), 11u);
	EXPECT_EQ(creature_tree::SlotsFor(creature_tree::belief_types::k_Forest).size(), 8u);
	EXPECT_EQ(creature_tree::SlotsFor(creature_tree::belief_types::k_Animal).size(), 7u);
	const std::vector<uint32_t> slots {1, 0, 1, 7, 0, 0, 6, 1, 16, 1, 0};
	const auto belief = creature_tree::FromSlots(k_Villager, slots);
	EXPECT_EQ(belief.Value(Attribute::Sex), 1u);
	EXPECT_EQ(belief.Value(Attribute::VillagerJob), 16u);
	EXPECT_EQ(creature_tree::ToSlots(belief), slots);
}

// The planner

TEST(CreaturePlanner, DistancePriority)
{
	EXPECT_FLOAT_EQ(creature_planner::DistancePriority(true, 500.0f, 0.4f), 1.6f);
	EXPECT_FLOAT_EQ(creature_planner::DistancePriority(false, 0.0f, 0.4f), 1.0f);
	EXPECT_FLOAT_EQ(creature_planner::DistancePriority(false, 100.0f, 0.4f), 0.8f);
	EXPECT_FLOAT_EQ(creature_planner::DistancePriority(false, 900.0f, 0.4f), 0.6f);
}

TEST(CreaturePlanner, ActionPriorityAndNovelty)
{
	EXPECT_FLOAT_EQ(creature_planner::ActionPriority(-1.0f), 0.5f);
	EXPECT_FLOAT_EQ(creature_planner::ActionPriority(1.0f), 0.505f);
	EXPECT_FLOAT_EQ(creature_planner::ActionPriority(1.0f, true), 0.0f);
	EXPECT_FLOAT_EQ(creature_planner::Novelty(0, false), 0.0f);
	EXPECT_FLOAT_EQ(creature_planner::Novelty(36000, false), 0.01f);
	EXPECT_FLOAT_EQ(creature_planner::Novelty(900, false), 0.01f);
	EXPECT_NEAR(creature_planner::Novelty(36000, true), 0.01f, 1e-6f);
	EXPECT_NEAR(creature_planner::Novelty(900, true), 4.0f * 0.025f * 0.025f, 1e-6f);
}

TEST(CreaturePlanner, PriorityAndSwitching)
{
	EXPECT_FLOAT_EQ(creature_planner::Priority(1.0f, std::nullopt, 0.5f, 0.1f, std::nullopt), 50.0f);
	EXPECT_FLOAT_EQ(creature_planner::Priority(1.0f, 1.0f, 0.5f, 0.1f, std::nullopt), 500.0f);
	EXPECT_FLOAT_EQ(creature_planner::Priority(1.0f, std::nullopt, 0.0f, 0.1f, std::nullopt), 1.0f);
	EXPECT_TRUE(creature_planner::ShouldSwitch(21.0f, 10.0f));
	EXPECT_FALSE(creature_planner::ShouldSwitch(20.0f, 10.0f));
	EXPECT_TRUE(creature_planner::ShouldSwitch(1.0f, 0.0f));
}

TEST(CreaturePlanner, PlansTheMostUsefulGoal)
{
	const std::vector<creature_planner::ObjectCandidate> objects {
	    {.id = 1, .distance = 10.0f, .usefulness = 0.0f},
	    {.id = 2, .distance = 150.0f, .usefulness = 0.82f},
	};
	const std::vector<creature_planner::ActionCandidate> actions {{.action = 12, .needsObject = true}};
	const auto plan = creature_planner::PlanDesire(Desire::Hunger, 1.0f, objects, 0.05f, actions);
	ASSERT_TRUE(plan.has_value());
	EXPECT_EQ(plan->object, 2u);
	EXPECT_FLOAT_EQ(plan->goalUsefulness, 0.82f);
	// Without anything learnt the nearer wins
	const std::vector<creature_planner::ObjectCandidate> unknown {
	    {.id = 1, .distance = 10.0f},
	    {.id = 2, .distance = 150.0f},
	};
	EXPECT_EQ(creature_planner::PlanDesire(Desire::Hunger, 1.0f, unknown, 0.05f, actions)->object, 1u);
	// An action that needs a goal can't be planned without one
	EXPECT_FALSE(creature_planner::PlanDesire(Desire::Hunger, 1.0f, {}, 0.05f, actions).has_value());
}

TEST(CreaturePlanner, OpinionBreaksTies)
{
	const std::vector<creature_planner::ActionCandidate> actions {{.action = 1, .opinion = 1.0f, .turnsSinceDone = 0},
	                                                              {.action = 2, .opinion = -1.0f, .turnsSinceDone = 0}};
	EXPECT_EQ(creature_planner::PlanDesire(Desire::Rest, 0.5f, {}, 0.02f, actions)->action, 1u);
}

TEST(CreaturePlanner, GoesRoundTheEligibleDesires)
{
	creature_planner::PlannerState state;
	std::array<bool, creature_desires::k_DesireCount> eligible {};
	eligible.at(static_cast<size_t>(Desire::Hunger)) = true;
	eligible.at(static_cast<size_t>(Desire::Tiredness)) = true;
	eligible.at(static_cast<size_t>(Desire::Rest)) = true;
	const auto first = creature_planner::NextGoals(state, eligible);
	ASSERT_EQ(first.size(), 2u);
	EXPECT_EQ(first[0], Desire::Hunger);
	EXPECT_EQ(first[1], Desire::Tiredness);
	const auto second = creature_planner::NextGoals(state, eligible);
	EXPECT_EQ(second[0], Desire::Rest);
	EXPECT_EQ(second[1], Desire::Hunger);
}

TEST(CreaturePlanner, ChoosesOnlyWhatIsPressingEnough)
{
	creature_planner::PlannerState state;
	state.best.at(0) = creature_planner::Plan {.desire = Desire::Impress, .action = 1, .priority = 30.0f};
	EXPECT_FALSE(creature_planner::Choose(state, 40.0f).has_value());
	EXPECT_TRUE(creature_planner::Choose(state, 15.0f).has_value());
	state.current = creature_planner::Plan {.desire = Desire::Hunger, .action = 2, .priority = 20.0f};
	EXPECT_FALSE(creature_planner::Choose(state, 15.0f).has_value());
	state.best.at(0)->priority = 41.0f;
	EXPECT_TRUE(creature_planner::Choose(state, 15.0f).has_value());
}

// Learning from feedback

TEST(CreatureLearning, LessonFactor)
{
	EXPECT_FLOAT_EQ(creature_learning::LessonFactor(2.0f, 1.0f), 0.5f);
	EXPECT_FLOAT_EQ(creature_learning::LessonFactor(2.0f, -1.0f), 1.5f);
	EXPECT_FLOAT_EQ(creature_learning::LessonFactor(200.0f, 0.0f), 1.0f);
}

TEST(CreatureLearning, DesireLessonAndDependencies)
{
	auto desires = TwoDesires();
	creature_learning::AllDesireRules rules {};
	auto& hunger = rules.at(static_cast<size_t>(Desire::Hunger));
	hunger = {.initialMax = 2.0f, .decayMin = 0.9f, .decayMax = 0.95f, .lessonDivisor = 2.0f, .learnable = true};
	creature_learning::Dependencies dependencies {};
	dependencies.at(static_cast<size_t>(Desire::Hunger)).at(static_cast<size_t>(Desire::Anger)) = -0.5f;
	creature_learning::LearnDesireLesson(desires, Desire::Hunger, 0, 1.0f, rules, dependencies);
	const auto& state = desires[Desire::Hunger];
	// Rewarded, it grows twice as fast, its source drives it sooner, it fades more slowly
	EXPECT_FLOAT_EQ(state.increaseSeconds, 10.0f);
	EXPECT_FLOAT_EQ(state.sources[0].threshold, 0.38f);
	EXPECT_FLOAT_EQ(state.sources[1].threshold, 0.5f);
	EXPECT_FLOAT_EQ(state.max, 2.0f);
	EXPECT_FLOAT_EQ(state.decay, 0.901f);
	// The opposed desire grows slower
	EXPECT_FLOAT_EQ(desires[Desire::Anger].increaseSeconds, 10.0f);
	creature_learning::LearnDesireLesson(desires, Desire::Hunger, 0, -1.0f, rules, dependencies);
	EXPECT_FLOAT_EQ(desires[Desire::Hunger].max, 1.9f);
	EXPECT_FLOAT_EQ(desires[Desire::Hunger].increaseSeconds, 15.0f);
}

TEST(CreatureLearning, OpinionsAndFeedbackStrength)
{
	EXPECT_FLOAT_EQ(creature_learning::OpinionAfter(0.0f, 1.0f), 0.8f);
	EXPECT_FLOAT_EQ(creature_learning::OpinionAfter(0.8f, -1.0f), -0.64f);
	EXPECT_FLOAT_EQ(creature_learning::Strengthened(0.2f), 0.5f);
	EXPECT_FLOAT_EQ(creature_learning::Strengthened(-0.9f), -0.9f);
}

TEST(CreatureLearning, SpreadsLessonsByDependency)
{
	creature_learning::Dependencies dependencies {};
	dependencies.at(static_cast<size_t>(Desire::Compassion)).at(static_cast<size_t>(Desire::Anger)) = -0.86f;
	dependencies.at(static_cast<size_t>(Desire::Compassion)).at(static_cast<size_t>(Desire::BeFriends)) = 0.5f;
	const auto spread = creature_learning::Spread(Desire::Compassion, 1.0f, dependencies);
	ASSERT_EQ(spread.size(), 3u);
	EXPECT_EQ(spread[0].first, Desire::Compassion);
	EXPECT_FLOAT_EQ(spread[1].second, -0.86f);
	EXPECT_FLOAT_EQ(spread[2].second, 0.5f);
}

TEST(CreatureLearning, CreditGoesToRecentActions)
{
	creature_learning::Context running {.action = 1, .running = true, .windowSeconds = 20.0f};
	EXPECT_FLOAT_EQ(creature_learning::LearningPriority(running), 1.0f);
	creature_learning::Context finished {.action = 2, .running = false, .secondsSince = 5.0f, .windowSeconds = 20.0f};
	EXPECT_FLOAT_EQ(creature_learning::LearningPriority(finished), 0.75f);
	finished.secondsSince = 25.0f;
	EXPECT_FLOAT_EQ(creature_learning::LearningPriority(finished), 0.0f);
	creature_learning::Context unlearnable {.action = 3, .learnable = false};
	EXPECT_FLOAT_EQ(creature_learning::LearningPriority(unlearnable), 0.0f);

	std::vector<creature_learning::Context> contexts;
	creature_learning::Remember(contexts, {.action = 1, .windowSeconds = 20.0f});
	creature_learning::Age(contexts, 1.0f);
	creature_learning::Remember(contexts, {.action = 2, .windowSeconds = 10.0f});
	creature_learning::Age(contexts, 1.0f);
	EXPECT_FALSE(contexts[0].running);
	EXPECT_EQ(creature_learning::BestContext(contexts), 1u);
	contexts[1].running = false;
	creature_learning::Age(contexts, 9.0f);
	// The older action's longer window now gives it the bigger share
	EXPECT_EQ(creature_learning::BestContext(contexts), 0u);
	for (uint32_t i = 0; i < 20; ++i)
	{
		creature_learning::Remember(contexts, {.action = i});
	}
	EXPECT_EQ(contexts.size(), creature_learning::k_MaxContexts);
}

TEST(CreatureLearning, DecidingSuppressesOpposedDesires)
{
	auto desires = TwoDesires();
	creature_learning::Dependencies dependencies {};
	dependencies.at(static_cast<size_t>(Desire::Hunger)).at(static_cast<size_t>(Desire::Anger)) = -0.5f;
	creature_learning::SuppressOpposed(desires, Desire::Hunger, dependencies, 10.0f);
	EXPECT_EQ(desires[Desire::Anger].suppressedTurns, 1200u);
	EXPECT_EQ(desires[Desire::Hunger].suppressedTurns, 0u);
}

TEST(CreatureLearning, DominanceAndActions)
{
	auto desires = TwoDesires();
	creature_learning::MakeLeastDominant(desires, Desire::Hunger, 1.3f);
	EXPECT_NEAR(desires[Desire::Hunger].value, 0.2f / 1.3f, 1e-6f);
	creature_learning::MakeFullyDominant(desires, Desire::Hunger);
	EXPECT_FLOAT_EQ(desires[Desire::Hunger].value, 2.0f);
	creature_learning::AfterAction(desires, Desire::Hunger, 0.3f);
	EXPECT_FLOAT_EQ(desires[Desire::Hunger].value, 0.6f);
}

TEST(CreatureLearning, DominantSourceFollowsDrive)
{
	auto desires = TwoDesires();
	creature_desires::UpdateDesires(desires, 10.0f);
	EXPECT_EQ(creature_learning::DominantSource(desires[Desire::Hunger]), 0u);
	creature_learning::ResetDrives(desires[Desire::Hunger]);
	EXPECT_FLOAT_EQ(desires[Desire::Hunger].sources[0].drive, 0.0f);
}

TEST(CreatureLearning, AttitudesToCreatures)
{
	std::vector<creature_learning::CreatureAttitude> attitudes;
	auto& attitude = creature_learning::AttitudeTo(attitudes, 7);
	const auto lessons = creature_learning::ChangeHowNice(attitude, 1.5f);
	EXPECT_FLOAT_EQ(attitude.howNice, 1.0f);
	EXPECT_EQ(lessons[0].first, Desire::BeFriends);
	EXPECT_FLOAT_EQ(lessons[0].second, 0.5f);
	EXPECT_FLOAT_EQ(lessons[1].second, -0.5f);
	EXPECT_EQ(&creature_learning::AttitudeTo(attitudes, 7), &attitudes[0]);
}

TEST(CreatureLearning, ThoughtsAndLikes)
{
	EXPECT_EQ(creature_learning::DesireLessonText(1.0f, "desire to eat"), "I've learnt to increase my desire to eat");
	EXPECT_EQ(creature_learning::ObjectLessonText(-1.0f, "villagers", "am trying to eat"),
	          "I've learnt to avoid villagers when I am trying to eat");
	creature_desires::DesireState state {.sources = {{.threshold = 0.3f}}};
	const std::array<float, 1> start {0.5f};
	EXPECT_LT(creature_learning::LikesLevel(Desire::Hunger, state, start), 5u);
	state.sources[0].threshold = 0.5f;
	EXPECT_EQ(creature_learning::LikesLevel(Desire::Hunger, state, start), 5u);
	state.value = 1.0f;
	state.max = 1.0f;
	EXPECT_EQ(creature_learning::LikesLevel(Desire::Fear, state, start), 0u);
}

// Learning by watching

TEST(CreatureWatching, LearnsASkillOnceWatchedLongEnough)
{
	const std::vector<creature_watching::SkillRule> skills {{.name = "Build", .watchSeconds = 6.0f, .minPhase = 4}};
	auto knowledge = creature_watching::StartKnowledge(skills, {});
	EXPECT_TRUE(creature_watching::SeeSkill(knowledge, 0, skills, 3, 0, 10.0f).ignored);
	EXPECT_FALSE(creature_watching::SeeSkill(knowledge, 0, skills, 5, 100, 10.0f).learnt);
	EXPECT_FALSE(creature_watching::SeeSkill(knowledge, 0, skills, 5, 150, 10.0f).learnt);
	EXPECT_TRUE(creature_watching::SeeSkill(knowledge, 0, skills, 5, 160, 10.0f).learnt);
	EXPECT_TRUE(knowledge.skillsKnown[0]);
}

TEST(CreatureWatching, LearnsAMiracleBySightingsAndSpecies)
{
	const std::vector<creature_watching::MiracleRule> miracles {{.name = "Heal", .timesToSee = 10, .minPhase = 8},
	                                                            {.name = "Food", .knownAtStart = true}};
	auto knowledge = creature_watching::StartKnowledge({}, miracles);
	EXPECT_TRUE(knowledge.miraclesKnown[1]);
	EXPECT_EQ(creature_watching::TimesToLearn(10, 1.5f), 15u);
	uint32_t turn = 0;
	// Seen again straight away it doesn't count
	EXPECT_NEAR(creature_watching::SeeMiracle(knowledge, 0, miracles, 13, turn, 3, 1.5f).share, 0.2f, 1e-6f);
	EXPECT_NEAR(creature_watching::SeeMiracle(knowledge, 0, miracles, 13, turn + 10, 3, 1.5f).share, 0.2f, 1e-6f);
	for (int i = 0; i < 3; ++i)
	{
		turn += creature_watching::k_MiracleSightingTurns;
		EXPECT_FALSE(creature_watching::SeeMiracle(knowledge, 0, miracles, 13, turn, 3, 1.5f).learnt);
	}
	turn += creature_watching::k_MiracleSightingTurns;
	EXPECT_TRUE(creature_watching::SeeMiracle(knowledge, 0, miracles, 13, turn, 3, 1.5f).learnt);
}

TEST(CreatureWatching, MimicryStages)
{
	const std::vector<creature_watching::MimicRule> rules {
	    {.name = "Play with toy", .chance = 0.9f, .needsLearningLeash = false, .stageSteps = 1, .copiesDesire = true},
	    {.name = "Steal", .chance = 0.9f, .needsLearningLeash = true, .stageSteps = 1},
	};
	std::optional<creature_watching::Mimicry> mimicry;
	const auto never = [] { return 0.99f; };
	const auto always = [] { return 0.0f; };
	const auto noExtra = [](uint32_t) { return 0u; };
	creature_watching::MimicConditions grown {.phase = 13};
	EXPECT_FALSE(creature_watching::StartMimicry(mimicry, 0, rules, grown, std::nullopt, never));
	EXPECT_FALSE(creature_watching::StartMimicry(mimicry, 1, rules, grown, std::nullopt, always));
	creature_watching::MimicConditions young {.phase = 2};
	EXPECT_FALSE(creature_watching::StartMimicry(mimicry, 0, rules, young, std::nullopt, always));
	ASSERT_TRUE(creature_watching::StartMimicry(mimicry, 0, rules, grown, 5u, always));
	EXPECT_EQ(mimicry->stage, creature_watching::MimicStage::Notice);
	creature_watching::StepMimicry(mimicry, rules, noExtra);
	creature_watching::StepMimicry(mimicry, rules, noExtra);
	EXPECT_EQ(mimicry->stage, creature_watching::MimicStage::CopyAction);
	creature_watching::StepMimicry(mimicry, rules, noExtra);
	creature_watching::StepMimicry(mimicry, rules, noExtra);
	EXPECT_EQ(mimicry->stage, creature_watching::MimicStage::CopyDesire);
	creature_watching::StepMimicry(mimicry, rules, noExtra);
	creature_watching::StepMimicry(mimicry, rules, noExtra);
	EXPECT_FALSE(mimicry.has_value());
}

// Carrying out plans

TEST(CreaturePlanActions, BuildsAgendas)
{
	const auto random = [](uint32_t) { return 0u; };
	const auto* eat = creature_plan_actions::For("EatAfterExamining");
	ASSERT_NE(eat, nullptr);
	EXPECT_FALSE(creature_plan_actions::Agenda(*eat, std::nullopt, {}, {}, random).has_value());
	EXPECT_EQ(creature_plan_actions::Agenda(*eat, 3u, {}, {}, random)->size(), 3u);
	const auto* drink = creature_plan_actions::For("DrinkFromTheSea");
	ASSERT_NE(drink, nullptr);
	EXPECT_FALSE(creature_plan_actions::Possible(*drink, {}));
	const auto* wave = creature_plan_actions::For("WaveAtPlayer");
	ASSERT_NE(wave, nullptr);
	creature_plan_actions::Situation situation {.camera = glm::vec2(0.0f, -100.0f)};
	EXPECT_EQ(creature_plan_actions::Agenda(*wave, std::nullopt, {}, situation, random)->size(), 2u);
	EXPECT_EQ(creature_plan_actions::For("CastFireball"), nullptr);
}

// The model of what is learnt

TEST(CreatureMindModel, LearningRebuildsTrees)
{
	creature_desires::Desires desires;
	auto learnt = creature_mind_model::Fresh(desires, 10, {});
	creature_mind_model::Learn(learnt, creature_mind_model::TreeKind::ActOn, Desire::Hunger,
	                           {.belief = Thing(k_Food, 0), .feedback = -1.0f}, k_HungerAttributes);
	creature_mind_model::Learn(learnt, creature_mind_model::TreeKind::ActOn, Desire::Hunger,
	                           {.belief = Thing(k_Villager, 1), .feedback = 1.0f}, k_HungerAttributes);
	const auto& tree = learnt.trees[0][static_cast<size_t>(Desire::Hunger)];
	EXPECT_LT(creature_tree::Evaluate(tree, Thing(k_Food, 0)), 0.0f);
	EXPECT_GT(creature_tree::Evaluate(tree, Thing(k_Villager, 1)), 0.0f);
	for (int i = 0; i < 12; ++i)
	{
		creature_mind_model::Think(learnt, "thought");
	}
	EXPECT_EQ(learnt.thoughts.size(), creature_mind_model::k_MaxThoughts);
}
