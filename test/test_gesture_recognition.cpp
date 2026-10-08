/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <algorithm>
#include <filesystem>
#include <numbers>
#include <vector>

#include <GestureFile.h>
#include <gtest/gtest.h>

#include "Gestures/GestureMatcher.h"
#include "Gestures/GesturePaths.h"
#include "Gestures/GestureRecorder.h"
#include "Gestures/GestureRequests.h"

using namespace openblack;
using namespace openblack::gesture;

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;
constexpr float k_ScreenAspect = 4.0f / 3.0f;

GestureRecorder Record(std::span<const glm::vec2> path)
{
	GestureRecorder recorder;
	for (const auto& point : path)
	{
		recorder.AddPoint(point, glm::vec3(point.x, 0.0f, point.y));
	}
	return recorder;
}

/// Templates made the way the game's were: by recording a drawing and keeping its key points
std::vector<gestures::GestureTemplate> FakeTemplates()
{
	std::vector<gestures::GestureTemplate> templates;
	const auto circle = Record(TraceCircle({300.0f, 300.0f}, 120.0f, true));
	templates.push_back(MakeTemplate(GestureType::Circle, circle.KeyPoints(), false, true, false, k_ScreenAspect));
	const auto scribble = Record(TraceScribble({300.0f, 300.0f}, 300.0f, 5));
	templates.push_back(MakeTemplate(GestureType::Scribble, scribble.KeyPoints(), true, false, true, k_ScreenAspect));
	const std::array<glm::vec2, 4> fork {glm::vec2 {100.0f, 400.0f}, {100.0f, 100.0f}, {300.0f, 100.0f}, {300.0f, 400.0f}};
	templates.push_back(
	    MakeTemplate(GestureType::ForkDown, Record(Trace(fork)).KeyPoints(), true, false, true, k_ScreenAspect));
	return templates;
}

std::optional<gestures::GestureFile> GameTemplates()
{
	const char* gamePath = std::getenv("OPENBLACK_GAME_PATH");
	if (gamePath == nullptr)
	{
		return std::nullopt;
	}
	gestures::GestureFile file;
	if (file.Open(std::filesystem::path(gamePath) / "Data" / "Gestures.jty") != gestures::GestureFileResult::Success)
	{
		return std::nullopt;
	}
	return file;
}
} // namespace

TEST(GestureAngles, HeadingsTurnsAndDirections)
{
	EXPECT_FLOAT_EQ(HeadingOf({1.0f, 0.0f}), 0.0f);
	EXPECT_NEAR(HeadingOf({0.0f, -1.0f}), 1.5f * k_Pi, 1e-5f);
	EXPECT_NEAR(TurnBetween(0.1f, 2.0f * k_Pi - 0.1f), -0.2f, 1e-5f);
	EXPECT_NEAR(TurnBetween(2.0f * k_Pi - 0.1f, 0.1f), 0.2f, 1e-5f);
	// Up the screen is 0, right 2, down 4, left 6
	EXPECT_EQ(DirectionOf(HeadingOf({0.0f, -1.0f})), 0);
	EXPECT_EQ(DirectionOf(HeadingOf({1.0f, 0.0f})), 2);
	EXPECT_EQ(DirectionOf(HeadingOf({0.0f, 1.0f})), 4);
	EXPECT_EQ(DirectionOf(HeadingOf({-1.0f, 0.0f})), 6);
	EXPECT_EQ(DirectionOf(HeadingOf({1.0f, -1.0f})), 1);
}

TEST(GestureRecorder, AStraightStrokeHasNoCorners)
{
	const std::array<glm::vec2, 2> stroke {glm::vec2 {100.0f, 100.0f}, {500.0f, 100.0f}};
	const auto recorder = Record(Trace(stroke));
	const auto keys = recorder.KeyPoints();
	ASSERT_EQ(keys.size(), 2u);
	EXPECT_EQ(keys.front().screen, glm::vec2(100.0f, 100.0f));
	EXPECT_EQ(keys.front().direction, 2);
}

