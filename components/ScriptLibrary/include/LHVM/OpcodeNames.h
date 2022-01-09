/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>

namespace openblack
{
namespace LHVM
{

static const std::array<std::string, 31> Opcode_Names = {
    "END",    "WAIT",    "PUSH", "POP",       "ADD",       "CALL",       "MINUS",     "UMINUS", "TIMES", "DIVIDE", "MODULUS",
    "NOT",    "AND",     "OR",   "EQ",        "NEQ",       "GEQ",        "LEQ",       "GT",     "LT",    "JUMP",   "SLEEP",
    "EXCEPT", "INTCAST", "RUN",  "ENDEXCEPT", "RETEXCEPT", "FAILEXCEPT", "BRKEXCEPT", "SWAP",   "LINE"};

}
} // namespace openblack
