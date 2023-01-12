/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#include <3D/Camera.h>
#include <3D/LandIsland.h>

#include "common/Fixtures.h"

class SetCameraPos: public TestLoadSceneScript
{
public:
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

	void AssertCameraPos(glm::vec3 expectedPos)
	{
		const auto actualPos = _game->GetCamera().GetPosition();
		ASSERT_FLOAT_EQ(actualPos.x, expectedPos.x);
		ASSERT_FLOAT_EQ(actualPos.z, expectedPos.z);
		ASSERT_FLOAT_EQ(actualPos.y, expectedPos.y);
	}
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, setCameraZero)
{
	LoadSceneWithCameraPos({0.0f, 0.0f}, {0.0f, 0.0f});
	const auto altitude = _game->GetLandIsland().GetHeightAt({0.0f, 0.0f});
	AssertCameraPos({44.7591362f, 26.7025127f, -108.090675f});
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, setCameraLand1)
{
	// Since the land island is mocked, the land is flat and the altitude is the same
	const auto cameraPos = glm::vec2(1788.40f, 2710.00f);
	LoadSceneWithCameraPos(cameraPos, {1915.05f, 2508.89f});
	const auto altitude = _game->GetLandIsland().GetHeightAt(cameraPos);
	ASSERT_FLOAT_EQ(altitude, 28.81); // Actual vanilla value 28.9173050f
	AssertCameraPos({1833.1592f, 55.512512f /*55.6197281f*/, 2601.9094f});
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, setCameraTwoGods)
{
	// Since the land island is mocked, the land is flat and the altitude is the same
	LoadSceneWithCameraPos({2185.72f, 2315.78f}, {1989.46f, 2356.52f});
	AssertCameraPos({2230.47900f, 26.7025127f, 2207.68945f});
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, setCameraThreeGods)
{
	// Since the land island is mocked, the land is flat and the altitude is the same
	LoadSceneWithCameraPos({2816.90f, 2740.75f}, {2641.66f, 2381.22f});
	AssertCameraPos({2861.65894f, 26.7025127f, 2632.65942f});
}

// TODO(#562)
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(SetCameraPos, DISABLED_setCameraFourGods)
{
	// Since the land island is mocked, the land is flat and the altitude is the same
	LoadSceneWithCameraPos({0.0f, 0.0f}, {3159.89f, 1934.44f});
	AssertCameraPos({3159.89f, 26.7025127f, 1934.44f});
}
