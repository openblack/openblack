/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <glm/gtc/constants.hpp>
#include <gtest/gtest.h>

#include "Camera/CameraDrag.h"

using namespace openblack::camera_drag;

namespace
{
constexpr glm::ivec2 k_Screen {1280, 1024};
}

TEST(CameraDrag, CursorIsMeasuredFromTheMiddle)
{
	const auto middle = NormalisedCursor({640, 512}, k_Screen, k_Screen.y);
	EXPECT_FLOAT_EQ(middle.x, 0.0f);
	EXPECT_FLOAT_EQ(middle.y, 0.0f);
	const auto corner = NormalisedCursor({0, 1024}, k_Screen, k_Screen.y);
	EXPECT_FLOAT_EQ(corner.x, -0.5f);
	EXPECT_FLOAT_EQ(corner.y, 0.5f);
}

TEST(CameraDrag, TheMiddleOffersNoHints)
{
	EXPECT_EQ(IdleTricons({0.0f, 0.0f}, true), tricon::k_Idle);
	EXPECT_EQ(IdleTricons({0.45f, 0.43f}, true), tricon::k_Idle);
}

TEST(CameraDrag, TheSidesAndBottomOfferTurning)
{
	const auto turning = tricon::k_Idle | tricon::k_Rotate | tricon::k_Turning;
	EXPECT_EQ(IdleTricons({0.46f, 0.0f}, true), turning);
	EXPECT_EQ(IdleTricons({-0.46f, 0.0f}, true), turning);
	EXPECT_EQ(IdleTricons({0.0f, 0.44f}, true), turning);
	// The very bottom tilts too
	EXPECT_EQ(IdleTricons({0.0f, 0.495f}, true), turning | tricon::k_Pitch);
}

TEST(CameraDrag, TheTopOffersEverything)
{
	const auto all = tricon::k_Idle | tricon::k_Rotate | tricon::k_Pitch | tricon::k_Top | tricon::k_Turning;
	EXPECT_EQ(IdleTricons({0.0f, -0.495f}, true), all);
	// Over no land the top starts lower down
	EXPECT_EQ(IdleTricons({0.0f, -0.45f}, false), all);
	EXPECT_EQ(IdleTricons({0.0f, -0.45f}, true), tricon::k_Idle);
}

TEST(CameraDrag, PressingAwayFromTheEdgesGripsAtOnce)
{
	DragClassifier drag;
	drag.Start(tricon::k_Idle, {0.1f, 0.1f}, 1000);
	ASSERT_TRUE(drag.GetMode().has_value());
	EXPECT_EQ(*drag.GetMode(), DragMode::Pan);
	EXPECT_EQ(drag.GetTricons(), 0u);
}

TEST(CameraDrag, NothingIsDecidedUntilTheMouseMovesFarEnough)
{
	DragClassifier drag;
	drag.Start(IdleTricons({0.47f, 0.0f}, true), {0.47f, 0.0f}, 1000);
	EXPECT_FALSE(drag.GetMode().has_value());
	EXPECT_EQ(drag.GetTricons(), tricon::k_Rotate | tricon::k_Turning);
	// 20 pixels down a 1280 wide screen is under a fiftieth
	drag.Move({0, 20}, k_Screen, k_Screen.y, 1100, true);
	EXPECT_FALSE(drag.GetMode().has_value());
	drag.Move({0, 10}, k_Screen, k_Screen.y, 1150, true);
	ASSERT_TRUE(drag.GetMode().has_value());
	EXPECT_EQ(*drag.GetMode(), DragMode::EdgeRotate);
	EXPECT_EQ(drag.GetTricons(), tricon::k_Rotate);
}

TEST(CameraDrag, AQuickDragFromTheSideTowardsTheMiddlePans)
{
	DragClassifier quick;
	quick.Start(IdleTricons({0.47f, 0.0f}, true), {0.47f, 0.0f}, 1000);
	quick.Move({-40, 0}, k_Screen, k_Screen.y, 1100, true);
	EXPECT_EQ(quick.GetMode(), DragMode::Pan);

	DragClassifier slow;
	slow.Start(IdleTricons({0.47f, 0.0f}, true), {0.47f, 0.0f}, 1000);
	slow.Move({-40, 0}, k_Screen, k_Screen.y, 1301, true);
	EXPECT_EQ(slow.GetMode(), DragMode::EdgeRotate);

	DragClassifier outwards;
	outwards.Start(IdleTricons({-0.47f, 0.0f}, true), {-0.47f, 0.0f}, 1000);
	outwards.Move({-40, 0}, k_Screen, k_Screen.y, 1100, true);
	EXPECT_EQ(outwards.GetMode(), DragMode::EdgeRotate);
}

