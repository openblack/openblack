/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CinematicDirectorSystem.h"

#include <utility>

#include "InfoConstants.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;

namespace
{
/// How long the bars take to slide, from the game's info
float WideScreenTime()
{
	return Locator::infoConstants::has_value() ? Locator::infoConstants::value().helpSystem.wideScreenTime : 0.0f;
}
} // namespace

void CinematicDirectorSystem::FadeTo(uint8_t red, uint8_t green, uint8_t blue, int8_t seconds)
{
	_fade.FadeTo(red, green, blue, seconds);
}

void CinematicDirectorSystem::FadeBackToNormal(int8_t seconds)
{
	_fade.FadeBackToNormal(seconds);
}

bool CinematicDirectorSystem::IsFadeFinished() const
{
	return _fade.IsFinished();
}

uint32_t CinematicDirectorSystem::GetFadeColour() const
{
	return _fade.GetColour();
}

void CinematicDirectorSystem::SetWideScreen(bool on, uint32_t owner)
{
	if (on == _bars.IsOn())
	{
		return;
	}
	if (on)
	{
		_hideDialogs = true;
	}
	// The game's own bars leave the interface be; a script's put it away until they go
	_interfaceActive = !on || owner == 0;
	_wideScreenOwner = on ? owner : 0;
	_bars.Set(on, WideScreenTime());
}

bool CinematicDirectorSystem::IsWideScreenOn() const
{
	return _bars.IsOn();
}

uint32_t CinematicDirectorSystem::GetWideScreenOwner() const
{
	return _wideScreenOwner;
}

bool CinematicDirectorSystem::IsInterfaceActive() const
{
	return _interfaceActive;
}

bool CinematicDirectorSystem::TakeHideDialogs()
{
	return std::exchange(_hideDialogs, false);
}

bool CinematicDirectorSystem::IsWideScreenTransitionFinished() const
{
	return _bars.IsTransitionFinished();
}

float CinematicDirectorSystem::GetWideScreenFraction() const
{
	return _bars.GetFraction();
}

void CinematicDirectorSystem::ProcessTurn()
{
	_fade.ProcessTurn();
}

void CinematicDirectorSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	_bars.Update(gameTime.count(), WideScreenTime());
}
