/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <fstream>

#include <Camera/Camera.h>
#include <Locator.h>
#include <gtest/gtest.h>
#include <json_helpers.h>

// Enable this define because we use a custom locator
#define LOCATOR_IMPLEMENTATIONS
#include <Camera/DefaultWorldCameraModel.h>
#include <ECS/Systems/Implementations/DynamicsSystem.h>

#include "scenarios/Mock.h"

using nlohmann::json;
using namespace openblack;

class TestDefaultCameraModel: public ::testing::Test
{
protected:
	void SetUp() override
	{
		const auto* testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		const auto testResultsPath = std::filesystem::path(k_ScenarioPath) / (testName + std::string(".json"));
		ASSERT_TRUE(std::filesystem::exists(testResultsPath));
		std::ifstream(testResultsPath) >> _scenario;

		_camera = std::make_unique<Camera>();
		Locator::windowing::emplace<MockWindowingSystem>();
		Locator::terrainSystem::emplace<MockTerrain>();
		Locator::dynamicsSystem::emplace<ecs::systems::DynamicsSystem>();
		Locator::gameActionSystem::emplace<MockAction>();

		const auto aspect = Locator::windowing::value().GetAspectRatio();
		constexpr float k_CameraXFov {70.0f};
		constexpr float k_CameraNearClip {1.0f};
		constexpr float k_CameraFarClip {static_cast<float>(0x10000)};
		_camera->SetProjectionMatrixPerspective(k_CameraXFov, aspect, k_CameraNearClip, k_CameraFarClip);
		// Camera has a model, but we want to test in isolation, so we create a model here
		_model = CameraModel::CreateModel(CameraModel::Model::DefaultWorld, _camera->GetOrigin(), _camera->GetFocus());
	}

	void TearDown() override
	{
		_model.reset();
		_camera.reset();
		Locator::terrainSystem::reset();
		Locator::windowing::reset();
		Locator::dynamicsSystem::reset();
		Locator::gameActionSystem::reset();
	}

	static void ValidateModel(const DefaultWorldCameraModel& m, const json& json, int frameNumber);

	static constexpr std::string_view k_ScenarioPath = TEST_BINARY_DIR "/camera/scenarios";
	json _scenario;
	std::unique_ptr<Camera> _camera;
	std::unique_ptr<CameraModel> _model;
};

void ValidateCamera(const Camera& c, json expected)
{
	ASSERT_EQ(c.GetOrigin(), glm::vec3(expected["camera_origin_zoomer"]["x"]["current_value"].get<float>(),
	                                   expected["camera_origin_zoomer"]["y"]["current_value"].get<float>(),
	                                   expected["camera_origin_zoomer"]["z"]["current_value"].get<float>()));
	ASSERT_EQ(c.GetFocus(), glm::vec3(expected["camera_heading_zoomer"]["x"]["current_value"].get<float>(),
	                                  expected["camera_heading_zoomer"]["y"]["current_value"].get<float>(),
	                                  expected["camera_heading_zoomer"]["z"]["current_value"].get<float>()));
}

void TestDefaultCameraModel::ValidateModel(const DefaultWorldCameraModel& m, const json& json, int frameNumber)
{
	const auto& expected = json["cameraController"];

	ASSERT_EQ(m._screenSpaceCenterRaycastHit.has_value(), expected["screenCentreHit"].get<bool>())
	    << "at frame " << frameNumber;
	if (m._screenSpaceCenterRaycastHit.has_value())
	{
		ASSERT_EQ(m._screenSpaceCenterRaycastHit, expected["screenCentreHitPoint"].get<glm::vec3>())
		    << "at frame " << frameNumber;
	}

	ASSERT_EQ(m._originFocusDistanceAtInteractionStart, expected["originHeadingDistanceAtInteractionStart"].get<float>())
	    << "at frame " << frameNumber;
	ASSERT_EQ(m._focusDistance, expected["headingDistance"].get<float>()) << "at frame " << frameNumber;
	ASSERT_EQ(m._averageIslandDistance, expected["averageIslandDistance"].get<float>()) << "at frame " << frameNumber;
	ASSERT_EQ(m._focusAtClick, expected["headingAtClick"].get<glm::vec3>()) << "at frame " << frameNumber;

	ASSERT_EQ(m.GetTargetOrigin(), expected["fallbackOrigin"].get<glm::vec3>()) << "at frame " << frameNumber;
	ASSERT_EQ(m.GetTargetFocus(), expected["fallbackHeading"].get<glm::vec3>()) << "at frame " << frameNumber;

	// Frame 1 should have 15.8141842, -13.4212151 but current raycasting doesn't find anything.
	// This will have to be investigated and reversed if the mouse mock pos contributes greatly
	ASSERT_EQ(m._screenSpaceMouseRaycastHit.has_value(), expected["handHit"].get<bool>()) << "at frame " << frameNumber;
	if (m._screenSpaceMouseRaycastHit.has_value())
	{
		ASSERT_EQ(m._screenSpaceMouseRaycastHit, expected["mouseHitPoint"].get<glm::vec3>()) << "at frame " << frameNumber;
	}

	ASSERT_EQ(m._rotateAroundDelta, json["RotateAroundDelta"].get<glm::vec3>()) << "at frame " << frameNumber;
	ASSERT_EQ(m._keyBoardMoveDelta, json["KeyboardMoveDelta"].get<glm::vec2>()) << "at frame " << frameNumber;
}

TEST_F(TestDefaultCameraModel, single_line)
{
	ValidateModel(reinterpret_cast<DefaultWorldCameraModel&>(*_model), _scenario["frames"][0], 0);
	for (int i = 0; i < _scenario["frames"].size() - 1; ++i)
	{
		const auto& framePrev = _scenario["frames"][i];
		const auto& framePost = _scenario["frames"][i + 1];
		ASSERT_EQ(framePrev["frame"], i);
		ASSERT_EQ(framePost["frame"], i + 1);

		const auto deltaTimePrev = framePrev["g_delta_time"].get<std::chrono::milliseconds>();
		const auto updateInfo = _model->Update(deltaTimePrev, *_camera);

		if (updateInfo)
		{
			const auto m1 = glm::zero<glm::vec3>();
			// You have to normalize the velocity with the NEW duration
			(*_camera)
			    .SetOriginInterpolator(_camera->GetOrigin(), updateInfo->origin,
			                           _camera->GetOriginVelocity() * updateInfo->duration.count(), m1)
			    .SetFocusInterpolator(_camera->GetFocus(), updateInfo->focus,
			                          _camera->GetFocusVelocity() * updateInfo->duration.count(), m1)
			    .SetInterpolatorDuration(updateInfo->duration);
		}

		const auto duration = _camera->GetInterpolatorDuration().count();
		if (duration == 0.0f)
		{
			(*_camera).SetInterpolatorT(1.0f);
		}
		else
		{
			const auto dtSeconds = std::chrono::duration_cast<std::chrono::duration<float>>(deltaTimePrev);
			(*_camera).SetInterpolatorTime(std::min(std::chrono::duration<float>(0.1f), dtSeconds));
		}

		ValidateModel(reinterpret_cast<DefaultWorldCameraModel&>(*_model), framePost, i + 1);
	}
}