TEST(GestureRecorder, ARightAngleIsACorner)
{
	const std::array<glm::vec2, 3> stroke {glm::vec2 {100.0f, 100.0f}, {400.0f, 100.0f}, {400.0f, 400.0f}};
	const auto recorder = Record(Trace(stroke));
	const auto keys = recorder.KeyPoints();
	ASSERT_EQ(keys.size(), 3u);
	EXPECT_NEAR(keys[1].screen.x, 400.0f, 1.0f);
	EXPECT_NEAR(keys[1].screen.y, 100.0f, 1.0f);
	// Turning right on the screen, y down, is a positive turn
	EXPECT_NEAR(keys[1].turn, k_Pi / 2.0f, 0.05f);
	EXPECT_EQ(keys[1].direction, 4);
}

TEST(GestureRecorder, KeepsTheLastEightyPoints)
{
	GestureRecorder recorder;
	for (int i = 0; i < 200; ++i)
	{
		recorder.AddPoint({static_cast<float>(i * 5), 100.0f}, glm::vec3(0.0f));
	}
	EXPECT_EQ(recorder.Count(), k_Capacity);
	EXPECT_EQ(recorder.At(0).screen.x, 600.0f);
	EXPECT_EQ(recorder.At(0).key, KeyType::Start);
	EXPECT_EQ(recorder.At(k_Capacity - 1).key, KeyType::End);
}

TEST(GestureRecorder, HeldStillItForgetsThePath)
{
	GestureRecorder recorder;
	recorder.AddPoint({10.0f, 10.0f}, glm::vec3(0.0f));
	recorder.AddPoint({50.0f, 10.0f}, glm::vec3(0.0f));
	for (uint32_t i = 0; i < k_StillSamples + 2; ++i)
	{
		recorder.AddPoint({90.0f, 10.0f}, glm::vec3(0.0f));
	}
	EXPECT_LT(recorder.Count(), 5u);
	EXPECT_EQ(recorder.At(0).screen, glm::vec2(90.0f, 10.0f));
}

TEST(GestureRecorder, OverNothingTheLastLandPointStandsIn)
{
	GestureRecorder recorder;
	recorder.AddPoint({10.0f, 10.0f});
	EXPECT_TRUE(recorder.Empty());
	recorder.AddPoint({10.0f, 10.0f}, {1.0f, 2.0f, 3.0f});
	recorder.AddPoint({40.0f, 10.0f});
	EXPECT_EQ(recorder.At(1).world, glm::vec3(1.0f, 2.0f, 3.0f));
}

TEST(GestureMatcher, AspectBands)
{
	EXPECT_TRUE(AspectFits(0.1f, 0.05f));
	EXPECT_FALSE(AspectFits(0.1f, 0.8f));
	EXPECT_TRUE(AspectFits(1.0f, 0.8f));
	EXPECT_FALSE(AspectFits(1.0f, 9.0f));
	EXPECT_TRUE(AspectFits(9.0f, 0.8f));
	EXPECT_FALSE(AspectFits(9.0f, 0.05f));
	// A square on a 4:3 screen is three quarters wide against tall, as the game measures it
	EXPECT_NEAR(AspectOf({.min = {0.0f, 0.0f}, .max = {99.0f, 99.0f}}, k_ScreenAspect), 0.75f, 1e-5f);
}

TEST(GestureMatcher, RecognisesACircleAnywhereAtAnySize)
{
	const auto templates = FakeTemplates();
	const auto drawn = Record(TraceCircle({700.0f, 400.0f}, 200.0f, true));
	const auto match = Recognise(templates, GestureType::Circle, drawn.KeyPoints(), k_ScreenAspect);
	ASSERT_TRUE(match.has_value());
	EXPECT_EQ(match->templateIndex, 0u);
	EXPECT_FALSE(match->mirrored);
	EXPECT_LE(match->largestDifference, k_TurnTolerance);
	EXPECT_FALSE(Recognise(templates, GestureType::Scribble, drawn.KeyPoints(), k_ScreenAspect).has_value());
}

