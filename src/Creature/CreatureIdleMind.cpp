/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureIdleMind.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include "Creature/CreatureLayers.h"
#include "Creature/CreatureObjectActions.h"

using namespace openblack;
using namespace openblack::creature_mind;
namespace animations = openblack::creature_layers::animations;

namespace
{
/// Random fractions of a second are drawn in this many steps
constexpr uint32_t k_FractionSteps = 1000;
/// A face is due again once its wait is within this much of over, so that the turns' rounding doesn't delay it
constexpr float k_FaceDue = 1e-3f;

float RandomFraction(const Random& random)
{
	return static_cast<float>(random(k_FractionSteps)) / static_cast<float>(k_FractionSteps);
}

/// The movements that the creature's movement says are done or have failed
bool IsSubMove(Movement::Kind kind)
{
	return kind == Movement::Kind::GoNearObject || kind == Movement::Kind::GetAwayFromObject ||
	       kind == Movement::Kind::TurnToFaceObject;
}

void FinishStep(IdleMind& mind)
{
	++mind.step;
	mind.stepStarted = false;
	mind.stepSeconds = 0.0f;
	mind.sitEnding = false;
}

/// A start, loop and end held for some seconds
Step Static(std::array<size_t, 3> sequence, float seconds)
{
	return {.kind = Step::Kind::Static,
	        .seconds = seconds,
	        .animation = sequence[1],
	        .sleepyEyes = false,
	        .movement = {},
	        .sequence = sequence};
}

Step Action(size_t animation, bool sleepyEyes)
{
	return {.kind = Step::Kind::Action, .seconds = 0.0f, .animation = animation, .sleepyEyes = sleepyEyes};
}

/// Every step of an agenda pulls a face for what the creature does or feels, except while its eyes are closed
std::vector<Step> WithFace(std::vector<Step> agenda, creature_face::Cue cue)
{
	for (auto& step : agenda)
	{
		if (!step.closedEyes && step.face == creature_face::Cue::None)
		{
			step.face = cue;
		}
	}
	return agenda;
}

Step Object(ObjectOrder order, Effect effect = Effect::None)
{
	Step step {.kind = Step::Kind::Object};
	step.order = order;
	step.effect = effect;
	return step;
}

Step PickUp(uint32_t object)
{
	return Object({.kind = ObjectOrder::Kind::PickUp, .object = object});
}
} // namespace

std::string_view creature_mind::Name(Activity activity)
{
	switch (activity)
	{
	case Activity::BeIdle:
		return "Being idle";
	case Activity::Sit:
		return "Sitting";
	case Activity::ShowDesire:
		return "Showing a desire";
	case Activity::HangAround:
		return "Hanging around";
	case Activity::Told:
		return "Doing as told";
	case Activity::Eat:
		return "Eating";
	case Activity::Drink:
		return "Drinking";
	case Activity::Sleep:
		return "Sleeping";
	case Activity::Poo:
		return "Having a poo";
	case Activity::Puke:
		return "Being sick";
	case Activity::Faint:
		return "Out cold";
	case Activity::Examine:
		return "Looking something over";
	case Activity::PlayWithObject:
		return "Throwing something about";
	case Activity::Hurl:
		return "Hurling something";
	case Activity::PutDown:
		return "Putting something down";
	case Activity::Planned:
		return "Acting on a desire";
	case Activity::None:
	default:
		return "Nothing";
	}
}

std::vector<Step> creature_mind::BeIdle(const Random& random)
{
	std::vector<Step> agenda;
	for (int i = 0; i < k_IdleRepeats; ++i)
	{
		Step wait {.kind = Step::Kind::Wait, .seconds = k_IdleWaitSeconds + RandomFraction(random)};
		wait.face = creature_face::Cue::Idle;
		agenda.push_back(wait);
		auto yawn = Action(animations::k_Tired, true);
		yawn.face = creature_face::Cue::Idle;
		agenda.push_back(yawn);
	}
	return agenda;
}

Step creature_mind::SitDown(const Random& random)
{
	auto sit = Static({animations::k_StartSit, animations::k_Sit, animations::k_EndSit},
	                  static_cast<float>(k_SitSeconds + random(k_SitExtraSeconds)));
	sit.face = creature_face::Cue::Idle;
	return sit;
}

