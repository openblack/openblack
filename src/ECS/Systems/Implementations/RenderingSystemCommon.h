/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <map>
#include <vector>

#include <glm/mat4x4.hpp>

#include "3D/AllMeshes.h"
#include "ECS/Systems/RenderingSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

class RenderingSystemCommon: public RenderingSystemInterface
{
public:
	~RenderingSystemCommon();
	void SetDirty() override;
	void SetLayoutDirty() override;
	void PrepareDraw(bool drawBoundingBox, bool drawFootpaths, bool drawStreams) override;
	const RenderContext& GetContext() override { return _renderContext; }

private:
	virtual void PrepareDrawDescs(bool drawBoundingBox) = 0;
	virtual void PrepareDrawUploadUniforms(bool drawBoundingBox) = 0;
	/// Uploads the instances into the draw lists as they are, when the draw lists can be kept while things only move.
	/// Returns false when they must be made again first, as they always are by default.
	virtual bool UploadUniformsKeepingDescs([[maybe_unused]] bool drawBoundingBox) { return false; }

protected:
	RenderContext _renderContext;
};
} // namespace openblack::ecs::systems
