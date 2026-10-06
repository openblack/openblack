/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureObjectActions.h"

using namespace openblack;
using namespace openblack::creature_object_actions;

std::string_view creature_object_actions::Name(Kind kind)
{
	switch (kind)
	{
	case Kind::PickUp:
		return "Picking up";
	case Kind::PutDown:
		return "Putting down";
	case Kind::Discard:
		return "Tossing away";
	case Kind::Lob:
		return "Lobbing";
	case Kind::Eat:
		return "Eating what it holds";
	case Kind::Keep:
		return "Looking over what it holds";
	case Kind::Throw:
		return "Throwing";
	case Kind::Destroy:
		return "Knocking down";
	case Kind::Point:
		return "Pointing";
	}
	return "Nothing";
}

Hands creature_object_actions::HandsFor(Kind kind)
{
	switch (kind)
	{
	case Kind::PutDown:
	case Kind::Discard:
	case Kind::Lob:
	case Kind::Eat:
	case Kind::Keep:
	case Kind::Throw:
		return Hands::Holding;
	case Kind::PickUp:
	case Kind::Destroy:
		return Hands::Empty;
	case Kind::Point:
		return Hands::Either;
	}
	return Hands::Either;
}

std::string_view creature_object_actions::Name(TownAttitude attitude)
{
	switch (attitude)
	{
	case TownAttitude::Fear:
		return "fear";
	case TownAttitude::Respect:
		return "respect";
	case TownAttitude::None:
		break;
	}
	return "nothing much";
}

TownAttitude creature_object_actions::AttitudeTo(Kind kind, bool targetIsVillager, bool holdingVillager)
{
	if (holdingVillager)
	{
		return TownAttitude::Fear;
	}
	switch (kind)
	{
	case Kind::Eat:
		return targetIsVillager ? TownAttitude::Fear : TownAttitude::None;
	case Kind::Throw:
	case Kind::Destroy:
		return TownAttitude::Fear;
	case Kind::PickUp:
	case Kind::PutDown:
	case Kind::Discard:
	case Kind::Lob:
	case Kind::Keep:
	case Kind::Point:
		break;
	}
	return TownAttitude::None;
}

float creature_object_actions::AttitudeSeconds(TownAttitude attitude)
{
	switch (attitude)
	{
	case TownAttitude::Fear:
		return 30.0f;
	case TownAttitude::Respect:
		return 10.0f;
	case TownAttitude::None:
		break;
	}
	return 0.0f;
}
