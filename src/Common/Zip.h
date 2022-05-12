/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openblack::zip
{

[[nodiscard]] std::vector<uint8_t> Inflate(const std::vector<uint8_t>& deflatedData, size_t inflatedSize);

} // namespace openblack::zip
