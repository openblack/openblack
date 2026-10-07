/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <map>
#include <string>
#include <string_view>

namespace openblack::psys
{

/// The names and values of a C header of enumerations the effect files name things by, such as Data/AllMeshes.h, whose
/// lines read `MSH_A_BAT_1 = 1, // comment`, or Data/SoundAction.h, whose later lines are bare names. An enumerator
/// without an initialiser takes its value as a C compiler gives it: one more than the enumerator before, or 0 first in
/// an enum. Anything that isn't a name with an optional whole-number initialiser is passed over, and the bare names
/// after it until the next whole number too, since their values can't be known.
[[nodiscard]] std::map<std::string, int32_t, std::less<>> ParseEnumHeader(std::string_view text);

} // namespace openblack::psys
