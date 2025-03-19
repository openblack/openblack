/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/
#pragma once

#include <cstdint>

#include <bgfx/defines.h>

constexpr auto k_BgfxDefaultStateInvertedZ = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                                             BGFX_STATE_CULL_CW | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA;
