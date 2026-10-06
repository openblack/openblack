/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureIdleMind.h"

#include <algorithm>

#include "Creature/CreatureLayers.h"

using namespace openblack;
using namespace openblack::creature_mind;
namespace animations = openblack::creature_layers::animations;

namespace
{
/// Random fractions of a second are drawn in this many steps
constexpr uint32_t k_FractionSteps = 1000;

float RandomFraction(const Random& random)
{
	return static_cast<float>(random(k_FractionSteps)) / static_cast<float>(k_FractionSteps);
}

void FinishStep(IdleMind& mind)
{
	++mind.step;
	mind.stepStarted = false;
	mind.stepSeconds = 0.0f;
	mind.sitEnding = false;
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
	case Activity::Told:
		return "Doing as told";
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
		agenda.push_back({.kind = Step::Kind::Wait,
		                  .seconds = k_IdleWaitSeconds + RandomFraction(random),
		                  .animation = 0,
		                  .sleepyEyes = false});
		agenda.push_back({.kind = Step::Kind::Action, .seconds = 0.0f, .animation = animations::k_Tired, .sleepyEyes = true});
	}
	return agenda;
}

Step creature_mind::SitDown(const Random& random)
{
	return {.kind = Step::Kind::Sit,
	        .seconds = static_cast<float>(k_SitSeconds + random(k_SitExtraSeconds)),
	        .animation = animations::k_Sit,
	        .sleepyEyes = false};
}

void creature_mind::Plan(IdleMind& mind, Activity activity, std::vector<Step> agenda)
{
	mind.activity = activity;
	mind.agenda = std::move(agenda);
	mind.step = 0;
	mind.stepStarted = false;
	mind.stepSeconds = 0.0f;
	mind.sitEnding = false;
}

void creature_mind::ChooseNext(IdleMind& mind, const Senses& senses, const Random& random)
{
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
			Plan(mind, Activity::ShowDesire,
			     {{.kind = Step::Kind::Action, .seconds = 0.0f, .animation = *emote, .sleepyEyes = false}});
			mind.showDesireSeconds = k_ShowDesireSeconds;
			return;
		}
	}
	if (random(k_SitOdds) == 0)
	{
		Plan(mind, Activity::Sit, {SitDown(random)});
	}
	else
	{
		Plan(mind, Activity::BeIdle, BeIdle(random));
	}
}

Commands creature_mind::Think(IdleMind& mind, const Senses& senses, const Random& random)
{
	Commands commands;
	mind.showDesireSeconds = std::max(mind.showDesireSeconds - senses.seconds, 0.0f);
	if (mind.faceSeconds > 0.0f)
	{
		mind.faceSeconds -= senses.seconds;
		if (mind.faceSeconds <= 0.0f)
		{
			commands.face = std::optional<size_t> {};
		}
	}

	if (mind.step >= mind.agenda.size())
	{
		ChooseNext(mind, senses, random);
	}
	if (mind.step >= mind.agenda.size())
	{
		return commands;
	}
	const auto& step = mind.agenda[mind.step];
	const bool sitting = step.kind == Step::Kind::Sit && mind.stepStarted;
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
		commands.face = animations::k_FirstFace + random(animations::k_IdleFaceCount);
		mind.faceSeconds = k_FaceSeconds;
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
		case Step::Kind::Sit:
			commands.startSit = true;
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
			FinishStep(mind);
		}
		break;
	case Step::Kind::Sit:
		commands.lookAbout = senses.bodyLooping;
		if (!senses.bodyBusy)
		{
			FinishStep(mind);
		}
		else if (!mind.sitEnding && mind.stepSeconds >= step.seconds)
		{
			commands.endSit = true;
			mind.sitEnding = true;
		}
		break;
	}
	return commands;
}