std::vector<Step> creature_mind::Sleep(const Random& random)
{
	std::vector<Step> agenda;
	if (random(k_YawnBeforeSleepLots) == 0)
	{
		agenda.push_back(Action(animations::k_Tired, true));
	}
	auto sleep = Static({animations::k_StartSleep, animations::k_Sleep, animations::k_EndSleep}, 0.0f);
	sleep.untilRested = true;
	sleep.closedEyes = true;
	sleep.effect = Effect::Slept;
	agenda.push_back(sleep);
	agenda.push_back(Action(animations::k_Confused, true));
	return WithFace(std::move(agenda), creature_face::Cue::Grimace);
}

std::vector<Step> creature_mind::Eat(uint32_t food)
{
	return WithFace({PickUp(food),
	                 Object({.kind = ObjectOrder::Kind::Keep, .animation = creature_object_actions::k_ExamineObject}),
	                 Object({.kind = ObjectOrder::Kind::Eat}, Effect::Eat)},
	                creature_face::Cue::Curiosity);
}

std::vector<Step> creature_mind::ExamineByPickingUp(uint32_t object, const Random& random)
{
	const auto keep = creature_object_actions::k_FirstKeepAnimation +
	                  random(static_cast<uint32_t>(creature_object_actions::k_KeepAnimationCount));
	const auto letGo = random(k_PutDownLots) >= k_TossLots ? ObjectOrder::Kind::PutDown : ObjectOrder::Kind::Discard;
	return WithFace({PickUp(object), Object({.kind = ObjectOrder::Kind::Keep, .animation = keep}, Effect::Examined),
	                 Object({.kind = letGo})},
	                creature_face::Cue::Curiosity);
}

std::vector<Step> creature_mind::ThrowAbout(uint32_t object, const Random& random)
{
	constexpr uint32_t k_Degrees = 360;
	const auto angle = static_cast<float>(random(k_Degrees)) * std::numbers::pi_v<float> / 180.0f;
	const auto distance = k_ThrowAroundDistance + static_cast<float>(random(k_ThrowAroundExtra));
	return WithFace({PickUp(object), Object({.kind = ObjectOrder::Kind::ThrowNearby,
	                                         .point = distance * glm::vec2(std::cos(angle), std::sin(angle))},
	                                        Effect::ThrewAbout)},
	                creature_face::Cue::Playfulness);
}

std::vector<Step> creature_mind::Hurl(uint32_t object, glm::vec2 target, const Random& random)
{
	std::vector<Step> agenda;
	if (random(k_AngryBeforeHurlLots) == 0)
	{
		agenda.push_back(Action(animations::k_Angry, false));
	}
	agenda.push_back(PickUp(object));
	agenda.push_back(Object({.kind = ObjectOrder::Kind::Throw, .point = target}, Effect::Hurled));
	return WithFace(std::move(agenda), creature_face::Cue::Anger);
}

std::vector<Step> creature_mind::PutDownHeld()
{
	return {Object({.kind = ObjectOrder::Kind::PutDown})};
}

std::vector<Step> creature_mind::Drink(glm::vec2 shore, glm::vec2 water)
{
	auto drink = Action(animations::k_Drink, false);
	drink.effect = Effect::Drink;
	return {{.kind = Step::Kind::Move,
	         .seconds = 0.0f,
	         .animation = 0,
	         .sleepyEyes = false,
	         .movement = {.kind = Movement::Kind::ToPoint,
	                      .point = shore,
	                      .object = std::nullopt,
	                      .run = false,
	                      .minDistance = 0.0f,
	                      .maxDistance = k_DrinkReach}},
	        {.kind = Step::Kind::Move,
	         .seconds = 0.0f,
	         .animation = 0,
	         .sleepyEyes = false,
	         .movement = {.kind = Movement::Kind::TurnToFace,
	                      .point = water,
	                      .object = std::nullopt,
	                      .run = false,
	                      .minDistance = 0.0f,
	                      .maxDistance = 0.0f}},
	        drink};
}

std::vector<Step> creature_mind::Poo(const Random& random)
{
	std::vector<Step> agenda;
	if (random(k_ShowPooLots) == 0)
	{
		agenda.push_back(Action(animations::k_NeedAPoo, false));
	}
	auto poo = Static({animations::k_StartPoo, animations::k_Poo, animations::k_EndPoo}, k_PooSeconds);
	poo.effect = Effect::Poo;
	agenda.push_back(poo);
	return WithFace(std::move(agenda), creature_face::Cue::Smile);
}

