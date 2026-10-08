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

#include "ECS/Systems/Implementations/GestureSystem.h"
#include "Gestures/GesturePaths.h"

using namespace openblack;
using namespace openblack::ecs::systems;

namespace
{
GestureSystemInterface::Frame Frame(float seconds)
{
	GestureSystemInterface::Frame frame {
	    .seconds = seconds, .cursor = {100.0f, 100.0f}, .view = {.screenSize = {1024.0f, 768.0f}}};
	// A flat land under the whole screen
	frame.view.landAt = [](glm::vec2 pixel) { return std::optional<glm::vec3>(glm::vec3(pixel.x, 0.0f, pixel.y)); };
	return frame;
}
} // namespace

TEST(GestureSystem, HandsOutEachEventOnce)
{
	GestureSystem system;
	system.Inject({.kind = GestureEvent::Kind::Circle, .gesture = GestureType::Circle, .radius = 20.0f});
	system.Inject({.kind = GestureEvent::Kind::PowerUp, .powerUpLevel = 1});
	const auto events = system.TakeEvents();
	ASSERT_EQ(events.size(), 2u);
	EXPECT_EQ(events[0].kind, GestureEvent::Kind::Circle);
	EXPECT_FLOAT_EQ(events[0].radius, 20.0f);
	EXPECT_EQ(events[1].powerUpLevel, 1);
	EXPECT_TRUE(system.TakeEvents().empty());
	system.Inject({.kind = GestureEvent::Kind::Scribble});
	system.Reset();
	EXPECT_TRUE(system.TakeEvents().empty());
}

TEST(GestureSystem, SamplesTheCursorAtTheGamesPace)
{
	GestureSystem system;
	// Sixty frames a second for a second: about 34 samples
	for (int i = 0; i < 60; ++i)
	{
		auto frame = Frame(1.0f / 60.0f);
		frame.cursor = {100.0f + static_cast<float>(i * 10), 100.0f};
		system.Update(frame);
	}
	EXPECT_GE(system.GetPath().Count(), 28u);
	EXPECT_LE(system.GetPath().Count(), 34u);
	// On a screen 768 high the path is measured in its pixels
	EXPECT_LE(system.GetPath().At(0).screen.x, 120.0f);

	// Off the world, the path is forgotten
	auto away = Frame(0.1f);
	away.overWorld = false;
	system.Update(away);
	EXPECT_TRUE(system.GetPath().Empty());
}

TEST(GestureSystem, ADrawnPathIsTakenInPlaceOfTheCursor)
{
	GestureSystem system;
	const auto path = gesture::TraceScribble({500.0f, 300.0f}, 300.0f, 3);
	system.DrawPath(path, false);
	EXPECT_TRUE(system.IsDrawingPath());
	for (size_t i = 0; i < path.size(); ++i)
	{
		system.Update(Frame(gesture::k_SampleSeconds));
	}
	EXPECT_FALSE(system.IsDrawingPath());
	ASSERT_EQ(system.GetPath().Count(), path.size());
	EXPECT_EQ(system.GetPath().At(0).screen, path.front());
	// Without the game's templates nothing is recognised
	EXPECT_FALSE(system.GetLastRecognised().has_value());
}

TEST(GestureSystem, NothingIsRecordedUntilTheCursorIsOverTheLand)
{
	GestureSystem system;
	auto frame = Frame(gesture::k_SampleSeconds);
	frame.view.landAt = [](glm::vec2 /*pixel*/) { return std::optional<glm::vec3>(); };
	system.Update(frame);
	EXPECT_TRUE(system.GetPath().Empty());
}

TEST(GestureSystem, MovingTheCameraWipesThePathUnlessItShakes)
{
	GestureSystem system;
	int step = 0;
	const auto drawn = [&system, &step](glm::vec3 eye, bool shaking, int samples) {
		for (int i = 0; i < samples; ++i, ++step)
		{
			auto frame = Frame(gesture::k_SampleSeconds);
			frame.cursor = {100.0f + static_cast<float>(step * 20), 100.0f};
			frame.view.cameraEye = eye;
			frame.cameraShaking = shaking;
			system.Update(frame);
		}
	};
	drawn({0.0f, 50.0f, 0.0f}, false, 5);
	EXPECT_EQ(system.GetPath().Count(), 5u);
	// The camera moves while it shakes: the path stays and goes on
	drawn({1.0f, 50.0f, 0.0f}, true, 1);
	EXPECT_EQ(system.GetPath().Count(), 6u);
	// It moves without shaking: the path starts again from this frame's point
	drawn({2.0f, 50.0f, 0.0f}, false, 1);
	EXPECT_EQ(system.GetPath().Count(), 1u);
	// Still, it keeps the path
	drawn({2.0f, 50.0f, 0.0f}, false, 3);
	EXPECT_EQ(system.GetPath().Count(), 4u);
}
