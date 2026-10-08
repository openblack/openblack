/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include <gtest/gtest.h>

#include "ECS/Systems/Implementations/GestureEventQueue.h"

using namespace openblack;
using namespace openblack::ecs::systems;

TEST(GestureEventQueue, HandsOutEachEventOnceOldestFirst)
{
	GestureEventQueue queue;
	queue.Inject({.kind = GestureEvent::Kind::Circle,
	              .gesture = GestureType::Circle,
	              .centre = {10.0f, 0.0f, 20.0f},
	              .radius = 30.0f,
	              .powerUpLevel = -1});
	queue.Inject({.kind = GestureEvent::Kind::Scribble, .gesture = GestureType::Scribble});
	const auto events = queue.TakeEvents();
	ASSERT_EQ(events.size(), 2u);
	EXPECT_EQ(events[0].kind, GestureEvent::Kind::Circle);
	EXPECT_FLOAT_EQ(events[0].radius, 30.0f);
	EXPECT_EQ(events[1].kind, GestureEvent::Kind::Scribble);
	EXPECT_TRUE(queue.TakeEvents().empty());
}

TEST(GestureEventQueue, ResetDropsWaitingEvents)
{
	GestureEventQueue queue;
	queue.Inject({.kind = GestureEvent::Kind::PowerUp, .powerUpLevel = 1});
	queue.Reset();
	EXPECT_TRUE(queue.TakeEvents().empty());
}
