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

namespace openblack::graphics
{

struct IndexBufferHandle
{
	uint64_t id = 0;
};

struct VertexBufferHandle
{
	uint64_t id = 0;
};

struct TextureHandle
{
	uint64_t id = 0;
};

struct SamplerHandle
{
	uint64_t id = 0;
};

} // namespace openblack::graphics