TEST(CameraDrag, AtTheTopUpAndDownTiltsAndAcrossTurns)
{
	const auto top = IdleTricons({0.0f, -0.495f}, true);
	DragClassifier upDown;
	upDown.Start(top, {0.0f, -0.495f}, 0);
	upDown.Move({0, -40}, k_Screen, k_Screen.y, 500, true);
	EXPECT_EQ(upDown.GetMode(), DragMode::PitchFromTop);
	EXPECT_EQ(upDown.GetTricons(), tricon::k_Pitch);

	DragClassifier acrossTop;
	acrossTop.Start(top, {0.0f, -0.495f}, 0);
	acrossTop.Move({40, 0}, k_Screen, k_Screen.y, 500, true);
	EXPECT_EQ(acrossTop.GetMode(), DragMode::EdgeRotate);
}

TEST(CameraDrag, AQuickTiltDownOverLandPans)
{
	const auto bottom = IdleTricons({0.0f, 0.495f}, true);
	DragClassifier quick;
	quick.Start(bottom, {0.0f, 0.495f}, 0);
	quick.Move({0, 40}, k_Screen, k_Screen.y, 50, true);
	EXPECT_EQ(quick.GetMode(), DragMode::Pan);

	DragClassifier slow;
	slow.Start(bottom, {0.0f, 0.495f}, 0);
	slow.Move({0, 40}, k_Screen, k_Screen.y, 80, true);
	EXPECT_EQ(slow.GetMode(), DragMode::Pitch);
}

TEST(CameraDrag, EdgeRotateHoldsTheCursorOnTheRing)
{
	// From the middle of the right side, straight in to the middle of the screen
	const auto step = EdgeRotate({640, 512}, {1216, 512}, k_Screen, k_Screen.y);
	// A cursor right on the middle has no direction, and stays in the middle
	EXPECT_EQ(step.cursor, glm::ivec2(640, 512));

	// Far out to the right, it comes in to nine tenths of the half width
	const auto out = EdgeRotate({1280, 512}, {1216, 512}, k_Screen, k_Screen.y);
	EXPECT_EQ(out.cursor, glm::ivec2(1216, 512));
	EXPECT_NEAR(out.angle, 0.0f, 1e-6f);
}

TEST(CameraDrag, EdgeRotateTurnsByTheAngleSweptRoundTheMiddle)
{
	// A quarter of the way round, from the right to the bottom, clockwise on the screen
	const auto step = EdgeRotate({640, 1024}, {1216, 512}, k_Screen, k_Screen.y);
	EXPECT_EQ(step.cursor, glm::ivec2(640, 973));
	EXPECT_NEAR(step.angle, -glm::half_pi<float>(), 1e-5f);
	// Across the top, the turn takes the short way round
	const auto across = EdgeRotate({600, 51}, {680, 51}, k_Screen, k_Screen.y);
	EXPECT_LT(std::abs(across.angle), 0.2f);
}

TEST(CameraDrag, ADragDownTheWholeScreenTiltsBySevenThirdsOfTheFieldOfView)
{
	EXPECT_NEAR(PitchStep(1024, 1024, 1.0f), 2.33333f, 1e-5f);
	EXPECT_NEAR(PitchStep(-512, 1024, 0.9f), -0.5f * 2.33333f * 0.9f, 1e-5f);
}

TEST(CameraDrag, BothButtonsTurnOnlyOnceMovedFarEnoughAcross)
{
	TwoButtonTurn turn;
	EXPECT_EQ(turn.Update(false, 5, 400, 1280), 0);
	// A thirty second of a 1280 wide screen is 32 pixels
	EXPECT_EQ(turn.Update(true, 20, 400, 1280), 0);
	EXPECT_EQ(turn.Update(true, 32, 400, 1280), 0);
	EXPECT_EQ(turn.Update(true, 33, 400, 1280), 33);
	// Once turning, any movement across turns
	EXPECT_EQ(turn.Update(true, 2, 400, 1280), 2);
	// Letting go starts again
	EXPECT_EQ(turn.Update(false, 0, 400, 1280), 0);
	EXPECT_EQ(turn.Update(true, 2, 400, 1280), 0);
	// The cursor having moved that far since the press turns too
	EXPECT_EQ(turn.Update(true, 1, 433, 1280), 1);
}

TEST(CameraDrag, TheCinemaBarsMeasureByA16To9Picture)
{
	EXPECT_EQ(ViewHeight({1280, 1024}, false), 1024);
	// 1024 less what the bars take, 1024 - 1280 * 9 / 16 = 304
	EXPECT_EQ(ViewHeight({1280, 1024}, true), 720);
	// The cursor is measured down from the middle of the screen by the picture's height
	EXPECT_FLOAT_EQ(NormalisedCursor({640, 512 + 360}, {1280, 1024}, 720).y, 0.5f);
	// On a screen as wide as 16:9 or wider the bars take nothing
	EXPECT_EQ(ViewHeight({1920, 1080}, true), 1080);
}
