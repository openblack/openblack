/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// https://stackoverflow.com/questions/40062883/how-to-use-a-macro-in-an-include-directive

// Turn off formatting because it adds spaces which break the stringifying
// clang-format off

#ifdef _MSC_VER
// Disable warning about not enough arguments for macro evocation EXPAND when SHADER_DIR is undefined
#pragma warning( push )
#pragma warning( disable : 4003 )
#endif

// NOLINTNEXTLINE(bugprone-macro-parentheses): The / is not an operand and this is not a macro
#define GENERATED_SHADERS_DIR generated/shaders/

#ifndef SHADER_DIR
#define SHADER_DIR
#endif

#define STRINGIFY_MACRO(x) STR(x)
#define STR(x) #x
#define EXPAND(x) x
#define CONCAT(n1, n2) STRINGIFY_MACRO(EXPAND(n1)EXPAND(n2))
#define CONCAT5(n1, n2, n3, n4, n5) STRINGIFY_MACRO(EXPAND(n1)EXPAND(n2)EXPAND(n3)EXPAND(n4)EXPAND(n5))

// Concatenate the four elements of your path.
#include CONCAT5(GENERATED_SHADERS_DIR, SHADER_DIR,glsl/, SHADER_NAME,.sc.bin.h)
#include CONCAT5(GENERATED_SHADERS_DIR, SHADER_DIR,spv/, SHADER_NAME,.sc.bin.h)
#include CONCAT5(GENERATED_SHADERS_DIR, SHADER_DIR,essl/, SHADER_NAME,.sc.bin.h)
#if defined(_WIN32)
#include CONCAT5(GENERATED_SHADERS_DIR, SHADER_DIR,dx11/, SHADER_NAME,.sc.bin.h)
#endif //  defined(_WIN32)
#if __APPLE__
#include CONCAT5(GENERATED_SHADERS_DIR, SHADER_DIR,mtl/, SHADER_NAME,.sc.bin.h)
#endif // __APPLE__

#undef SHADER_DIR
#undef SHADER_NAME

#ifdef _MSC_VER
#pragma warning( pop )
#endif

// clang-format on
