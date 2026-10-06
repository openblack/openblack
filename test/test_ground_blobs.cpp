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

#include "Graphics/GroundBlobs.h"

using namespace openblack::graphics;

namespace
{
void ExpectNear(const glm::vec3& a, const glm::vec3& b)
{
	EXPECT_NEAR(a.x, b.x, 1e-5f);
	EXPECT_NEAR(a.y, b.y, 1e-5f);
	EXPECT_NEAR(a.z, b.z, 1e-5f);
}
} // namespace

TEST(GroundBlobs, FallsAlongTheDiagonalOnFlatLand)
{
	ExpectNear(ground_blobs::Fall({0.0f, 1.0f, 0.0f}, 1.0f), {1.41421356f, 0.0f, 1.41421356f});
	ExpectNear(ground_blobs::Fall({0.0f, 1.0f, 0.0f}, 2.0f), {2.82842712f, 0.0f, 2.82842712f});
}

TEST(GroundBlobs, FallLiesOnASlope)
{
	// On land tilted towards x, the fall loses the part of it into the land
	const glm::vec3 normal {0.6f, 0.8f, 0.0f};
	const auto fall = ground_blobs::Fall(normal, 1.0f);
	EXPECT_NEAR(glm::dot(fall, normal), 0.0f, 1e-5f);
}

TEST(GroundBlobs, QuadStartsBehindTheFootAndCrossesIt)
{
	const auto quad = ground_blobs::MakeQuad({10.0f, 5.0f, 20.0f}, {1.0f, 0.0f, 1.0f});
	const float a = 0.14142136f;
	ExpectNear(quad.corners[0], {10.0f - 0.02f + a, 5.0f, 20.0f - 0.02f - a});
	ExpectNear(quad.corners[1], {10.0f - 0.02f - a, 5.0f, 20.0f - 0.02f + a});
	ExpectNear(quad.corners[2], {11.0f - a, 5.0f, 21.0f + a});
	ExpectNear(quad.corners[3], {11.0f + a, 5.0f, 21.0f - a});
}

TEST(GroundBlobs, FeetLeanTowardsEachOther)
{
	const glm::vec3 first {0.0f, 0.0f, 0.0f};
	const glm::vec3 second {1.0f, 0.0f, 0.0f};
	const glm::vec3 fall {0.0f, 0.0f, 2.0f};
	const auto feet = ground_blobs::Feet(first, second, fall);
	// Each far end is the foot plus its fall, leaning half the way to the other foot
	ExpectNear((feet[0].corners[2] + feet[0].corners[3]) * 0.5f, first + fall + glm::vec3(0.5f, 0.0f, 0.0f));
	ExpectNear((feet[1].corners[2] + feet[1].corners[3]) * 0.5f, second + fall - glm::vec3(0.5f, 0.0f, 0.0f));
}
