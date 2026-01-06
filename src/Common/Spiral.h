/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/fwd.hpp>

namespace openblack::utils::spiral
{

const glm::i16vec2& Spiral(int& index, int& subIndex);
int GetIncrementSpiralSizeFromRadius(float radius, float step);
void SpiralIncrement(glm::vec3& point, int& index, int& subIndex, float size);
int GetMapCellSpiralSizeFromRadius(float radius);

} // namespace openblack::utils::spiral