std::vector<Step> creature_mind::Puke()
{
	auto puke = Static({animations::k_StartPuke, animations::k_Puke, animations::k_EndPuke}, k_PukeSeconds);
	puke.effect = Effect::Puke;
	return WithFace({puke}, creature_face::Cue::Grimace);
}

std::vector<Step> creature_mind::Faint()
{
	auto faint = Static({animations::k_Faint, animations::k_Faint, animations::k_GetUp}, k_FaintSeconds);
	faint.holdLoop = true;
	faint.closedEyes = true;
	faint.effect = Effect::CameRound;
	return {faint};
}

std::vector<Step> creature_mind::EatHeld()
{
	return WithFace({Object({.kind = ObjectOrder::Kind::Eat}, Effect::Eat)}, creature_face::Cue::Curiosity);
}

std::vector<Step> creature_mind::Emote(size_t animation)
{
	return WithFace({Action(animation, false)}, creature_face::Cue::AttitudeToPlayer);
}

std::vector<Step> creature_mind::FaceAndEmote(glm::vec2 point, size_t animation)
{
	Step turn {.kind = Step::Kind::Move};
	turn.movement = {.kind = Movement::Kind::TurnToFace, .point = point};
	return WithFace({turn, Action(animation, false)}, creature_face::Cue::AttitudeToPlayer);
}

std::vector<Step> creature_mind::ApproachAndEmote(uint32_t object, size_t animation)
{
	Step approach {.kind = Step::Kind::Move};
	approach.movement = {.kind = Movement::Kind::ToObject, .object = object, .maxDistance = k_ApproachDistance};
	return WithFace({approach, Action(animation, false)}, creature_face::Cue::AttitudeToPlayer);
}

std::vector<Step> creature_mind::LookAt(glm::vec2 point, float seconds)
{
	Step turn {.kind = Step::Kind::Move};
	turn.movement = {.kind = Movement::Kind::TurnToFace, .point = point};
	return WithFace({turn, {.kind = Step::Kind::Wait, .seconds = seconds}}, creature_face::Cue::Amazed);
}

std::vector<Step> creature_mind::FollowFor(uint32_t object, float seconds)
{
	Step follow {.kind = Step::Kind::Move, .seconds = seconds};
	follow.movement = {.kind = Movement::Kind::Follow, .object = object, .maxDistance = k_FollowDistance};
	return WithFace({follow}, creature_face::Cue::Amazed);
}

std::vector<Step> creature_mind::RunFrom(glm::vec2 point)
{
	Step flee {.kind = Step::Kind::Move};
	flee.movement = {.kind = Movement::Kind::FleeFrom, .point = point, .run = true};
	return WithFace({flee}, creature_face::Cue::Fear);
}

std::vector<Step> creature_mind::DestroyThing(uint32_t object)
{
	return WithFace({Object({.kind = ObjectOrder::Kind::Destroy, .object = object})}, creature_face::Cue::Anger);
}

std::vector<Step> creature_mind::LookAbout(float seconds)
{
	return WithFace({{.kind = Step::Kind::Wait, .seconds = seconds}}, creature_face::Cue::Idle);
}

std::optional<NeedPlan> creature_mind::ChooseNeed(const Wants& wants, const Random& random)
{
	struct Need
	{
		Activity activity;
		float value;
		bool possible;
	};
	const std::array<Need, 4> needs {{
	    {.activity = Activity::Eat, .value = wants.hunger, .possible = wants.food.has_value() && !wants.holding},
	    {.activity = Activity::Sleep, .value = wants.tiredness, .possible = true},
	    {.activity = Activity::Poo, .value = wants.poo, .possible = true},
	    {.activity = Activity::Drink, .value = wants.water, .possible = wants.waterSpot.has_value()},
	}};
	const Need* strongest = nullptr;
	for (const auto& need : needs)
	{
		if (need.possible && need.value >= k_ActOnNeed && (strongest == nullptr || need.value > strongest->value))
		{
			strongest = &need;
		}
	}
	if (strongest == nullptr)
	{
		return std::nullopt;
	}
	switch (strongest->activity)
	{
	case Activity::Eat:
		return NeedPlan {.activity = Activity::Eat, .agenda = Eat(*wants.food)};
	case Activity::Drink:
		return NeedPlan {.activity = Activity::Drink, .agenda = Drink(wants.waterSpot->shore, wants.waterSpot->water)};
	case Activity::Poo:
		return NeedPlan {.activity = Activity::Poo, .agenda = Poo(random)};
	case Activity::Sleep:
	default:
		return NeedPlan {.activity = Activity::Sleep, .agenda = Sleep(random)};
	}
}

