/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Camera/CameraPan.h"

using namespace openblack::camera_pan;

namespace
{
void ExpectNear(glm::vec3 actual, glm::vec3 expected, float epsilon = 1e-3f)
{
	EXPECT_NEAR(actual.x, expected.x, epsilon);
	EXPECT_NEAR(actual.y, expected.y, epsilon);
	EXPECT_NEAR(actual.z, expected.z, epsilon);
}
} // namespace

TEST(CameraPan, OnLevelLandThePlaneIsTheLand)
{
	// As the game recorded it: the camera 3 above level land, gripping the land ahead
	const auto plane = PlaneThrough({1000.0f, 0.0f, 1116.20728f}, {1000.0f, 3.0f, 1120.0f}, 0.0f);
	ExpectNear(plane.normal, {0.0f, 1.0f, 0.0f});
	EXPECT_NEAR(plane.distance, 0.0f, 1e-5f);
}

TEST(CameraPan, OnASlopeThePlaneLeansBackTowardsTheCamera)
{
	// Land gripped up a hill ahead
	const auto plane = PlaneThrough({0.0f, 10.0f, 10.0f}, {0.0f, 50.0f, 0.0f}, 0.0f);
	ExpectNear(plane.normal, glm::normalize(glm::vec3(0.0f, 1.0f, -1.0f)));
	EXPECT_NEAR(plane.distance, glm::dot(plane.normal, glm::vec3(0.0f, 10.0f, 10.0f)), 1e-4f);
	// The ground under the camera counts no higher than the gripped land: from higher ground the plane is level
	const auto lower = PlaneThrough({0.0f, 10.0f, 10.0f}, {0.0f, 50.0f, 0.0f}, 30.0f);
	ExpectNear(lower.normal, {0.0f, 1.0f, 0.0f});
}

TEST(CameraPan, TheLandUnderTheCursorStaysUnderIt)
{
	const GripPlane level {.normal = {0.0f, 1.0f, 0.0f}, .distance = 0.0f};
	const glm::vec3 origin {0.0f, 10.0f, 0.0f};
	const glm::vec3 focus {0.0f, 0.0f, 10.0f};
	const auto distance = glm::distance(origin, focus);
	// Gripped straight below and ahead; the cursor now looks 5 further on
	const auto atGrip = glm::normalize(glm::vec3(0.0f, -10.0f, 10.0f));
	const auto now = glm::normalize(glm::vec3(0.0f, -10.0f, 15.0f));
	const auto place = Pan(level, origin, focus, distance, now, atGrip, {400, 200}, {400, 300});
	ASSERT_TRUE(place.has_value());
	// The camera comes 5 back, so the land gripped is where the cursor now looks
	ExpectNear(place->origin, {0.0f, 10.0f, -5.0f});
	ExpectNear(place->focus, {0.0f, 0.0f, 5.0f});
}

TEST(CameraPan, TheMoveIsLimitedByHowFarTheCursorWent)
{
	const GripPlane level {.normal = {0.0f, 1.0f, 0.0f}, .distance = 0.0f};
	const glm::vec3 origin {0.0f, 10.0f, 0.0f};
	const glm::vec3 focus {0.0f, 0.0f, 100.0f};
	const auto distance = glm::distance(origin, focus);
	const auto atGrip = glm::normalize(glm::vec3(0.0f, -10.0f, 10.0f));
	// Nearly along the land: the land under it is far away
	const auto now = glm::normalize(glm::vec3(0.0f, -1.0f, 200.0f));
	// One pixel moved: no more than 50
	const auto place = Pan(level, origin, focus, distance, now, atGrip, {400, 299}, {400, 300});
	ASSERT_TRUE(place.has_value());
	EXPECT_NEAR(glm::distance(place->focus, focus), 50.0f, 1e-2f);
	// A hundred pixels: 0.011 of the distance for each
	const auto further = Pan(level, origin, focus, distance, now, atGrip, {400, 200}, {400, 300});
	ASSERT_TRUE(further.has_value());
	EXPECT_NEAR(glm::distance(further->focus, focus), 100.0f * distance * 0.011f, 1e-2f);
}

TEST(CameraPan, LookingAboveTheLandDragsNothing)
{
	const GripPlane level {.normal = {0.0f, 1.0f, 0.0f}, .distance = 0.0f};
	const glm::vec3 origin {0.0f, 10.0f, 0.0f};
	const auto atGrip = glm::normalize(glm::vec3(0.0f, -10.0f, 10.0f));
	EXPECT_FALSE(
	    Pan(level, origin, {0.0f, 0.0f, 10.0f}, 14.0f, glm::normalize(glm::vec3(0.0f, 1.0f, 10.0f)), atGrip, {0, 0}, {0, 0})
	        .has_value());
	// Along the land
	EXPECT_FALSE(Pan(level, origin, {0.0f, 0.0f, 10.0f}, 14.0f, {0.0f, 0.0f, 1.0f}, atGrip, {0, 0}, {0, 0}).has_value());
}

TEST(CameraPan, StopsThreeShortOfLandInItsWay)
{
	const CameraPlace place {.origin = {0.0f, 10.0f, 100.0f}, .focus = {0.0f, 0.0f, 110.0f}};
	// Land 50 along the way: the camera stops 3 short of it
	const auto stopped = StopShortOfLand(place, {0.0f, 10.0f, 0.0f}, {0.0f, 5.0f, 50.0f});
	ExpectNear(stopped.origin, {0.0f, 10.0f, 47.0f});
	ExpectNear(stopped.focus, {0.0f, 0.0f, 57.0f});
	// Land beyond the move doesn't stop it
	const auto onward = StopShortOfLand(place, {0.0f, 10.0f, 0.0f}, {0.0f, 5.0f, 200.0f});
	ExpectNear(onward.origin, place.origin);
	// Nor does a move of 3 or less across
	const CameraPlace small {.origin = {0.0f, 10.0f, 3.0f}, .focus = {0.0f, 0.0f, 13.0f}};
	ExpectNear(StopShortOfLand(small, {0.0f, 10.0f, 0.0f}, {0.0f, 5.0f, 1.0f}).origin, small.origin);
}

TEST(CameraPan, TheSeaIsMetGoingDownNearTheCamera)
{
	const auto hit = SeaHit({0.0f, 10.0f, 0.0f}, {0.0f, 5.0f, 10.0f}, {0.0f, 10.0f, 0.0f});
	ASSERT_TRUE(hit.has_value());
	ExpectNear(*hit, {0.0f, 0.0f, 20.0f});
	// Going up, or level, never meets it
	EXPECT_FALSE(SeaHit({0.0f, 10.0f, 0.0f}, {0.0f, 15.0f, 10.0f}, {0.0f, 10.0f, 0.0f}).has_value());
	EXPECT_FALSE(SeaHit({0.0f, 10.0f, 0.0f}, {0.0f, 10.0f, 10.0f}, {0.0f, 10.0f, 0.0f}).has_value());
	// Nor more than 7500 across from the camera
	EXPECT_FALSE(SeaHit({0.0f, 10.0f, 0.0f}, {0.0f, 9.99f, 10.0f}, {0.0f, 10.0f, 0.0f}).has_value());
}
