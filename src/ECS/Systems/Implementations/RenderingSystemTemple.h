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
#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

#include "3D/AllMeshes.h"
#include "ECS/Components/Temple.h"
#include "RenderingSystemCommon.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#warning "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

class RenderingSystemTemple final: public RenderingSystemCommon
{
public:
	~RenderingSystemTemple();

private:
	void PrepareDrawDescs(bool drawBoundingBox) override;
	void PrepareDrawUploadUniforms(bool drawBoundingBox) override;
	std::set<ecs::components::TempleRoom> _loadedRooms;
};
} // namespace openblack::ecs::systems
