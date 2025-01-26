/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <Camera/Camera.h>
#include <glm/gtx/vec_swizzle.hpp>
#include <gtest/gtest.h>
#include <json_helpers.h>

using namespace openblack;

constexpr uint16_t k_Width = 800;
constexpr uint16_t k_Height = 600;

class TestCamera: public ::testing::Test
{
protected:
	void SetUp() override
	{
		_camera = std::make_unique<Camera>();
		_camera->SetOrigin(glm::zero<glm::vec3>());
		_camera->SetFocus(glm::vec3(0.0f, 0.0f, 1.0f));
		_camera->SetProjectionMatrixPerspective(60.0f, static_cast<float>(k_Width) / static_cast<float>(k_Height), 0.001f,
		                                        1000.0f);
	}
	void TearDown() override { _camera.reset(); }
	std::unique_ptr<Camera> _camera;
};

TEST_F(TestCamera, Project)
{
	const auto screenSize = glm::u16vec2 {k_Width, k_Height};
	const auto result = _camera->ProjectWorldToScreen(glm::vec3(0.0f, 0.0f, 1.0f), U16Extent2 {{0, 0}, screenSize});
	ASSERT_TRUE(result.has_value());
	if (result.has_value())
	{
		ASSERT_EQ(static_cast<glm::u16vec2>(glm::xy(result.value())), glm::u16vec2(k_Width / 2, k_Height / 2));
	}
}

TEST_F(TestCamera, ProjectDeproject)
{
	constexpr auto k_ScreenPoints = std::array {
	    glm::u16vec2 {k_Width / 2, k_Height / 2},
	    glm::u16vec2 {k_Width / 4, k_Height / 4},
	};
	for (const auto& screenPoint : k_ScreenPoints)
	{
		constexpr auto k_ScreenSize = glm::u16vec2 {k_Width, k_Height};
		const auto ray =
		    _camera->DeprojectScreenToWorld(static_cast<glm::vec2>(screenPoint) / static_cast<glm::vec2>(k_ScreenSize));
		const auto result = _camera->ProjectWorldToScreen(ray.origin, U16Extent2 {{0, 0}, k_ScreenSize});
		ASSERT_TRUE(result.has_value());
		if (result.has_value())
		{
			ASSERT_EQ(static_cast<glm::u16vec2>(glm::xy(result.value())), screenPoint);
		}
	}
}
