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
#include <ECS/Registry.h>
#include <Locator.h>
#include <gtest/gtest.h>
#include <json_helpers.h>

#include "scenarios/DoubleClickFlyTo.h"
#include "scenarios/DragUpDown.h"
#include "scenarios/MoveBackwardForward.h"
#include "scenarios/MoveRightLeft.h"
#include "scenarios/PanRightLeft.h"
#include "scenarios/TiltDownZoomOut.h"
#include "scenarios/TiltUpDown.h"
#include "scenarios/TiltUpPanLeft.h"
#include "scenarios/ZoomOutIn.h"

// Enable this define because we use a custom locator
#define LOCATOR_IMPLEMENTATIONS
#include <Camera/DefaultWorldCameraModel.h>

#include "scenarios/Mock.h"

using nlohmann::json;
using openblack::ecs::Registry;
using namespace openblack;

struct TestValues
{
	std::string_view name;
	MockDynamicsSystem* dynamicsSystem;
	MockAction* actionInterface;
};
// Padding causes valgrind errors https://github.com/google/googletest/issues/3805
static_assert(std::has_unique_object_representations_v<TestValues>);

class TestDefaultCameraModel: public testing::TestWithParam<TestValues>
{
protected:
	void SetUp() override
	{
		const auto testName = GetParam().name;
		const auto testResultsPath = std::filesystem::path(k_ScenarioPath) / (testName.data() + std::string(".json"));
		ASSERT_TRUE(std::filesystem::exists(testResultsPath));
		std::ifstream(testResultsPath) >> _scenario;

		Locator::entitiesRegistry::emplace<Registry>();

		_camera = std::make_unique<Camera>();
		GetParam().dynamicsSystem->camera = _camera.get();

		Locator::terrainSystem::emplace<MockTerrain>();
		Locator::windowing::emplace<MockWindowingSystem>();
		Locator::dynamicsSystem::reset<MockDynamicsSystem>(GetParam().dynamicsSystem);
		Locator::gameActionSystem::reset<MockAction>(GetParam().actionInterface);

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

	static void SetModel(DefaultWorldCameraModel& m, const json& json);
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

void TestDefaultCameraModel::SetModel(DefaultWorldCameraModel& m, const json& json)
{
	const auto& expected = json["cameraController"];

	if (expected["screenCentreHit"].get<bool>())
	{
		m._screenSpaceCenterRaycastHit = expected["screenCentreHitPoint"].get<glm::vec3>();
	}
	else
	{
		m._screenSpaceCenterRaycastHit = std::nullopt;
	}

	m._currentOrigin = expected["origin"].get<glm::vec3>();
	m._currentFocus = expected["heading"].get<glm::vec3>();

	m._originFocusDistanceAtInteractionStart = expected["originHeadingDistanceAtInteractionStart"].get<float>();
	m._originToHandPlaneNormal = expected["handHeadingNormalAtInteractionStart"].get<glm::vec3>();
	m._alignmentAtInteractionStart = expected["mouseHitPointDot"].get<float>();
	m._focusDistance = expected["headingDistance"].get<float>();
	m._averageIslandDistance = expected["averageIslandDistance"].get<float>();
	m._originAtClick = expected["originAtClick"].get<glm::vec3>();
	m._focusAtClick = expected["headingAtClick"].get<glm::vec3>();

	m._targetOrigin = expected["fallbackOrigin"].get<glm::vec3>();
	m._targetFocus = expected["fallbackHeading"].get<glm::vec3>();

	if (expected["handHit"].get<bool>())
	{
		m._screenSpaceMouseRaycastHit = expected["mouseHitPoint"].get<glm::vec3>();
	}
	else
	{
		m._screenSpaceMouseRaycastHit = std::nullopt;
	}

	m._rotateAroundDelta = json["RotateAroundDelta"].get<glm::vec3>();
	m._keyBoardMoveDelta = json["KeyboardMoveDelta"].get<glm::vec2>();

	const auto handStatus = expected["handStatus"].get<std::string>();
	if (handStatus == "CAMERA_MODE_HAND_STATUS_NORMAL")
	{
		m._modePrev = DefaultWorldCameraModel::Mode::Cartesian;
	}
	else if (handStatus == "CAMERA_MODE_HAND_STATUS_ZOOMING")
	{
		m._modePrev = DefaultWorldCameraModel::Mode::Polar;
	}
	else if (handStatus == "CAMERA_MODE_HAND_STATUS_GRABBING_LAND")
	{
		m._modePrev = DefaultWorldCameraModel::Mode::DraggingLandscape;
	}
	else
	{
		assert(false);
	}
}

void TestDefaultCameraModel::ValidateModel(const DefaultWorldCameraModel& m, const json& json, int frameNumber)
{
	const float ep = 5e-2f;
	const auto& expected = json["cameraController"];

	ASSERT_EQ(m._screenSpaceCenterRaycastHit.has_value(), expected["screenCentreHit"].get<bool>())
	    << "at start of frame " << frameNumber;
	if (m._screenSpaceCenterRaycastHit.has_value())
	{
		ASSERT_EQ(m._screenSpaceCenterRaycastHit, expected["screenCentreHitPoint"].get<glm::vec3>())
		    << "at start of frame " << frameNumber;
	}

	ASSERT_NEAR(m._originFocusDistanceAtInteractionStart, expected["originHeadingDistanceAtInteractionStart"].get<float>(), ep)
	    << "at start of frame " << frameNumber;
	ASSERT_NEAR(m._focusDistance, expected["headingDistance"].get<float>(), ep) << "at start of frame " << frameNumber;
	ASSERT_NEAR(m._averageIslandDistance, expected["averageIslandDistance"].get<float>(), ep)
	    << "at start of frame " << frameNumber;
	ASSERT_NEAR(m._focusAtClick.x, expected["headingAtClick"].get<glm::vec3>().x, ep) << "at start of frame " << frameNumber;
	ASSERT_NEAR(m._focusAtClick.y, expected["headingAtClick"].get<glm::vec3>().y, ep) << "at start of frame " << frameNumber;
	ASSERT_NEAR(m._focusAtClick.z, expected["headingAtClick"].get<glm::vec3>().z, ep) << "at start of frame " << frameNumber;

	ASSERT_NEAR(m._mouseAtClick.x, expected["mousePosPrevious"].get<glm::u16vec2>().x, ep)
	    << "at start of frame " << frameNumber;
	ASSERT_NEAR(m._mouseAtClick.y, expected["mousePosPrevious"].get<glm::u16vec2>().y, ep)
	    << "at start of frame " << frameNumber;

	ASSERT_NEAR(m.GetTargetOrigin().x, expected["fallbackOrigin"].get<glm::vec3>().x, ep)
	    << "at start of frame " << frameNumber;
	ASSERT_NEAR(m.GetTargetOrigin().y, expected["fallbackOrigin"].get<glm::vec3>().y, ep)
	    << "at start of frame " << frameNumber;
	ASSERT_NEAR(m.GetTargetOrigin().z, expected["fallbackOrigin"].get<glm::vec3>().z, ep)
	    << "at start of frame " << frameNumber;
	ASSERT_NEAR(m.GetTargetFocus().x, expected["fallbackHeading"].get<glm::vec3>().x, ep)
	    << "at start of frame " << frameNumber;
	ASSERT_NEAR(m.GetTargetFocus().y, expected["fallbackHeading"].get<glm::vec3>().y, ep)
	    << "at start of frame " << frameNumber;
	ASSERT_NEAR(m.GetTargetFocus().z, expected["fallbackHeading"].get<glm::vec3>().z, ep)
	    << "at start of frame " << frameNumber;

	// Frame 1 should have 15.8141842, -13.4212151 but current raycasting doesn't find anything.
	// This will have to be investigated and reversed if the mouse mock pos contributes greatly
	ASSERT_EQ(m._screenSpaceMouseRaycastHit.has_value(), expected["handHit"].get<bool>())
	    << "at start of frame " << frameNumber;
	if (m._screenSpaceMouseRaycastHit.has_value())
	{
		ASSERT_EQ(m._screenSpaceMouseRaycastHit, expected["mouseHitPoint"].get<glm::vec3>())
		    << "at start of frame " << frameNumber;
	}

	if (!m._flightPath.has_value())
	{
		const auto handStatus = expected["handStatus"].get<std::string>();
		if (handStatus == "CAMERA_MODE_HAND_STATUS_NORMAL")
		{
			ASSERT_EQ(m._modePrev, DefaultWorldCameraModel::Mode::Cartesian) << "at start of frame " << frameNumber;
		}
		else if (handStatus == "CAMERA_MODE_HAND_STATUS_ZOOMING")
		{
			ASSERT_EQ(m._modePrev, DefaultWorldCameraModel::Mode::Polar) << "at start of frame " << frameNumber;
		}
		else if (handStatus == "CAMERA_MODE_HAND_STATUS_GRABBING_LAND")
		{
			ASSERT_EQ(m._modePrev, DefaultWorldCameraModel::Mode::DraggingLandscape) << "at start of frame " << frameNumber;
		}
		else
		{
			ASSERT_FALSE(true) << "at start of frame " << frameNumber;
		}
	}

	ASSERT_EQ(m._flightPath.has_value(), expected["setZoomerToFlyToTargetFocus"].get<bool>())
	    << "at start of frame " << frameNumber;
	if (m._flightPath.has_value())
	{
		ASSERT_EQ(m._flightPath->midpoint.has_value(), expected["setZoomerToFlyToHalfWay"].get<bool>())
		    << "at start of frame " << frameNumber;
		ASSERT_NEAR(m._flightPath->origin.x, expected["flyToTargetOrigin"].get<glm::vec3>().x, ep)
		    << "at start of frame " << frameNumber;
		ASSERT_NEAR(m._flightPath->origin.y, expected["flyToTargetOrigin"].get<glm::vec3>().y, ep)
		    << "at start of frame " << frameNumber;
		ASSERT_NEAR(m._flightPath->origin.z, expected["flyToTargetOrigin"].get<glm::vec3>().z, ep)
		    << "at start of frame " << frameNumber;
		ASSERT_NEAR(m._flightPath->focus.x, expected["flyToTargetFocus"].get<glm::vec3>().x, ep)
		    << "at start of frame " << frameNumber;
		ASSERT_NEAR(m._flightPath->focus.y, expected["flyToTargetFocus"].get<glm::vec3>().y, ep)
		    << "at start of frame " << frameNumber;
		ASSERT_NEAR(m._flightPath->focus.z, expected["flyToTargetFocus"].get<glm::vec3>().z, ep)
		    << "at start of frame " << frameNumber;
		if (m._flightPath->midpoint.has_value())
		{
			ASSERT_NEAR(m._flightPath->midpoint->x, expected["setZoomerToFlyToHalfWay"].get<glm::vec3>().x, ep)
			    << "at start of frame " << frameNumber;
			ASSERT_NEAR(m._flightPath->midpoint->y, expected["setZoomerToFlyToHalfWay"].get<glm::vec3>().y, ep)
			    << "at start of frame " << frameNumber;
			ASSERT_NEAR(m._flightPath->midpoint->z, expected["setZoomerToFlyToHalfWay"].get<glm::vec3>().z, ep)
			    << "at start of frame " << frameNumber;
		}
	}
}

TEST_P(TestDefaultCameraModel, ValidateRecordedData)
{
	for (int i = 0; i < _scenario["frames"].size() - 1; ++i)
	{
		const auto& framePrev = _scenario["frames"][i];
		const auto& framePost = _scenario["frames"][i + 1];
		ASSERT_EQ(framePrev["frame"], i);
		ASSERT_EQ(framePost["frame"], i + 1);

		SetModel(reinterpret_cast<DefaultWorldCameraModel&>(*_model), framePrev);

		{
			const auto p0 = glm::vec3 {
			    framePrev["camera"]["camera_origin_zoomer"]["x"]["start_value"].get<float>(),
			    framePrev["camera"]["camera_origin_zoomer"]["y"]["start_value"].get<float>(),
			    framePrev["camera"]["camera_origin_zoomer"]["z"]["start_value"].get<float>(),
			};
			const auto p1 = glm::vec3 {
			    framePrev["camera"]["camera_origin_zoomer"]["x"]["destination"].get<float>(),
			    framePrev["camera"]["camera_origin_zoomer"]["y"]["destination"].get<float>(),
			    framePrev["camera"]["camera_origin_zoomer"]["z"]["destination"].get<float>(),
			};
			const auto v0 = glm::vec3 {
			    framePrev["camera"]["camera_origin_zoomer"]["x"]["start_speed"].get<float>(),
			    framePrev["camera"]["camera_origin_zoomer"]["y"]["start_speed"].get<float>(),
			    framePrev["camera"]["camera_origin_zoomer"]["z"]["start_speed"].get<float>(),
			};
			const auto v1 = glm::vec3 {
			    framePrev["camera"]["camera_origin_zoomer"]["x"]["destination_speed"].get<float>(),
			    framePrev["camera"]["camera_origin_zoomer"]["y"]["destination_speed"].get<float>(),
			    framePrev["camera"]["camera_origin_zoomer"]["z"]["destination_speed"].get<float>(),
			};
			const auto duration = framePrev["camera"]["camera_origin_zoomer"]["x"]["duration"].get<float>();
			const auto currentTime = framePrev["camera"]["camera_origin_zoomer"]["x"]["current_time"].get<float>();
			const auto t = duration != 0.0f ? currentTime / duration : 0.0f;
			(*_camera).SetOriginInterpolator(p0, p1, v0 * duration, v1 * duration).SetInterpolatorT(t);
		}
		{
			const auto p0 = glm::vec3 {
			    framePrev["camera"]["camera_heading_zoomer"]["x"]["start_value"].get<float>(),
			    framePrev["camera"]["camera_heading_zoomer"]["y"]["start_value"].get<float>(),
			    framePrev["camera"]["camera_heading_zoomer"]["z"]["start_value"].get<float>(),
			};
			const auto p1 = glm::vec3 {
			    framePrev["camera"]["camera_heading_zoomer"]["x"]["destination"].get<float>(),
			    framePrev["camera"]["camera_heading_zoomer"]["y"]["destination"].get<float>(),
			    framePrev["camera"]["camera_heading_zoomer"]["z"]["destination"].get<float>(),
			};
			const auto v0 = glm::vec3 {
			    framePrev["camera"]["camera_heading_zoomer"]["x"]["start_speed"].get<float>(),
			    framePrev["camera"]["camera_heading_zoomer"]["y"]["start_speed"].get<float>(),
			    framePrev["camera"]["camera_heading_zoomer"]["z"]["start_speed"].get<float>(),
			};
			const auto v1 = glm::vec3 {
			    framePrev["camera"]["camera_heading_zoomer"]["x"]["destination_speed"].get<float>(),
			    framePrev["camera"]["camera_heading_zoomer"]["y"]["destination_speed"].get<float>(),
			    framePrev["camera"]["camera_heading_zoomer"]["z"]["destination_speed"].get<float>(),
			};
			const auto duration = framePrev["camera"]["camera_heading_zoomer"]["x"]["duration"].get<float>();
			const auto currentTime = framePrev["camera"]["camera_heading_zoomer"]["x"]["current_time"].get<float>();
			const auto t = duration != 0.0f ? currentTime / duration : 0.0f;
			(*_camera).SetFocusInterpolator(p0, p1, v0 * duration, v1 * duration).SetInterpolatorT(t);
		}

		GetParam().dynamicsSystem->frameNumber = i;
		GetParam().actionInterface->frameNumber = i;

		const auto deltaTimePrev = std::chrono::milliseconds(framePrev["g_delta_time"].get<int>());
		const auto updateInfo = _model->Update(deltaTimePrev, *_camera);
		_model->HandleActions(deltaTimePrev);

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

#define SCENARIO_VALUES(name)                                     \
	TestValues                                                    \
	{                                                             \
		#name, new name##MockDynamicsSystem, new name##MockAction \
	}

const auto k_TestingScenarioValues = testing::Values( //
    SCENARIO_VALUES(MoveBackwardForward),             //
    SCENARIO_VALUES(MoveRightLeft),                   //
    SCENARIO_VALUES(PanRightLeft),                    //
    SCENARIO_VALUES(TiltUpDown),                      //
    SCENARIO_VALUES(ZoomOutIn),                       //
    SCENARIO_VALUES(TiltDownZoomOut),                 //
    SCENARIO_VALUES(TiltUpPanLeft),                   //
    SCENARIO_VALUES(DragUpDown),                      //
    SCENARIO_VALUES(DoubleClickFlyTo)                 //
);

INSTANTIATE_TEST_SUITE_P(TestScenarioInstantiation, TestDefaultCameraModel, k_TestingScenarioValues,
                         [](const testing::TestParamInfo<TestDefaultCameraModel::ParamType>& info) {
	                         return info.param.name.data();
                         });
