/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreaturePlanActions.h"

#include <algorithm>
#include <array>

#include "Creature/CreatureLayers.h"

using namespace openblack;
using namespace openblack::creature_plan_actions;
namespace animations = openblack::creature_layers::animations;
using creature_mind::Activity;

namespace
{
/// Actions of the creature's animation set that have no name elsewhere
constexpr size_t k_Sneeze = 62;
constexpr size_t k_LookAtMe = 69;
constexpr size_t k_PickMe = 74;
/// Looking something over from where it stands, following it about, and gazing at the view, last this long
constexpr float k_LookSeconds = 4.0f;
constexpr float k_FollowSeconds = 8.0f;
constexpr float k_GazeSeconds = 6.0f;

constexpr std::array k_Executors {
    Executor {.action = "EatAfterExamining", .target = Target::Food, .build = Build::Eat, .activity = Activity::Eat},
    Executor {.action = "EatAlive", .target = Target::LiveFood, .build = Build::Eat, .activity = Activity::Eat},
    Executor {.action = "EatFromFoodPile", .target = Target::Food, .build = Build::Eat, .activity = Activity::Eat},
    Executor {.action = "SleepOnTheSpot", .build = Build::Sleep, .activity = Activity::Sleep},
    Executor {.action = "Poo", .build = Build::Poo, .activity = Activity::Poo},
    Executor {.action = "Puke", .build = Build::Puke, .activity = Activity::Puke},
    Executor {.action = "DrinkFromTheSea", .build = Build::Drink, .activity = Activity::Drink},
    Executor {.action = "ExamineByPickingUp",
              .target = Target::Pickable,
              .build = Build::ExamineByPickingUp,
              .activity = Activity::Examine},
    Executor {.action = "ExamineByLooking", .target = Target::Anything, .build = Build::ExamineByLooking},
    Executor {.action = "ExamineByFollowing", .target = Target::Living, .build = Build::ExamineByFollowing},
    Executor {.action = "ThrowBallAtObject",
              .target = Target::Pickable,
              .build = Build::ThrowAbout,
              .activity = Activity::PlayWithObject},
    // Throwing things about and into the sea, kicking a ball about and throwing to impress are all throwing what it
    // picks up somewhere nearby, as the throws aimed at the sea or at an audience need systems that aren't here yet
    Executor {
        .action = "ThrowAround", .target = Target::Pickable, .build = Build::ThrowAbout, .activity = Activity::PlayWithObject},
    Executor {.action = "KickBallAround",
              .target = Target::Pickable,
              .build = Build::ThrowAbout,
              .activity = Activity::PlayWithObject},
    Executor {.action = "ThrowInTheSea",
              .target = Target::Pickable,
              .build = Build::ThrowAbout,
              .activity = Activity::PlayWithObject},
    Executor {.action = "ThrowToImpress",
              .target = Target::Pickable,
              .build = Build::ThrowAbout,
              .activity = Activity::PlayWithObject},
    Executor {.action = "KickTree", .target = Target::Tree, .build = Build::Destroy},
    Executor {.action = "PullSillyFaces", .build = Build::Emote, .animation = animations::k_FeelPlayful},
    Executor {.action = "Hurl", .target = Target::Pickable, .build = Build::Hurl, .activity = Activity::Hurl},
    Executor {.action = "Stomp", .target = Target::Destroyable, .build = Build::Destroy},
    Executor {.action = "Kick", .target = Target::Destroyable, .build = Build::Destroy},
    Executor {.action = "SitDown", .build = Build::SitDown, .activity = Activity::Sit},
    Executor {.action = "BeIdle", .build = Build::BeIdle, .activity = Activity::BeIdle},
    Executor {.action = "HangAroundAtHome", .build = Build::HangAround, .activity = Activity::HangAround},
    Executor {.action = "CommunicateState", .build = Build::ShowDesire, .activity = Activity::ShowDesire},
    Executor {.action = "WaveAtPlayer", .build = Build::FaceCameraEmote, .animation = animations::k_FriendlyWave},
    Executor {.action = "LookAtHand", .build = Build::FaceCameraEmote, .animation = k_LookAtMe},
    Executor {.action = "PointAtCamera", .build = Build::FaceCameraEmote, .animation = k_LookAtMe},
    Executor {.action = "BePatheticToPlayer", .build = Build::FaceCameraEmote, .animation = k_PickMe},
    Executor {.action = "HowlAtPlayer", .build = Build::FaceCameraEmote, .animation = animations::k_Summon},
    Executor {.action = "RunAwayFromObject", .target = Target::Living, .build = Build::RunFromObject},
    Executor {.action = "BeFrightenedOnTheSpot", .build = Build::Emote, .animation = animations::k_Frightened},
    Executor {.action = "BeSad", .build = Build::Emote, .animation = animations::k_Sad},
    Executor {.action = "Scratch", .build = Build::Emote, .animation = animations::k_Scratch},
    Executor {.action = "Shiver", .build = Build::Emote, .animation = animations::k_Cold},
    Executor {.action = "Sneeze", .build = Build::Emote, .animation = k_Sneeze},
    Executor {.action = "ShowHotness", .build = Build::Emote, .animation = animations::k_Hot},
    Executor {.action = "RunAwayFromPlayer", .build = Build::RunFromPlayer},
    Executor {.action = "ShowImpressiveAnimation", .build = Build::Emote, .animation = animations::k_Impress},
    Executor {
        .action = "Stroke", .target = Target::Villager, .build = Build::ApproachEmote, .animation = animations::k_FeelingNice},
    Executor {
        .action = "SmileAtFriend", .target = Target::Creature, .build = Build::ApproachEmote, .animation = animations::k_Happy},
    Executor {.action = "WaveAtFriend",
              .target = Target::Creature,
              .build = Build::ApproachEmote,
              .animation = animations::k_FriendlyWave},
    Executor {.action = "LookAtMountains", .build = Build::LookAbout},
    Executor {.action = "LookOutToSea", .build = Build::LookAbout},
    Executor {.action = "LookAtSun", .build = Build::LookAbout},
    Executor {.action = "LookAtMoon", .build = Build::LookAbout},
};
} // namespace

