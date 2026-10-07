/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GestureRequests.h"

#include <algorithm>

using namespace openblack;
using namespace openblack::gesture;

std::string_view gesture::Name(Purpose purpose)
{
	switch (purpose)
	{
	case Purpose::SizeCircle:
		return "size the miracle";
	case Purpose::PowerUp:
		return "power up";
	case Purpose::DropSeed:
		return "drop the seed";
	case Purpose::ShakeOffLeash:
		return "shake the leash off";
	case Purpose::LeashGesture:
		return "leash";
	case Purpose::PickLeash:
		return "pick a leash";
	case Purpose::ClosePicker:
		return "close the picker";
	}
	return "";
}

bool gesture::ShowsRecognition(Purpose purpose)
{
	switch (purpose)
	{
	case Purpose::DropSeed:
	case Purpose::ShakeOffLeash:
	case Purpose::ClosePicker:
		return false;
	case Purpose::SizeCircle:
	case Purpose::PowerUp:
	case Purpose::LeashGesture:
	case Purpose::PickLeash:
		return true;
	}
	return false;
}

std::string_view gesture::Name(GestureType gesture)
{
	constexpr std::array<std::string_view, 24> k_Names {
	    "none",
	    "spiral",
	    "inverse spiral",
	    "S",
	    "circle",
	    "scribble",
	    "three",
	    "vertical scribble",
	    "star",
	    "fork right",
	    "fork up",
	    "fork left",
	    "fork down",
	    "heart",
	    "R",
	    "square spiral",
	    "Cyrillic L",
	    "E",
	    "reverse S",
	    "infinity",
	    "W",
	    "house",
	    "inverse square spiral",
	    "square wave",
	};
	const auto index = static_cast<size_t>(gesture);
	return index < k_Names.size() ? k_Names.at(index) : "unknown";
}

int gesture::KnownLeashes(const HandContext::Creature& creature)
{
	return static_cast<int>(std::ranges::count(creature.knows, true));
}

std::vector<Request> gesture::Requests(const HandContext& context)
{
	std::vector<Request> requests;
	const auto& seed = context.seed;

	// A circle for the storm or shield in the hand, drawn while the Action button is held, until one is remembered
	if (context.actionHeld && seed.has_value() && seed->sizingGesture != GestureType::None && !context.circleRemembered)
	{
		requests.push_back({.gesture = seed->sizingGesture, .purpose = Purpose::SizeCircle});
	}

	const bool poweringUp = seed.has_value() && seed->ready && !seed->cast && seed->canPowerUp;
	if (poweringUp)
	{
		// A scribble calls off the power-up asked for, or drops the seed; each other level's gesture powers it up
		requests.push_back({.gesture = GestureType::Scribble, .purpose = Purpose::DropSeed});
		for (int level = 0; level < static_cast<int>(seed->powerUpGestures.size()); ++level)
		{
			const auto gesture = seed->powerUpGestures.at(static_cast<size_t>(level));
			if (gesture != GestureType::None && level != seed->powerUp)
			{
				requests.push_back({.gesture = gesture, .purpose = Purpose::PowerUp, .powerUpLevel = level});
			}
		}
	}
	else if (context.pickerOpen && context.creature.has_value() && context.creature->leashed)
	{
		// The picker: a scribble closes it; each other leash the creature knows is a gesture away
		requests.push_back({.gesture = GestureType::Scribble, .purpose = Purpose::ClosePicker});
		for (size_t i = 0; i < context.creature->knows.size(); ++i)
		{
			const auto leash = static_cast<LeashType>(i + 1);
			const auto gesture = context.leashGestures.at(i + 1);
			if (context.creature->knows.at(i) && leash != context.creature->worn && gesture != GestureType::None)
			{
				requests.push_back({.gesture = gesture, .purpose = Purpose::PickLeash, .leash = leash});
			}
		}
	}

	if (!poweringUp)
	{
		if (!seed.has_value())
		{
			// The empty hand shakes off the leash it holds; one tied to something stays
			if (context.creature.has_value() && context.creature->leashed && !context.creature->tied)
			{
				requests.push_back({.gesture = GestureType::Scribble, .purpose = Purpose::ShakeOffLeash});
			}
		}
		else if (context.inInfluence)
		{
			// Out of the player's influence the seed can't be scribbled away
			requests.push_back({.gesture = GestureType::Scribble, .purpose = Purpose::DropSeed});
		}
	}

	// The leash gesture: on an unleashed creature that knows a leash, or to change the leash of one that knows two
	if (context.creature.has_value() && !context.creature->fighting && !context.pickerOpen &&
	    context.leashGesture != GestureType::None)
	{
		const auto known = KnownLeashes(*context.creature);
		if ((context.creature->leashed && known > 1) || (!context.creature->leashed && known > 0))
		{
			requests.push_back({.gesture = context.leashGesture, .purpose = Purpose::LeashGesture});
		}
	}
	return requests;
}

void LeashPicker::Open()
{
	open = true;
	seconds = 0.0f;
}

void LeashPicker::Close()
{
	open = false;
	seconds = 0.0f;
}

void LeashPicker::Update(float elapsed, float timeoutSeconds, bool leashed)
{
	if (!open)
	{
		return;
	}
	seconds += elapsed;
	if (seconds > timeoutSeconds || !leashed)
	{
		Close();
	}
}