std::optional<NeedPlan> creature_mind::ChooseObjectActivity(const Wants& wants, const Random& random)
{
	if (wants.holding)
	{
		return NeedPlan {.activity = Activity::PutDown, .agenda = PutDownHeld()};
	}
	if (!wants.object.has_value())
	{
		return std::nullopt;
	}
	struct Desire
	{
		Activity activity;
		float value;
		bool possible;
	};
	const std::array<Desire, 3> desires {{
	    {.activity = Activity::Examine, .value = wants.curiosity, .possible = true},
	    {.activity = Activity::PlayWithObject, .value = wants.play, .possible = true},
	    {.activity = Activity::Hurl, .value = wants.anger, .possible = wants.hurlTarget.has_value()},
	}};
	const Desire* strongest = nullptr;
	for (const auto& desire : desires)
	{
		if (desire.possible && desire.value >= k_ActOnDesire && (strongest == nullptr || desire.value > strongest->value))
		{
			strongest = &desire;
		}
	}
	if (strongest == nullptr)
	{
		return std::nullopt;
	}
	switch (strongest->activity)
	{
	case Activity::PlayWithObject:
		return NeedPlan {.activity = Activity::PlayWithObject, .agenda = ThrowAbout(*wants.object, random)};
	case Activity::Hurl:
		return NeedPlan {.activity = Activity::Hurl, .agenda = Hurl(*wants.object, *wants.hurlTarget, random)};
	case Activity::Examine:
	default:
		return NeedPlan {.activity = Activity::Examine, .agenda = ExamineByPickingUp(*wants.object, random)};
	}
}

namespace
{
const Step* CurrentStep(const IdleMind& mind)
{
	return mind.stepStarted && mind.step < mind.agenda.size() ? &mind.agenda[mind.step] : nullptr;
}
} // namespace

bool creature_mind::IsAsleep(const IdleMind& mind)
{
	const auto* step = CurrentStep(mind);
	return step != nullptr && step->kind == Step::Kind::Static && step->untilRested;
}

bool creature_mind::IsUnconscious(const IdleMind& mind)
{
	const auto* step = CurrentStep(mind);
	return step != nullptr && step->kind == Step::Kind::Static && step->holdLoop;
}

std::vector<Step> creature_mind::HangAround(const Random& random)
{
	constexpr uint32_t k_Degrees = 360;
	const auto angle = static_cast<float>(random(k_Degrees)) * std::numbers::pi_v<float> / 180.0f;
	const auto distance = k_HangAroundDistance + static_cast<float>(random(k_HangAroundExtra));
	std::vector<Step> agenda {{.kind = Step::Kind::Move,
	                           .seconds = 0.0f,
	                           .animation = 0,
	                           .sleepyEyes = false,
	                           .movement = {.kind = Movement::Kind::Nearby,
	                                        .point = distance * glm::vec2(std::cos(angle), std::sin(angle)),
	                                        .object = std::nullopt,
	                                        .run = false,
	                                        .minDistance = 0.0f,
	                                        .maxDistance = 1.0f}},
	                          SitDown(random)};
	return WithFace(std::move(agenda), creature_face::Cue::Idle);
}

void creature_mind::Plan(IdleMind& mind, Activity activity, std::vector<Step> agenda)
{
	mind.activity = activity;
	mind.agenda = std::move(agenda);
	++mind.serial;
	mind.step = 0;
	mind.stepStarted = false;
	mind.stepSeconds = 0.0f;
	mind.sitEnding = false;
	mind.wakeWanted = false;
	mind.gaveUp = false;
}