std::span<const Executor> creature_plan_actions::All()
{
	return k_Executors;
}

const Executor* creature_plan_actions::For(std::string_view action)
{
	const auto found = std::ranges::find(k_Executors, action, &Executor::action);
	return found != k_Executors.end() ? &*found : nullptr;
}

bool creature_plan_actions::Possible(const Executor& executor, const Situation& situation)
{
	switch (executor.build)
	{
	case Build::Drink:
		return situation.water.has_value();
	case Build::Hurl:
		return situation.hurlTarget.has_value();
	case Build::FaceCameraEmote:
	case Build::RunFromPlayer:
		return situation.camera.has_value();
	case Build::ShowDesire:
		return situation.showDesireAnimation.has_value();
	default:
		return true;
	}
}

std::optional<std::vector<creature_mind::Step>> creature_plan_actions::Agenda(const Executor& executor,
                                                                              std::optional<uint32_t> object,
                                                                              glm::vec2 objectPoint, const Situation& situation,
                                                                              const creature_mind::Random& random)
{
	if (!Possible(executor, situation) || (executor.target != Target::None && !object.has_value()))
	{
		return std::nullopt;
	}
	switch (executor.build)
	{
	case Build::Eat:
		return creature_mind::Eat(*object);
	case Build::Sleep:
		return creature_mind::Sleep(random);
	case Build::Poo:
		return creature_mind::Poo(random);
	case Build::Puke:
		return creature_mind::Puke();
	case Build::Drink:
		return creature_mind::Drink(situation.water->shore, situation.water->water);
	case Build::ExamineByPickingUp:
		return creature_mind::ExamineByPickingUp(*object, random);
	case Build::ExamineByLooking:
		return creature_mind::LookAt(objectPoint, k_LookSeconds);
	case Build::ExamineByFollowing:
		return creature_mind::FollowFor(*object, k_FollowSeconds);
	case Build::ThrowAbout:
		return creature_mind::ThrowAbout(*object, random);
	case Build::Hurl:
		return creature_mind::Hurl(*object, *situation.hurlTarget, random);
	case Build::Destroy:
		return creature_mind::DestroyThing(*object);
	case Build::SitDown:
		return std::vector {creature_mind::SitDown(random)};
	case Build::BeIdle:
		return creature_mind::BeIdle(random);
	case Build::HangAround:
		return creature_mind::HangAround(random);
	case Build::ShowDesire:
		return creature_mind::Emote(*situation.showDesireAnimation);
	case Build::Emote:
		return creature_mind::Emote(executor.animation);
	case Build::FaceCameraEmote:
		return creature_mind::FaceAndEmote(*situation.camera, executor.animation);
	case Build::ApproachEmote:
		return creature_mind::ApproachAndEmote(*object, executor.animation);
	case Build::RunFromObject:
		return creature_mind::RunFrom(objectPoint);
	case Build::RunFromPlayer:
		return creature_mind::RunFrom(*situation.camera);
	case Build::LookAbout:
		return creature_mind::LookAbout(k_GazeSeconds);
	}
	return std::nullopt;
}
