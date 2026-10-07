/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CastInput.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
void Lock(CastInput& input, bool onObject, uint32_t turn)
{
	input.state = CastInput::State::Locked;
	input.onObject = onObject;
	input.lockTurn = turn;
	input.lastApplyTurn = turn;
}
} // namespace

CastActions magic::Press(CastInput& input, const CastProfile& profile, const PressContext& context)
{
	CastActions actions;
	if (input.state != CastInput::State::Idle)
	{
		return actions;
	}
	// Outside the player's influence the press is taken, and nothing happens
	if (!context.inInfluence)
	{
		return actions;
	}
	// An object under the hand comes first, once the seed is ready
	if (context.seedReady && context.onValidObject)
	{
		switch (profile.castType)
		{
		case SpellCastType::SpellCastHandGesture:
			input.state = CastInput::State::Armed;
			input.onObject = true;
			actions.startHoldLoop = true;
			break;
		case SpellCastType::SpellCastInHand:
			Lock(input, true, context.turn);
			actions.cast = CastTarget::Object;
			break;
		case SpellCastType::SpellCastHandPosition:
			actions.cast = CastTarget::Object;
			break;
		}
		return actions;
	}
	// A seed that isn't ready yet can't be cast anywhere, and one only cast on objects can't be dropped on the land
	if (!context.seedReady)
	{
		actions.notReady = true;
		actions.fail = true;
		return actions;
	}
	if (!context.pointValid || profile.castOnObject)
	{
		actions.fail = true;
		return actions;
	}
	switch (profile.castType)
	{
	case SpellCastType::SpellCastHandGesture:
		input.state = CastInput::State::Armed;
		input.onObject = false;
		actions.startHoldLoop = true;
		break;
	case SpellCastType::SpellCastInHand:
		Lock(input, false, context.turn);
		actions.cast = CastTarget::Point;
		break;
	case SpellCastType::SpellCastHandPosition:
		actions.cast = CastTarget::Point;
		break;
	}
	return actions;
}

CastActions magic::Release(CastInput& input)
{
	CastActions actions;
	switch (input.state)
	{
	case CastInput::State::Idle:
		break;
	case CastInput::State::Armed:
		actions.stopHoldLoop = true;
		actions.cast = input.onObject ? CastTarget::Object : CastTarget::Point;
		break;
	case CastInput::State::Locked:
		actions.unlock = true;
		break;
	}
	input = {};
	return actions;
}

CastActions magic::Tick(CastInput& input, uint32_t turn, bool valid)
{
	CastActions actions;
	if (input.state != CastInput::State::Locked || turn == input.lastApplyTurn || !valid)
	{
		return actions;
	}
	input.lastApplyTurn = turn;
	actions.cast = input.onObject ? CastTarget::Object : CastTarget::Point;
	return actions;
}

CastActions magic::Cancel(CastInput& input)
{
	CastActions actions;
	actions.stopHoldLoop = input.state == CastInput::State::Armed;
	actions.unlock = input.state == CastInput::State::Locked;
	input = {};
	return actions;
}

uint32_t magic::TurnsHeld(const CastInput& input, uint32_t turn)
{
	return input.state == CastInput::State::Locked && turn >= input.lockTurn ? turn - input.lockTurn : 0;
}
