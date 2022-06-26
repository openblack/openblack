/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <map>
#include <vector>

#include "3D/AllMeshes.h"
#include "ECS/Systems/Interfaces/RenderingSystemInterface.h"
#include "bgfx/bgfx.h"
#include "entt/entt.hpp"
#include "glm/mat4x4.hpp"

namespace openblack::ecs::systems
{

class RenderingSystem final: public RenderingSystemInterface
{
public:
	~RenderingSystem() override;
	void SetDirty() override;
	void PrepareDraw(bool drawBoundingBox, bool drawFootpaths, bool drawStreams) override;
	const RenderContext& GetContext() override { return _renderContext; }

private:
	void PrepareDrawDescs(bool drawBoundingBox);
	void PrepareDrawUploadUniforms(bool drawBoundingBox);

	RenderContext _renderContext;
};
} // namespace openblack::ecs::systems
