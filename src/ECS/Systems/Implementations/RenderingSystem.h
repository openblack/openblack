/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/mat4x4.hpp>

#include "3D/AllMeshes.h"
#include "RenderingSystemCommon.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#pragma message("Locator interface implementations should only be included in Locator.cpp, use interface instead.")
#endif

namespace openblack::ecs::systems
{

class RenderingSystem final: public RenderingSystemCommon
{
public:
	~RenderingSystem();

private:
	void PrepareDrawDescs(bool drawBoundingBox) override;
	void PrepareDrawUploadUniforms(bool drawBoundingBox) override;
};
} // namespace openblack::ecs::systems