void creature_mind::ChooseNext(IdleMind& mind, const Senses& senses, const Random& random)
{
	if (auto need = ChooseNeed(senses.wants, random))
	{
		Plan(mind, need->activity, std::move(need->agenda));
		return;
	}
	if (senses.wants.holding)
	{
		Plan(mind, Activity::PutDown, PutDownHeld());
		return;
	}
	if (mind.showDesireSeconds <= 0.0f)
	{
		std::optional<size_t> emote;
		if (senses.feedbackSeconds.has_value() && *senses.feedbackSeconds < k_FeedbackSeconds)
		{
			emote = senses.feedbackWasStroke ? animations::k_Happy : animations::k_Sad;
			mind.shown.reset();
		}
		else if (senses.strongest.has_value())
		{
			emote = creature_desires::EmoteFor(*senses.strongest);
			mind.shown = senses.strongest;
		}
		if (emote.has_value())
		{
			// Showing the player how it feels, its face shows what it thinks of them
			Plan(mind, Activity::ShowDesire, Emote(*emote));
			mind.showDesireSeconds = k_ShowDesireSeconds;
			return;
		}
	}
	if (auto activity = ChooseObjectActivity(senses.wants, random))
	{
		Plan(mind, activity->activity, std::move(activity->agenda));
		return;
	}
	const auto lot = random(k_ActivityLots);
	if (lot == 0)
	{
		Plan(mind, Activity::Sit, {SitDown(random)});
	}
	else if (lot == k_ActivityLots - 1)
	{
		Plan(mind, Activity::HangAround, HangAround(random));
	}
	else
	{
		Plan(mind, Activity::BeIdle, BeIdle(random));
	}
}

std::optional<creature_face::Request> creature_mind::PullFace(IdleMind& mind, creature_face::Cue cue, const Senses& senses,
                                                              const Random& random)
{
	mind.faceSeconds = k_FaceRepeatSeconds;
	auto feelings = senses.feelings;
	feelings.variety = mind.faceVariety;
	feelings.idlePick = random(creature_face::k_IdleFaceCount);
	const auto face = creature_face::Choose(cue, feelings);
	if (face.has_value())
	{
		mind.faceVariety += random(k_FaceVarietyStep);
	}
	return face;
}

