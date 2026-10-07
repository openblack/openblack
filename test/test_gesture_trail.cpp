/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <algorithm>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Gestures/GestureTrailBuilder.h"
#include "Particles/GestureTrail.h"
#include "Particles/LightSheet.h"

using namespace openblack;
using namespace openblack::particles;
namespace trail = openblack::particles::gesture_trail;

namespace
{
TrailPath Line(float length, int points)
{
	std::vector<glm::vec3> line;
	for (int i = 0; i < points; ++i)
	{
		line.emplace_back(length * static_cast<float>(i) / static_cast<float>(points - 1), 0.0f, 0.0f);
	}
	return TrailPath(line);
}
} // namespace

TEST(GestureTrailPath, WalkedByDistanceAlongIt)
{
	const TrailPath path({{0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 30.0f}});
	EXPECT_FLOAT_EQ(path.Length(), 40.0f);
	EXPECT_EQ(path.At(0.0f), glm::vec3(0.0f));
	EXPECT_EQ(path.At(0.125f), glm::vec3(5.0f, 0.0f, 0.0f));
	EXPECT_EQ(path.At(0.25f), glm::vec3(10.0f, 0.0f, 0.0f));
	EXPECT_EQ(path.At(0.625f), glm::vec3(10.0f, 0.0f, 15.0f));
	EXPECT_EQ(path.At(1.0f), glm::vec3(10.0f, 0.0f, 30.0f));
	// Past its end, its last point
	EXPECT_EQ(path.At(2.0f), glm::vec3(10.0f, 0.0f, 30.0f));
}

TEST(GestureTrailPath, FewerThanThreePointsGiveTheFirst)
{
	const TrailPath path({{1.0f, 2.0f, 3.0f}, {11.0f, 2.0f, 3.0f}});
	EXPECT_EQ(path.At(0.5f), glm::vec3(1.0f, 2.0f, 3.0f));
	EXPECT_EQ(path.At(1.5f), glm::vec3(11.0f, 2.0f, 3.0f));
}

TEST(GestureTrailPath, MovingAPointMeasuresItAgain)
{
	auto path = Line(10.0f, 3);
	path.SetPoint(2, {20.0f, 0.0f, 0.0f});
	EXPECT_FLOAT_EQ(path.Length(), 20.0f);
}

TEST(GestureTrailSymbol, PutInTheUnitSquareAndTakenEvenlyAlongIt)
{
	// A line across the file's square from the left to the right, at its top
	const std::vector<glm::vec3> file {
	    {-100.0f, 0.0f, 100.0f}, {0.0f, 0.0f, 100.0f}, {100.0f, 0.0f, 100.0f}, {100.0f, 0.0f, 100.0f}};
	const auto symbol = gesture::MakeTrailSymbol(file);
	EXPECT_NEAR(symbol.minX, 0.0f, 1e-6f);
	EXPECT_NEAR(symbol.maxX, 1.0f, 1e-6f);
	// The top of the file's square is the top of the screen
	EXPECT_NEAR(symbol.minZ, 0.0f, 1e-6f);
	EXPECT_NEAR(symbol.maxZ, 0.0f, 1e-6f);
	ASSERT_EQ(symbol.points.size(), 4u);
	// A quarter of the way along at a time, the last a step short of its end
	EXPECT_NEAR(symbol.points[0].x, 0.0f, 1e-6f);
	EXPECT_NEAR(symbol.points[1].x, 0.25f, 1e-6f);
	EXPECT_NEAR(symbol.points[3].x, 0.75f, 1e-6f);
}

namespace
{
/// A camera looking straight down the screen's y onto the land: pixel (x, y) is the land point (x, 0, y times a stretch)
gesture::TrailView TopDown(float stretch)
{
	return {.cameraForward = {0.0f, -0.5f, 1.0f}, .pointUnder = [stretch](glm::ivec2 pixel) {
		        return glm::vec3(static_cast<float>(pixel.x), 0.0f, static_cast<float>(pixel.y) * stretch);
	        }};
}

gesture::TrailSymbol Square()
{
	const std::vector<glm::vec3> file {{-100.0f, 0.0f, 100.0f},
	                                   {100.0f, 0.0f, 100.0f},
	                                   {100.0f, 0.0f, -100.0f},
	                                   {-100.0f, 0.0f, -100.0f},
	                                   {-100.0f, 0.0f, 100.0f}};
	return gesture::MakeTrailSymbol(file);
}
} // namespace

TEST(GestureTrailBuild, NeedsTwoPointsOnTheLand)
{
	const std::vector<glm::vec3> one {{5.0f, 0.0f, 5.0f}};
	EXPECT_FALSE(gesture::BuildTrail(one, {.min = {0.0f, 0.0f}, .max = {100.0f, 100.0f}}, Square(), TopDown(1.0f)).has_value());
}

