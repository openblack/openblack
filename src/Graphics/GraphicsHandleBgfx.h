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

#include <bgfx/bgfx.h>

#include "GraphicsHandle.h"

namespace openblack::graphics
{

inline bgfx::FrameBufferHandle toBgfx(FrameBufferHandle handle)
{
	return {static_cast<uint16_t>(handle.id)};
}

inline FrameBufferHandle fromBgfx(bgfx::FrameBufferHandle handle)
{
	return {handle.idx};
}

} // namespace openblack::graphics
