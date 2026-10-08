/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "GestureEventQueue.h"

#include <utility>

using namespace openblack::ecs::systems;

std::vector<GestureEvent> GestureEventQueue::TakeEvents()
{
	return std::exchange(_events, {});
}

void GestureEventQueue::Inject(const GestureEvent& event)
{
	_events.push_back(event);
}

void GestureEventQueue::Reset()
{
	_events.clear();
}
