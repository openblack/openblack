/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Graphics/ZSort.h"

using namespace openblack::graphics;

TEST(ZSort, KeyIsTheDistanceSquared)
{
	EXPECT_FLOAT_EQ(zsort::Key({3.0f, 4.0f, 12.0f}, {0.0f, 0.0f, 0.0f}), 169.0f);
	EXPECT_FLOAT_EQ(zsort::Key({1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}), 0.0f);
}

TEST(ZSort, DepthOrdersAsTheDistance)
{
	const glm::vec3 camera {100.0f, 50.0f, -20.0f};
	float previous = 0.0f;
	uint32_t previousDepth = 0;
	for (float far = 0.01f; far < 20000.0f; far = (far * 1.37f) + 0.01f)
	{
		const glm::vec3 point = camera + glm::vec3(far, 0.0f, 0.0f);
		const auto key = zsort::Key(point, camera);
		const auto depth = zsort::Depth(point, camera);
		if (key > previous)
		{
			EXPECT_GT(depth, previousDepth);
		}
		previous = key;
		previousDepth = depth;
	}
}