TEST(GestureTrailBuild, TheShapeFitsTheDrawnBoxAndHasAsManyPointsAsThePath)
{
	const std::vector<glm::vec3> drawn {
	    {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}};
	const auto built = gesture::BuildTrail(drawn, {.min = {100.0f, 200.0f}, .max = {300.0f, 400.0f}}, Square(), TopDown(1.0f));
	ASSERT_TRUE(built.has_value());
	EXPECT_EQ(built->drawn.Size(), drawn.size());
	ASSERT_EQ(built->ideal.Size(), drawn.size());
	// The square's corner falls on the box's corner, a pixel truncated
	EXPECT_EQ(built->ideal.Points().front(), glm::vec3(100.0f, 0.0f, 200.0f));
	for (const auto& point : built->ideal.Points())
	{
		EXPECT_GE(point.x, 100.0f);
		EXPECT_LE(point.x, 301.0f);
		EXPECT_GE(point.z, 200.0f);
		EXPECT_LE(point.z, 401.0f);
	}
}

TEST(GestureTrailBuild, ADeepShapeIsSquashedUpTheScreen)
{
	const std::vector<glm::vec3> drawn {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}};
	// The land stretches eight times deeper than wide: squashed by three quarters until no more than twice as deep
	const auto built = gesture::BuildTrail(drawn, {.min = {0.0f, 0.0f}, .max = {200.0f, 200.0f}}, Square(), TopDown(8.0f));
	ASSERT_TRUE(built.has_value());
	float minZ = 1e9f;
	float maxZ = -1e9f;
	float minX = 1e9f;
	float maxX = -1e9f;
	for (const auto& point : built->ideal.Points())
	{
		minZ = std::min(minZ, point.z);
		maxZ = std::max(maxZ, point.z);
		minX = std::min(minX, point.x);
		maxX = std::max(maxX, point.x);
	}
	EXPECT_LE((maxZ - minZ) / (maxX - minX), 2.0f + 1e-3f);
	EXPECT_GT(maxZ - minZ, 0.0f);
}

TEST(GestureTrailMaths, TheTransitionEasesFromTheDrawnPathToTheShape)
{
	EXPECT_FLOAT_EQ(trail::Transition(0.0f, 0.9f), 0.0f);
	EXPECT_FLOAT_EQ(trail::Transition(1.0f, 0.9f), 1.0f);
	// Without a gain, straight
	EXPECT_FLOAT_EQ(trail::Transition(0.3f, 0.0f), 0.3f);
	// Eased in
	EXPECT_LT(trail::Transition(0.2f, 0.9f), 0.2f);
}

TEST(GestureTrailMaths, TheTrailGrowsFromBothEnds)
{
	// Half grown: bright at the ends, nothing in the middle
	EXPECT_EQ(trail::RevealAlpha(0.0f, 0.5f, 176.087f), 88);
	EXPECT_EQ(trail::RevealAlpha(0.5f, 0.5f, 176.087f), 0);
	EXPECT_EQ(trail::RevealAlpha(1.0f, 0.5f, 176.087f), 88);
	// Grown, the whole of it at the most
	EXPECT_EQ(trail::RevealAlpha(0.3f, 1.0f, 176.087f), 176);
}

TEST(GestureTrailMaths, TheWiggleRisesAndFallsThenShrinksOnceDispersed)
{
	EXPECT_FLOAT_EQ(trail::WiggleAmount(0.0f, 1.5f, 3.0f, 2.0f), 1.0f);
	const float age = std::numbers::pi_v<float> / 1.5f;
	EXPECT_NEAR(trail::WiggleAmount(age, 1.5f, 3.0f, 2.0f), 0.0f, 1e-6f);
	// Half way through shrinking
	const float shrinking = 4.0f;
	EXPECT_NEAR(trail::WiggleAmount(shrinking, 1.5f, 3.0f, 2.0f), static_cast<float>((std::cos(4.0 * 1.5) + 1.0) * 0.5) * 0.5f,
	            1e-6f);
	EXPECT_FLOAT_EQ(trail::WiggleAmount(5.5f, 1.5f, 3.0f, 2.0f), 0.0f);
}

