/* openblack - A reimplementation of Lionheads's Black & White engine
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <array>

namespace openblack {
namespace LHVM {

static const std::array<std::string, 31> Opcode_Names = {
	"END",
    "WAIT",
    "PUSH",
    "POP",
    "ADD",
    "CALL",
    "MINUS",
    "UMINUS",
    "TIMES",
    "DIVIDE",
    "MODULUS",
    "NOT",
    "AND",
	"OR",
    "EQ",
    "NEQ",
    "GEQ",
    "LEQ",
    "GT",
    "LT",
    "JUMP",
    "SLEEP",
    "EXCEPT",
    "INTCAST",
    "RUN",
    "ENDEXCEPT",
    "RETEXCEPT",
	"FAILEXCEPT",
    "BRKEXCEPT",
    "SWAP",
    "LINE"
};

}
}
