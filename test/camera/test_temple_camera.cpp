/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstring>

#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

#include "3D/CameraPath.h"
#include "Camera/Camera.h"
#include "Camera/TempleCameraModel.h"
#include "Common/Zoomer.h"

using namespace openblack;

namespace
{
/// A .cam file of a path over a second along x, looking ahead of itself
std::vector<uint8_t> MakeCam(uint32_t durationMs, uint32_t pointCount)
{
	std::vector<uint8_t> data(3 * sizeof(uint32_t) + pointCount * 6 * sizeof(float));
	const uint32_t header[3] {static_cast<uint32_t>(data.size()), durationMs, pointCount};
	std::memcpy(data.data(), header, sizeof(header));
	for (uint32_t i = 0; i < pointCount; ++i)
	{
		const float point[6] {static_cast<float>(i) * 10.0f, 5.0f, 0.0f, static_cast<float>(i) * 10.0f + 1.0f, 5.0f, 0.0f};
		std::memcpy(&data[sizeof(header) + i * sizeof(point)], point, sizeof(point));
	}
	return data;
}
} // namespace

TEST(Zoomer, ArrivesAtItsDestinationInTime)
{
	Zoomer value(2.0f);
	value.SetDestination(10.0f, 0.5f);
	float previous = value.GetValue();
	for (int i = 0; i < 25; ++i)
	{
		value.Update(0.02f);
		// Easing out of rest, it heads to the destination without going back
		EXPECT_GE(value.GetValue(), previous - 1e-4f);
		previous = value.GetValue();
	}
	EXPECT_FLOAT_EQ(value.GetValue(), 10.0f);
	EXPECT_FLOAT_EQ(value.GetSpeed(), 0.0f);
}

TEST(Zoomer, ItsSpeedIsHowFastItsValueChanges)
{
	// Zoomer's path is a polynomial in time whose terms are those of a Taylor series, so its speed is its derivative
	Zoomer value(0.0f);
	value.SetDestination(100.0f, 7.0f);
	value.Update(2.0f);
	const float before = value.GetValue();
	const float speed = value.GetSpeed();
	value.Update(0.001f);
	EXPECT_NEAR((value.GetValue() - before) / 0.001f, speed, 0.05f);
	// Easing out of rest towards a stop with no pull left, it has come most of the way by halfway
	value.Update(1.499f);
	EXPECT_NEAR(value.GetValue(), 68.75f, 0.01f);
}

TEST(Zoomer, PutsItThereForNoTime)
{
	Zoomer value(2.0f);
	value.SetDestination(-3.0f, 0.0f);
	EXPECT_FLOAT_EQ(value.GetValue(), -3.0f);
	value.Update(0.1f);
	EXPECT_FLOAT_EQ(value.GetValue(), -3.0f);
}

TEST(CameraPath, SamplesBetweenItsPointsAndHoldsTheLast)
{
	CameraPath path;
	ASSERT_TRUE(path.LoadFromBuffer(MakeCam(1000, 11)));
	// 10 spans of 100ms: halfway through the third is between the third and fourth points
	const auto middle = path.SampleAt(std::chrono::milliseconds(250));
	EXPECT_NEAR(middle.position.x, 25.0f, 1e-4f);
	EXPECT_NEAR(middle.focus.x, 26.0f, 1e-4f);
	EXPECT_FLOAT_EQ(path.SampleAt(std::chrono::milliseconds(0)).position.x, 0.0f);
	// Past its end, it stays just short of the last point, as InnerCamera's does
	EXPECT_NEAR(path.SampleAt(std::chrono::milliseconds(5000)).position.x, 99.9f, 1e-3f);
}

TEST(CameraPath, RefusesCamFilesShortOfTheirPoints)
{
	// CameraPath reports the file it couldn't read to the game's log
	if (spdlog::get("game") == nullptr)
	{
		spdlog::create<spdlog::sinks::null_sink_st>("game");
	}
	CameraPath path;
	auto data = MakeCam(1000, 4);
	data.resize(data.size() - 4);
	EXPECT_FALSE(path.LoadFromBuffer(data));
}

TEST(TempleCamera, LooksAtTheMainRoomFromItsOrbit)
{
	// Unturned, it starts behind the pool looking over it at the scroll, leaning a little up
	const auto start = TempleCameraModel::OrbitPose(0.0f, 0.7f);
	EXPECT_NEAR(start.origin.y, 9.92f, 1e-4f);
	EXPECT_NEAR(start.origin.z, 18.32f, 1e-4f);
	EXPECT_NEAR(start.focus.y, 8.4f, 1e-4f);
	// A quarter turn takes it round to the -x side
	const auto turned = TempleCameraModel::OrbitPose(glm::half_pi<float>(), 0.0f);
	EXPECT_NEAR(turned.origin.x, -14.4f, 1e-4f);
	EXPECT_NEAR(turned.origin.z, 0.0f, 1e-4f);
}

