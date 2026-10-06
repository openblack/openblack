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

#include <span>
#include <string_view>
#include <vector>

namespace openblack::lhvm
{

/// The type of a native function's argument or result, as the decompiler needs it to print a call
enum class ArgType : uint8_t
{
	None,   ///< No value (only used as a result type)
	Int,    ///< An integer, usually an enum constant
	Float,  ///< A number
	Coord,  ///< A position: three stack slots
	Object, ///< A game object reference
	Bool,   ///< A truth value
	String, ///< An offset into the program's string data, printed as a string literal
	Any,    ///< A value of any type
	VarArgs ///< A run of values whose count is given by the argument that follows it
};

struct NativeParam
{
	std::string_view name;
	ArgType type {ArgType::Any};
	/// For an integer: the script header enum its constants belong to ("HELP_TEXT*" for every text group), if known
	std::string_view enumName {};
};

/// The signature of one native (CHL API) function, indexed by its number in the call instruction
struct NativeSignature
{
	std::string_view name;
	/// Stack slots taken; negative when the function takes a variable number of arguments
	int32_t stackIn {0};
	/// Stack slots left as the result
	uint32_t stackOut {0};
	ArgType returnType {ArgType::None};
	/// Parameters in source order. Empty with a non-zero stackIn means the types are not known.
	std::vector<NativeParam> params;
};

/// The native functions of Black & White, mirroring the game's CHL API table (names, stack use and argument types)
[[nodiscard]] std::span<const NativeSignature> DefaultNativeSignatures();

} // namespace openblack::lhvm