TEST(GestureTrailMaths, TheShapeIsRaisedTowardsACameraAboveIt)
{
	// Looking straight down from 100 up: raised by the size, no more than half way
	EXPECT_EQ(trail::Lift({0.0f, 0.0f, 0.0f}, {0.0f, 100.0f, 0.0f}, 3.0f), glm::vec3(0.0f, 3.0f, 0.0f));
	EXPECT_EQ(trail::Lift({0.0f, 0.0f, 0.0f}, {0.0f, 4.0f, 0.0f}, 3.0f), glm::vec3(0.0f, 2.0f, 0.0f));
	// A camera level with it or below leaves it
	EXPECT_EQ(trail::Lift({0.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 0.0f}, 3.0f), glm::vec3(0.0f));
	// Seen from a slant, further along the line to the camera
	const auto raised = trail::Lift({0.0f, 0.0f, 0.0f}, {100.0f, 100.0f, 0.0f}, 3.0f);
	EXPECT_NEAR(raised.y, 3.0f, 1e-5f);
	EXPECT_NEAR(raised.x, 3.0f, 1e-5f);
}

TEST(GestureTrailMaths, TheSheetRisesAndFallsOverItsLife)
{
	EXPECT_FLOAT_EQ(trail::SheetStrength(0.0f, 4.0f), 0.0f);
	EXPECT_FLOAT_EQ(trail::SheetStrength(2.0f, 4.0f), 1.0f);
	EXPECT_FLOAT_EQ(trail::SheetStrength(1.0f, 4.0f), 0.75f);
	EXPECT_FLOAT_EQ(trail::SheetStrength(6.0f, 4.0f), 0.0f);
}

TEST(GestureTrailMaths, FlashesAndGlows)
{
	EXPECT_DOUBLE_EQ(trail::FlashLeft(1.05f, 2.1f), 1.0 - (static_cast<double>(1.05f) / 2.1f));
	EXPECT_DOUBLE_EQ(trail::FlashLeft(3.0f, 2.1f), 0.0);
	EXPECT_EQ(trail::Dimmed(0x00FF8040u, 128), 0x007F4020u);
}

TEST(GestureTrailMaths, TheChainIsSizedByTheHandsDistance)
{
	EXPECT_FLOAT_EQ(trail::ChainDistanceScale(0.0f), 0.2f);
	EXPECT_FLOAT_EQ(trail::ChainDistanceScale(25.0f), 0.6f);
	EXPECT_FLOAT_EQ(trail::ChainDistanceScale(200.0f), 1.0f);
	EXPECT_FLOAT_EQ(trail::ChainDistanceScale(1000.0f), 1.25f);
	EXPECT_FLOAT_EQ(trail::ChainDistanceScale(2000.0f), 1.5f);
}

TEST(GestureTrailSheet, ItsStrengthRunsAlongItAndTheWaveRolls)
{
	LightSheet sheet;
	std::vector<glm::vec3> points;
	for (int i = 0; i < 50; ++i)
	{
		points.emplace_back(static_cast<float>(i), 0.0f, 0.0f);
	}
	sheet.Start(points, 0x0000FFu, 9.0f, 0.03f);
	sheet.SetStrength(1.0f);
	sheet.Update(0.02f);
	EXPECT_FLOAT_EQ(sheet.Strengths()[0], 0.0f);
	sheet.Update(0.02f);
	// Moved one point along after 0.03 seconds, fed in at the first
	EXPECT_FLOAT_EQ(sheet.Strengths()[0], 1.0f);
	EXPECT_FLOAT_EQ(sheet.Strengths()[1], 0.0f);
	sheet.Update(0.07f);
	EXPECT_FLOAT_EQ(sheet.Strengths()[2], 1.0f);
	EXPECT_NEAR(sheet.Heights()[0], static_cast<float>((std::cos(-0.11 * 3.0) * 0.3 + 0.8) * 9.0), 1e-4f);

	std::vector<LightSheet::Vertex> vertices;
	std::vector<uint32_t> triangles;
	sheet.Build(vertices, triangles);
	ASSERT_EQ(vertices.size(), 150u);
	EXPECT_EQ(triangles.size(), 49u * 12u);
	// Dark at the land and the top, its light a quarter of the way up
	EXPECT_EQ(vertices[0].argb, 0xFF000000u);
	EXPECT_EQ(vertices[2].argb, 0xFF000000u);
	EXPECT_EQ(vertices[1].argb & 0xFFu, 0xFFu * 255u >> 8u);
	EXPECT_EQ(vertices[1].specularArgb, (((vertices[1].argb & 0xFFFFFFu) & 0xFEFEFEu) >> 1u) | 0x20000000u);
	EXPECT_NEAR(vertices[1].position.y, (vertices[2].position.y - vertices[0].position.y) * 0.25f, 1e-4f);
	// Its top spread out from the middle
	EXPECT_NEAR(vertices[2].position.x, ((0.0f - 24.5f) * 1.1f) + 24.5f, 1e-4f);
}