Commands creature_mind::Think(IdleMind& mind, const Senses& senses, const Random& random)
{
	Commands commands;
	mind.showDesireSeconds = std::max(mind.showDesireSeconds - senses.seconds, 0.0f);
	if (mind.step >= mind.agenda.size())
	{
		ChooseNext(mind, senses, random);
	}
	if (mind.step >= mind.agenda.size())
	{
		return commands;
	}
	const auto& step = mind.agenda[mind.step];
	const bool sitting = step.kind == Step::Kind::Static && mind.stepStarted;
	// A step that goes on pulls its face again every few seconds
	if (mind.stepStarted && step.face != creature_face::Cue::None)
	{
		mind.faceSeconds -= senses.seconds;
		if (mind.faceSeconds < k_FaceDue)
		{
			commands.face = PullFace(mind, step.face, senses, random);
		}
	}
	// A sit nobody is waiting on any more ends
	if (senses.bodyLooping && !sitting)
	{
		commands.endSit = true;
	}

	if (!mind.stepStarted)
	{
		commands.lookAbout = step.kind == Step::Kind::Wait;
		// Nothing starts while the body still plays an action
		if (step.kind != Step::Kind::Wait && senses.bodyBusy)
		{
			return commands;
		}
		mind.stepStarted = true;
		mind.stepSeconds = 0.0f;
		// Each step pulls the face it is meant to as it starts, held for as long as the face goes with
		commands.face = PullFace(mind, step.face, senses, random);
		switch (step.kind)
		{
		case Step::Kind::Action:
			commands.playOnce = step.animation;
			commands.mirrored = random(2) == 1;
			if (step.sleepyEyes)
			{
				commands.eyes = Eyes::Sleepy;
			}
			break;
		case Step::Kind::Static:
			commands.startSequence = step.sequence;
			commands.holdLoop = step.holdLoop;
			if (step.closedEyes)
			{
				commands.eyes = Eyes::Closed;
				// Asleep or out cold, it pulls no face
				commands.face.reset();
				commands.relaxFace = true;
			}
			break;
		case Step::Kind::Move:
			commands.move = step.movement;
			if (step.movement.kind == Movement::Kind::Nearby)
			{
				commands.move->kind = Movement::Kind::ToPoint;
				commands.move->point = senses.position + step.movement.point;
			}
			break;
		case Step::Kind::Gesture:
			commands.gesture = static_cast<uint32_t>(step.animation);
			break;
		case Step::Kind::Cast:
			commands.startSequence = step.sequence;
			mind.castDone = false;
			mind.castTurns = 0;
			break;
		case Step::Kind::Object:
			commands.object = step.order;
			if (step.order.kind == ObjectOrder::Kind::ThrowNearby)
			{
				commands.object->kind = ObjectOrder::Kind::Throw;
				commands.object->point = senses.position + step.order.point;
			}
			break;
		case Step::Kind::Wait:
			break;
		}
		return commands;
	}

	mind.stepSeconds += senses.seconds;
	switch (step.kind)
	{
	case Step::Kind::Wait:
		commands.lookAbout = true;
		if (mind.stepSeconds >= step.seconds)
		{
			FinishStep(mind);
		}
		break;
	case Step::Kind::Action:
		if (!senses.bodyBusy)
		{
			if (step.sleepyEyes)
			{
				commands.eyes = Eyes::Normal;
			}
			if (step.effect != Effect::None)
			{
				commands.effect = step.effect;
			}
			FinishStep(mind);
		}
		break;
	case Step::Kind::Static:
	{
		commands.lookAbout = senses.bodyLooping && !step.closedEyes;
		const bool done = mind.wakeWanted || (step.untilRested ? senses.rested : mind.stepSeconds >= step.seconds);
		if (!senses.bodyBusy)
		{
			if (step.closedEyes)
			{
				commands.eyes = Eyes::Normal;
			}
			// The body played the whole sequence before its time was up: it still takes effect
			if (!mind.sitEnding)
			{
				commands.effect = step.effect;
			}
			FinishStep(mind);
		}
		else if (!mind.sitEnding && done)
		{
			commands.endSit = true;
			commands.effect = step.effect;
			mind.sitEnding = true;
			mind.wakeWanted = false;
		}
		break;
	}
	case Step::Kind::Object:
		if (senses.hands == HandsState::Done)
		{
			commands.effect = step.effect;
			FinishStep(mind);
		}
		else if (senses.hands != HandsState::Busy)
		{
			// It couldn't, or was stopped: the rest of the agenda is no use without it
			mind.step = mind.agenda.size();
			mind.stepStarted = false;
			mind.gaveUp = true;
		}
		break;
	case Step::Kind::Cast:
		if (!mind.castDone)
		{
			// The miracle is cast as the loop of the pose begins, and held for whole turns from the next
			if (senses.bodyLooping)
			{
				commands.cast = step.cast;
				mind.castDone = true;
				const auto turnsPerSecond = static_cast<uint32_t>(std::lround(1.0f / senses.seconds));
				mind.castTurns = static_cast<uint32_t>(step.seconds * static_cast<float>(turnsPerSecond));
			}
			else if (!senses.bodyBusy)
			{
				// The pose ended before it could cast
				mind.step = mind.agenda.size();
				mind.stepStarted = false;
				mind.gaveUp = true;
			}
		}
		else if (mind.castTurns > 0)
		{
			--mind.castTurns;
		}
		else
		{
			// Its time up, it lets the miracle go and ends the pose, done once the end has played
			if (!mind.sitEnding)
			{
				commands.releaseCast = true;
				commands.endSit = true;
				mind.sitEnding = true;
			}
			if (!senses.bodyBusy)
			{
				FinishStep(mind);
			}
		}
		break;
	case Step::Kind::Gesture:
	case Step::Kind::Move:
		if (step.kind == Step::Kind::Gesture || IsSubMove(step.movement.kind))
		{
			if (senses.subMove == SubMove::Done)
			{
				FinishStep(mind);
			}
			else if (senses.subMove == SubMove::Failed)
			{
				mind.step = mind.agenda.size();
				mind.stepStarted = false;
				mind.gaveUp = true;
			}
			break;
		}
		// Done once it has arrived or given up, or its time is up
		if (!senses.moving)
		{
			FinishStep(mind);
		}
		else if (step.seconds > 0.0f && mind.stepSeconds >= step.seconds)
		{
			commands.stopMoving = true;
			FinishStep(mind);
		}
		break;
	}
	return commands;
}
