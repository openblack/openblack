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
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>

#include "3D/AllMeshes.h"
#include "RenderingSystemCommon.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
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
	bool UploadUniformsKeepingDescs(bool drawBoundingBox) override;
	void PrepareTreeDrawDescs(bool drawBoundingBox);
	/// Each returns false when an instance had no room left in its mesh's draw list, which must then be made again
	bool UploadInstances(bool drawBoundingBox);
	bool UploadTreeInstances(bool drawBoundingBox);

	/// Where the next instance of a mesh goes in the instance buffer
	struct InstanceSlots
	{
		uint32_t offset;
		uint32_t count;
		uint32_t filled;
		bool perEntity;
		/// The height of the mesh's bounding box, for the trees
		float height;
	};
	std::unordered_map<entt::id_type, InstanceSlots> _instanceSlots;
	std::unordered_map<entt::id_type, InstanceSlots> _treeSlots;
};
} // namespace openblack::ecs::systems