TEST(GestureMatcher, ACircleTheOtherWayRoundIsItsMirrorImage)
{
	auto templates = FakeTemplates();
	const auto drawn = Record(TraceCircle({500.0f, 400.0f}, 150.0f, false));
	const auto match = Recognise(templates, GestureType::Circle, drawn.KeyPoints(), k_ScreenAspect);
	ASSERT_TRUE(match.has_value());
	EXPECT_TRUE(match->mirrored);
	templates.front().allowMirror = 0;
	EXPECT_FALSE(Recognise(templates, GestureType::Circle, drawn.KeyPoints(), k_ScreenAspect).has_value());
}

TEST(GestureMatcher, RecognisesAScribbleButNotATallOne)
{
	const auto templates = FakeTemplates();
	const auto wide = Record(TraceScribble({400.0f, 300.0f}, 250.0f, 5));
	EXPECT_TRUE(Recognise(templates, GestureType::Scribble, wide.KeyPoints(), k_ScreenAspect).has_value());
	// The same strokes up and down set off another way and are too tall
	std::vector<glm::vec2> corners;
	for (int i = 0; i <= 5; ++i)
	{
		corners.emplace_back(400.0f + static_cast<float>(i) * 2.0f, i % 2 == 0 ? 200.0f : 450.0f);
	}
	const auto tall = Record(Trace(corners));
	EXPECT_FALSE(Recognise(templates, GestureType::Scribble, tall.KeyPoints(), k_ScreenAspect).has_value());
}

TEST(GestureMatcher, ADifferentShapeIsntRecognised)
{
	const auto templates = FakeTemplates();
	const auto scribble = Record(TraceScribble({400.0f, 300.0f}, 250.0f, 5));
	EXPECT_FALSE(Recognise(templates, GestureType::ForkDown, scribble.KeyPoints(), k_ScreenAspect).has_value());
	const std::array<glm::vec2, 4> fork {glm::vec2 {500.0f, 500.0f}, {500.0f, 300.0f}, {650.0f, 300.0f}, {650.0f, 500.0f}};
	EXPECT_TRUE(Recognise(templates, GestureType::ForkDown, Record(Trace(fork)).KeyPoints(), k_ScreenAspect).has_value());
	const auto closest = ClosestMatch(templates, Record(Trace(fork)).KeyPoints(), k_ScreenAspect);
	ASSERT_TRUE(closest.has_value());
	EXPECT_EQ(closest->templateIndex, 2u);
}

TEST(GestureMatcher, ACirclesPlaceAndSize)
{
	const auto templates = FakeTemplates();
	const auto drawn = Record(TraceCircle({400.0f, 300.0f}, 100.0f, true));
	const auto match = Recognise(templates, GestureType::Circle, drawn.KeyPoints(), k_ScreenAspect);
	ASSERT_TRUE(match.has_value());
	const auto box = GestureBox(drawn, *match);
	EXPECT_NEAR(box.Centre().x, 400.0f, 15.0f);
	EXPECT_NEAR(box.Centre().y, 300.0f, 15.0f);
	const auto edge = CircleEdge(box);
	EXPECT_NEAR(edge.x - box.Centre().x, 100.0f, 15.0f);
	EXPECT_FLOAT_EQ(edge.y, box.Centre().y);
	// Measured across the camera's view only, a little enlarged
	EXPECT_FLOAT_EQ(CircleRadius({0.0f, 0.0f, 0.0f}, {10.0f, 3.0f, 7.0f}, {1.0f, 0.2f, 0.0f}), 10.5f);
}

TEST(GestureRequests, NothingToDoNothingWaitedFor)
{
	EXPECT_TRUE(Requests({}).empty());
}

TEST(GestureRequests, AStormSeedWaitsForACircleThenPowersUp)
{
	HandContext context;
	context.seed = HandContext::Seed {.sizingGesture = GestureType::Circle,
	                                  .powerUpGestures = {GestureType::InverseSpiral, GestureType::Spiral, GestureType::None}};
	// Without the Action button held, the storm isn't readied to be sized
	EXPECT_EQ(Requests(context).size(), 3u);
	context.actionHeld = true;
	const auto requests = Requests(context);
	ASSERT_EQ(requests.size(), 4u);
	EXPECT_EQ(requests[0], (Request {.gesture = GestureType::Circle, .purpose = Purpose::SizeCircle}));
	EXPECT_EQ(requests[1], (Request {.gesture = GestureType::Scribble, .purpose = Purpose::DropSeed}));
	EXPECT_EQ(requests[2], (Request {.gesture = GestureType::InverseSpiral, .purpose = Purpose::PowerUp, .powerUpLevel = 0}));
	EXPECT_EQ(requests[3], (Request {.gesture = GestureType::Spiral, .purpose = Purpose::PowerUp, .powerUpLevel = 1}));

	// With a circle remembered and the seed at level 0, only the other level and the scribble
	context.circleRemembered = true;
	context.seed->powerUp = 0;
	const auto later = Requests(context);
	ASSERT_EQ(later.size(), 2u);
	EXPECT_EQ(later[1].powerUpLevel, 1);
}

