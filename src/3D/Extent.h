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

namespace openblack
{
template <glm::length_t L, typename T, glm::qualifier Q = glm::defaultp>
struct Extent
{
	glm::vec<L, T, Q> minimum;
	glm::vec<L, T, Q> maximum;
};

using Extent2 = Extent<2, float>;
using U16Extent2 = Extent<2, uint16_t>;
} // namespace openblack
