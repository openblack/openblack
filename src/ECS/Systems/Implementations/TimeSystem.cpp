/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TimeSystem.h"

using namespace openblack::ecs::systems;

void TimeSystem::Start()
{
	_start = std::chrono::steady_clock::now();
}

void TimeSystem::Update()
{
	auto now = std::chrono::steady_clock::now();
	_elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - _start);
}

std::chrono::milliseconds TimeSystem::GetElapsedTime() const
{
	return _elapsedTime;
}
