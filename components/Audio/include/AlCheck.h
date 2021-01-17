/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>

#include <string>

namespace openblack::audio
{
void AlErrorCheck(std::string message, std::string file, uint64_t line);
#define alCheckCall(FUNCTION_CALL) \
	FUNCTION_CALL;                 \
	AlErrorCheck(#FUNCTION_CALL, __FILE__, __LINE__)
} // namespace openblack::audio