TEST(GestureRequests, ASeedThatCantPowerUpIsStillDroppedByAScribble)
{
	HandContext context;
	context.seed = HandContext::Seed {.ready = false, .powerUpGestures = {GestureType::Spiral}};
	const auto requests = Requests(context);
	ASSERT_EQ(requests.size(), 1u);
	EXPECT_EQ(requests[0].purpose, Purpose::DropSeed);
}

TEST(GestureRequests, OutOfInfluenceOnlyASeedBeingPoweredUpIsScribbledAway)
{
	HandContext context;
	context.inInfluence = false;
	// A seed from a globe can't be powered up, and outside the player's influence it can't be dropped either
	context.seed = HandContext::Seed {.powerUpGestures = {GestureType::Spiral}, .canPowerUp = false};
	EXPECT_TRUE(Requests(context).empty());
	// A seed being powered up is scribbled away wherever the hand is
	context.seed->canPowerUp = true;
	const auto requests = Requests(context);
	ASSERT_FALSE(requests.empty());
	EXPECT_EQ(requests[0].purpose, Purpose::DropSeed);
}

TEST(GestureRequests, AThingInTheHandIsShakenOutOnlyInInfluence)
{
	HandContext context;
	context.holdingObject = true;
	// A leash in the hand isn't shaken off while the hand holds a thing: the thing is
	context.creature = HandContext::Creature {.leashed = true};
	auto requests = Requests(context);
	ASSERT_EQ(requests.size(), 1u);
	EXPECT_EQ(requests[0], (Request {.gesture = GestureType::Scribble, .purpose = Purpose::ShakeOffHeld}));
	EXPECT_FALSE(ShowsRecognition(Purpose::ShakeOffHeld));
	context.inInfluence = false;
	EXPECT_TRUE(Requests(context).empty());
}

TEST(GestureRequests, ACircleIsWaitedForWhetherOrNotTheSeedIsReadyOrCast)
{
	HandContext context;
	context.actionHeld = true;
	context.seed = HandContext::Seed {.ready = false, .cast = true, .sizingGesture = GestureType::Circle};
	const auto requests = Requests(context);
	ASSERT_FALSE(requests.empty());
	EXPECT_EQ(requests[0].purpose, Purpose::SizeCircle);
}

TEST(GestureRequests, EveryGestureButAScribbleIsShownRecognised)
{
	EXPECT_TRUE(ShowsRecognition(Purpose::SizeCircle));
	EXPECT_TRUE(ShowsRecognition(Purpose::PowerUp));
	EXPECT_TRUE(ShowsRecognition(Purpose::LeashGesture));
	EXPECT_TRUE(ShowsRecognition(Purpose::PickLeash));
	EXPECT_FALSE(ShowsRecognition(Purpose::DropSeed));
	EXPECT_FALSE(ShowsRecognition(Purpose::ShakeOffLeash));
	EXPECT_FALSE(ShowsRecognition(Purpose::ClosePicker));
}