TEST(TempleCamera, FindsTheMainRoomsDoorsAroundItsWalls)
{
	// From the room's centre at door height, out along each door's eighth of the room, a little past its start
	for (uint32_t door = 0; door < 8; ++door)
	{
		const float angle = static_cast<float>(door) * glm::quarter_pi<float>() + 0.0f;
		const glm::vec3 direction {std::cos(angle), 0.0f, std::sin(angle)};
		const auto hit = TempleCameraModel::RayCastRoom(glm::vec3(0.0f, 15.0f, 0.0f), direction * 0.2f);
		ASSERT_TRUE(hit.has_value());
		EXPECT_FALSE(hit->floor);
		EXPECT_NEAR(hit->distance, TempleCameraModel::k_RoomRadius, 1e-3f);
		const auto found = TempleCameraModel::DoorAt(*hit);
		ASSERT_TRUE(found.has_value()) << "door " << door;
		EXPECT_EQ(*found, door);
	}
	// Between doors, and above them, there are none
	const glm::vec3 between {std::cos(0.4f), 0.0f, std::sin(0.4f)};
	EXPECT_FALSE(TempleCameraModel::DoorAt(*TempleCameraModel::RayCastRoom(glm::vec3(0.0f, 15.0f, 0.0f), between)));
	EXPECT_FALSE(TempleCameraModel::DoorAt(*TempleCameraModel::RayCastRoom(glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(1, 0, 0))));
}

TEST(TempleCamera, HitsTheFloorBelowAndLeadsThroughTheDoorsToTheRooms)
{
	const auto hit = TempleCameraModel::RayCastRoom(glm::vec3(0.0f, 10.0f, 20.0f), glm::vec3(0.0f, -0.2f, -0.05f));
	ASSERT_TRUE(hit.has_value());
	EXPECT_TRUE(hit->floor);
	EXPECT_NEAR(hit->point.z, 17.5f, 1e-3f);

	// The rooms off the main room, around it from +x
	EXPECT_EQ(TempleCameraModel::RoomBehindDoor(0), TempleRoom::CreatureCave);
	EXPECT_EQ(TempleCameraModel::RoomBehindDoor(1), TempleRoom::Options);
	EXPECT_EQ(TempleCameraModel::RoomBehindDoor(3), TempleRoom::Multi);
	EXPECT_EQ(TempleCameraModel::RoomBehindDoor(4), TempleRoom::Credits);
	EXPECT_EQ(TempleCameraModel::RoomBehindDoor(5), TempleRoom::SaveGame);
	EXPECT_EQ(TempleCameraModel::RoomBehindDoor(7), TempleRoom::Challenge);
	// And the way out
	EXPECT_FALSE(TempleCameraModel::RoomBehindDoor(2).has_value());
	// Through a door, the camera heads out along its eighth
	const auto through = TempleCameraModel::TurnToDoor({{100.0f, 8.0f, 0.0f}, {120.0f, 10.0f, 0.0f}}, 2);
	EXPECT_NEAR(through.origin.x, 0.0f, 1e-3f);
	EXPECT_NEAR(through.origin.z, 100.0f, 1e-3f);
}

TEST(TempleCamera, ClickingADoorOnScreenFindsIt)
{
	// The main room as the player takes it over, looking at the scroll through InnerCamera's lens
	constexpr glm::vec2 k_Screen {1280.0f, 720.0f};
	const auto pose = TempleCameraModel::OrbitPose(0.0f, 0.7f);
	Camera camera;
	camera.SetProjectionMatrixPerspective(90.0f, k_Screen.x / k_Screen.y, 0.2f, 1000.0f);
	camera.SetOrigin(pose.origin);
	camera.SetFocus(pose.focus);

	// The middle of door 7 at door height, where the mouse is, cast back into the room as the camera's model does
	const float doorAngle = -glm::quarter_pi<float>();
	const glm::vec3 door {TempleCameraModel::k_RoomRadius * std::cos(doorAngle), 15.0f,
	                      TempleCameraModel::k_RoomRadius * std::sin(doorAngle)};
	glm::vec3 screen;
	ASSERT_TRUE(camera.ProjectWorldToScreen(door, glm::vec4(0.0f, 0.0f, k_Screen), screen));
	glm::vec3 rayOrigin;
	glm::vec3 rayDirection;
	camera.DeprojectScreenToWorld(glm::vec2(screen) / k_Screen, rayOrigin, rayDirection);
	const auto direction = rayDirection * (0.2f / glm::dot(rayDirection, camera.GetForward()));
	const auto hit = TempleCameraModel::RayCastRoom(camera.GetOrigin(), direction);
	ASSERT_TRUE(hit.has_value());
	EXPECT_NEAR(hit->height, 15.0f, 0.05f);
	EXPECT_EQ(TempleCameraModel::DoorAt(*hit), 7u);
	EXPECT_EQ(TempleCameraModel::RoomBehindDoor(7), TempleRoom::Challenge);
}
