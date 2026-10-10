/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <tuple>

#include <entt/fwd.hpp>
#include <glm/mat4x4.hpp>

#include "Graphics/Mesh.h"

namespace openblack::ecs::systems
{
struct RenderContext
{
	RenderContext();
	~RenderContext();
	std::unique_ptr<graphics::Mesh> boundingBox;
	std::unique_ptr<graphics::Mesh> streams;
	std::unique_ptr<graphics::Mesh> footpaths;
	std::unique_ptr<graphics::Mesh> footprints;

	struct InstancedDrawDesc
	{
		InstancedDrawDesc(uint32_t offset, uint32_t count, bool morphWithTerrain)
		    : offset(offset)
		    , count(count)
		    , morphWithTerrain(morphWithTerrain)
		{
		}
		uint32_t offset;
		uint32_t count;
		bool morphWithTerrain;
	};

	/// A list of cpu-side uniforms which is refilled at every \ref PrepareDraw.
	/// This vector will resize to the number of instances it manages
	/// but in practice, it should only grow its reserved memory.
	/// If debug bounding boxes are enabled, it will double in size to fit all
	/// bounding boxes in the second half of the list.
	std::vector<glm::mat4> instanceUniforms;

	/// Tree instance data structure that combines model matrix and sway parameters
	struct TreeInstanceData
	{
		glm::mat4 modelMatrix; // Model transform
		glm::vec4 swayParams;  // x,y=direction, z=time, w=strength
	};

	/// CPU-side buffer of tree instance data (matrix + sway params)
	std::vector<TreeInstanceData> treeInstanceData;

	struct InstancedDrawKey
	{
		entt::id_type meshId;
		bool morphWithTerrain;
		bool operator<(const InstancedDrawKey& other) const
		{
			return std::tie(meshId, morphWithTerrain) < std::tie(other.meshId, other.morphWithTerrain);
		}
	};

	/// Stores information for rendering which is prepared at \ref PrepareDraw.
	std::map<InstancedDrawKey, InstancedDrawDesc> instancedDrawDescs;
	std::map<entt::id_type, InstancedDrawDesc> treeInstancedDrawDescs;

	/// Prepared CPU instance data uploaded by the renderer.
	std::vector<glm::mat4> treeInstanceUniforms;
	std::vector<glm::vec4> treeSwayParams;

	bool dirty {true};
	bool hasBoundingBoxes {false};
	uint32_t treeInstanceCount {0};
};

class RenderingSystemInterface
{
public:
	virtual void SetDirty() = 0;
	virtual void PrepareDraw(bool drawBoundingBox, bool drawFootpaths, bool drawStreams) = 0;
	virtual const RenderContext& GetContext() = 0;
	inline ~RenderingSystemInterface() = default;
};
} // namespace openblack::ecs::systems