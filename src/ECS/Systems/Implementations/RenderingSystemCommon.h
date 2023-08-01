/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
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
#include "ECS/Systems/RenderingSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#warning "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

class RenderingSystemCommon: public RenderingSystemInterface
{
public:
	~RenderingSystemCommon();
	void SetDirty() override;
	void PrepareDraw(bool drawBoundingBox, bool drawFootpaths, bool drawStreams) override;
	const RenderContext& GetContext() override { return _renderContext; }

private:
	virtual void PrepareDrawDescs(bool drawBoundingBox) = 0;
	virtual void PrepareDrawUploadUniforms(bool drawBoundingBox) = 0;

protected:
	RenderContext _renderContext;
};
} // namespace openblack::ecs::systems
