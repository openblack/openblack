/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <3D/LandIslandInterface.h>
#include <Camera/Camera.h>
#include <gtest/gtest.h>

#include "Game.h"
#include "LHScriptX/Script.h"
#include "Locator.h"
#include "test_fixed.h"

class SetCameraPos: public TestFixed
{
protected:
	void SetUp() override
	{
		using namespace openblack;
		SetUpGame(Arguments {
		    .rendererType = bgfx::RendererType::Enum::Noop,
		    .gamePath = k_MockGamePath.string(),
		    .logFile = "stdout",
		});
	}

public:
	void LoadTestScene(const char* sceneScript)
	{
		openblack::lhscriptx::Script script;
		script.Load(sceneScript);
	}

	void LoadSceneWithCameraPos(glm::vec2 cameraPos, glm::vec2 citadelPos)
	{
		std::array<char, 0x100> script;
		snprintf(script.data(), script.size(),
		         "VERSION(2.300000)\n"
		         "LOAD_LANDSCAPE(\".\\Data\\Landscape\\Land1.lnd\")\n"
		         "START_CAMERA_POS(\"%.02f,%.02f\")\n"
		         "CREATE_CITADEL(\"%.02f,%.02f\", 0, \"PLAYER_ONE\", 0, 1000)\n",
		         cameraPos.x, cameraPos.y, citadelPos.x, citadelPos.y);
		LoadTestScene(script.data());
	}

	void ExpectCameraPos(glm::vec3 expectedPos)
	{
		constexpr float k_Ep = 1e-2;
		const auto actualPos = openblack::Locator::camera::value().GetOrigin();
		EXPECT_NEAR(actualPos.x, expectedPos.x, k_Ep);
		EXPECT_NEAR(actualPos.y, expectedPos.y, k_Ep);
		EXPECT_NEAR(actualPos.z, expectedPos.z, k_Ep);
	}
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, setCameraZero)
{
	LoadSceneWithCameraPos({0.0f, 0.0f}, {0.0f, 0.0f});
	ExpectCameraPos({44.7591362f, 26.7025127f, -108.090675f});
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, setCameraLand1)
{
	// Since the land island is mocked, the land is flat and the altitude is the same
	const auto cameraPos = glm::vec2(1788.40f, 2710.00f);
	LoadSceneWithCameraPos(cameraPos, {1915.05f, 2508.89f});
	const auto altitude = openblack::Locator::terrainSystem::value().GetHeightAt(cameraPos);
	ASSERT_FLOAT_EQ(altitude, 28.81); // Actual vanilla value 28.9173050f
	ExpectCameraPos({1833.1592f, 55.512512f /*55.6197281f*/, 2601.9094f});
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, setCameraTwoGods)
{
	// Since the land island is mocked, the land is flat and the altitude is the same
	LoadSceneWithCameraPos({2185.72f, 2315.78f}, {1989.46f, 2356.52f});
	ExpectCameraPos({2230.47900f, 26.7025127f, 2207.68945f});
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, setCameraThreeGods)
{
	// Since the land island is mocked, the land is flat and the altitude is the same
	LoadSceneWithCameraPos({2816.90f, 2740.75f}, {2641.66f, 2381.22f});
	ExpectCameraPos({2861.65894f, 26.7025127f, 2632.65942f});
}

// TODO(#562)
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, DISABLED_setCameraFourGods)
{
	// Since the land island is mocked, the land is flat and the altitude is the same
	LoadSceneWithCameraPos({0.0f, 0.0f}, {3159.89f, 1934.44f});
	ExpectCameraPos({3159.89f, 26.7025127f, 1934.44f});
}
