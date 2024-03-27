/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/detail/qualifier.hpp>
#include <glm/gtx/string_cast.hpp>

namespace glm
{
template <length_t L, typename T, qualifier Q>
std::ostream& operator<<(std::ostream& os, const vec<L, T, Q>& v)
{
	return os << glm::to_string(v);
}
} // namespace glm
