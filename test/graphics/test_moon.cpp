/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

#include "Graphics/Moon.h"

namespace moon = openblack::graphics::moon;

namespace
{
/// 6 January 2000, a new moon, in days since 1970
constexpr int64_t k_NewMoonDay = 0x2AD2;
} // namespace

TEST(Moon, ShowsAroundMidnight)
{
	const auto midnight = moon::Place(0.0f);
	ASSERT_TRUE(midnight.has_value());
	EXPECT_FLOAT_EQ(midnight->offset.x, 4000.0f);
	EXPECT_FLOAT_EQ(midnight->offset.y, 950.0f);
	EXPECT_NEAR(midnight->offset.z, 0.0f, 1e-3f);
	EXPECT_FLOAT_EQ(midnight->alpha, 200.0f);

	EXPECT_FALSE(moon::Place(6.0f).has_value());
	EXPECT_FALSE(moon::Place(12.0f).has_value());
	EXPECT_TRUE(moon::Place(23.0f).has_value());
}

TEST(Moon, PhaseFollowsTheRealMoon)
{
	// A whole turn at a new moon, then half way through a moon month of about 29.5 days
	EXPECT_NEAR(moon::Phase(k_NewMoonDay * 86400), 6.2831855f, 1e-5f);
	EXPECT_NEAR(moon::Phase((k_NewMoonDay + 15) * 86400 + 3600), (1.0f - (15.0f * 0.03386318f)) * 6.2831855f, 1e-4f);
}

TEST(Moon, FacesTheCamera)
{
	const glm::vec3 eye {0.0f, 100.0f, 0.0f};
	const auto view = glm::lookAt(eye, glm::vec3(4000.0f, 1000.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::vec3 position = eye + glm::vec3(4000.0f, 950.0f, 0.0f);
	const auto basis = moon::Basis(view, glm::inverse(view), position);
	// Four times the mesh's size, its axes square to each other, its third along the line from the camera
	for (int i = 0; i < 3; ++i)
	{
		EXPECT_NEAR(glm::length(basis[i]), 4.0f, 1e-4f);
	}
	EXPECT_NEAR(glm::dot(basis[0], basis[1]), 0.0f, 1e-3f);
	EXPECT_NEAR(glm::dot(glm::normalize(basis[2]), glm::normalize(position - eye)), 1.0f, 1e-4f);

	const auto model = moon::Model(basis, position, 0.0f);
	EXPECT_NEAR(glm::length(glm::vec3(model[0])), 4.0f * 0.65f, 1e-4f);
	EXPECT_EQ(glm::vec3(model[3]), position);
}

TEST(Moon, GlowIsASquareAboutTheMoon)
{
	const glm::mat3 basis(4.0f);
	const auto glow = moon::MakeGlow(basis, glm::vec3(0.0f));
	EXPECT_EQ(glow.corners[0], glm::vec3(-2000.0f, -2000.0f, 0.0f));
	EXPECT_EQ(glow.corners[3], glm::vec3(2000.0f, 2000.0f, 0.0f));
	EXPECT_EQ(moon::GlowColour(glm::vec3(0.6f, 0.5f, 0.4f)), glm::vec3(0.1f, 0.1f, 0.1f));
}