TEST(GestureRequests, TheLeash)
{
	HandContext context;
	context.creature = HandContext::Creature {.knows = {false, true, false}};
	// Unleashed, knowing a leash: the leash gesture puts it on
	auto requests = Requests(context);
	ASSERT_EQ(requests.size(), 1u);
	EXPECT_EQ(requests[0], (Request {.gesture = GestureType::SquareSpirial, .purpose = Purpose::LeashGesture}));

	// Held in the hand, knowing one leash: a scribble shakes it off; the leash gesture has nothing to pick
	context.creature->leashed = true;
	context.creature->worn = LeashType::Rope;
	requests = Requests(context);
	ASSERT_EQ(requests.size(), 1u);
	EXPECT_EQ(requests[0].purpose, Purpose::ShakeOffLeash);

	// Tied to something, it stays on
	context.creature->tied = true;
	EXPECT_TRUE(Requests(context).empty());

	// Knowing all three: the leash gesture opens the picker, which offers the other two
	context.creature->tied = false;
	context.creature->knows = {true, true, true};
	requests = Requests(context);
	ASSERT_EQ(requests.size(), 2u);
	EXPECT_EQ(requests[1].purpose, Purpose::LeashGesture);
	context.pickerOpen = true;
	requests = Requests(context);
	ASSERT_EQ(requests.size(), 4u);
	EXPECT_EQ(requests[0], (Request {.gesture = GestureType::Scribble, .purpose = Purpose::ClosePicker}));
	EXPECT_EQ(requests[1],
	          (Request {.gesture = GestureType::VerticalScribble, .purpose = Purpose::PickLeash, .leash = LeashType::Evil}));
	EXPECT_EQ(requests[2], (Request {.gesture = GestureType::Heart, .purpose = Purpose::PickLeash, .leash = LeashType::Good}));
	// The scribble closes the picker before it could shake the leash off
	EXPECT_EQ(requests[3].purpose, Purpose::ShakeOffLeash);

	// A fighting creature can't be leashed by gesture
	context = {};
	context.creature = HandContext::Creature {.fighting = true, .knows = {false, true, false}};
	EXPECT_TRUE(Requests(context).empty());
}

TEST(GestureRequests, TheLeashPickerClosesByItself)
{
	LeashPicker picker;
	picker.Open();
	picker.Update(20.0f, 25.0f, true);
	EXPECT_TRUE(picker.open);
	picker.Update(6.0f, 25.0f, true);
	EXPECT_FALSE(picker.open);
	picker.Open();
	picker.Update(0.1f, 25.0f, false);
	EXPECT_FALSE(picker.open);
}

TEST(GestureMatcher, TheGamesTemplatesRecogniseTheirOwnShapes)
{
	const auto file = GameTemplates();
	if (!file.has_value())
	{
		GTEST_SKIP() << "No gesture templates in the game folder";
	}
	const auto& templates = file->GetTemplates();
	// Every gesture the game uses can be drawn from its own templates, and the shapes the game acts on aren't taken for
	// one another
	for (uint8_t gesture = 1; gesture <= 23; ++gesture)
	{
		const auto path = TraceGesture(templates, static_cast<GestureType>(gesture), {512.0f, 384.0f}, 300.0f, k_ScreenAspect);
		EXPECT_TRUE(path.has_value()) << "gesture " << static_cast<int>(gesture);
	}
	const auto scribble = TraceGesture(templates, GestureType::Scribble, {512.0f, 384.0f}, 300.0f, k_ScreenAspect);
	ASSERT_TRUE(scribble.has_value());
	const auto keys = Record(*scribble).KeyPoints();
	for (const auto other : {GestureType::Circle, GestureType::SquareSpirial, GestureType::Heart, GestureType::EShape,
	                         GestureType::VerticalScribble, GestureType::Spiral, GestureType::InverseSpiral})
	{
		EXPECT_FALSE(Recognise(templates, other, keys, k_ScreenAspect).has_value());
	}
	// A hand-drawn circle, either way round, is the game's circle
	for (const bool clockwise : {true, false})
	{
		const auto circle = Record(TraceCircle({500.0f, 400.0f}, 150.0f, clockwise));
		EXPECT_TRUE(Recognise(templates, GestureType::Circle, circle.KeyPoints(), k_ScreenAspect).has_value());
		EXPECT_FALSE(Recognise(templates, GestureType::Scribble, circle.KeyPoints(), k_ScreenAspect).has_value());
	}
	// And a hand-drawn scribble is the game's scribble
	const auto drawn = Record(TraceScribble({500.0f, 400.0f}, 300.0f, 5));
	EXPECT_TRUE(Recognise(templates, GestureType::Scribble, drawn.KeyPoints(), k_ScreenAspect).has_value());
}
