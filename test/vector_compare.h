/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cmath>

#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <gtest/gtest.h>

// Specialization for glm::vec2
inline ::testing::AssertionResult vectorsNear(const char* expr1, const char* expr2, const char* absErrorExpr,
                                              const glm::vec2& v1, const glm::vec2& v2, float absError = 0.001f)
{
	if (glm::all(glm::epsilonEqual(v1, v2, absError)))
	{
		return ::testing::AssertionSuccess();
	}

	return ::testing::AssertionFailure() << "The difference between " << expr1 << " and " << expr2 << " is more than "
	                                     << absErrorExpr << ".\n"
	                                     << expr1 << " is " << glm::to_string(v1) << "\n"
	                                     << expr2 << " is " << glm::to_string(v2);
}

// Create a single macro that accepts variable number of arguments
#define ASSERT_VEC_EXPAND_MACRO(x) x
#define ASSERT_VEC_GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define ASSERT_VEC2_NEAR(...) \
	ASSERT_VEC_EXPAND_MACRO(ASSERT_VEC_GET_MACRO(__VA_ARGS__, ASSERT_VEC2_NEAR_ABS, ASSERT_VEC2_NEAR_DEFAULT)(__VA_ARGS__))

// Implementation macros
#define ASSERT_VEC2_NEAR_DEFAULT(val1, val2) ASSERT_PRED_FORMAT3(vectorsNear, val1, val2, 0.001f)
#define ASSERT_VEC2_NEAR_ABS(val1, val2, absError) ASSERT_PRED_FORMAT3(vectorsNear, val1, val2, absError)
