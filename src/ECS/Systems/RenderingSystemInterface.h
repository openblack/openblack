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
#include <vector>

#include <entt/fwd.hpp>
#include <glm/mat4x4.hpp>

#include "Graphics/GraphicsHandle.h"
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
		InstancedDrawDesc(uint32_t offset, uint32_t count, bool morphWithTerrain, bool castsShadow)
		    : offset(offset)
		    , count(count)
		    , morphWithTerrain(morphWithTerrain)
		    , castsShadow(castsShadow)
		{
		}
		uint32_t offset;
		uint32_t count;
		bool morphWithTerrain;
		/// The instances cast their shadows onto the land (see graphics::ObjectShadows)
		bool castsShadow;
		/// The instances aren't seen in the reflection pass
		bool hiddenFromReflection {false};
		/// The instances show the reflection pass through them, as the temple's floor does
		bool showsReflection {false};
		/// Only the submeshes with joints are drawn, as WorldRoom::DrawDoors draws the main room's doors
		bool onlyJoints {false};
		/// The submeshes with joints are drawn only while their joints turn: the side rooms' copies of their doors
		bool hideShutJoints {false};
		/// The instances are of a temple room the player isn't in, which gives way where it overlaps the room they are in
		bool behindCurrentRoom {false};
		/// The instances are drawn as each primitive's material says: blended and writing depth or not
		bool materialBlending {false};
		/// The instances are all blended by their materials, so they are drawn after the opaque ones
		bool translucent {false};
		/// How far the instances' textures have slid across them
		glm::vec2 uvOffset {0.0f};
		/// Textures some of the submeshes are drawn with in place of their skins, by submesh: the temple's scrolls
		std::vector<std::pair<uint32_t, graphics::TextureHandle>> subMeshTextures;
		/// Colours added to some of the submeshes, by submesh: the temple's controls glowing under the cursor
		std::vector<std::pair<uint32_t, glm::vec3>> subMeshGlows;
		/// Submeshes left undrawn: the temple's buttons draw one of each of their pairs
		std::vector<uint32_t> hiddenSubMeshes;
	};

	/// A list of cpu-side uniforms which is refilled at every \ref PrepareDraw.
	/// This vector will resize to the number of instances it manages
	/// but in practice, it should only grow its reserved memory.
	/// If debug bounding boxes are enabled, it will double in size to fit all
	/// bounding boxes in the second half of the list.
	std::vector<glm::mat4> instanceUniforms;

	/// Tree instance data: the tree's matrix, swaying or bent
	struct TreeInstanceData
	{
		glm::mat4 modelMatrix;
	};

	/// CPU-side buffer of tree instance data
	std::vector<TreeInstanceData> treeInstanceData;

	/// Stores information for rendering which is prepared at \ref PrepareDraw.
	std::map<entt::id_type, InstancedDrawDesc> instancedDrawDescs;
	std::map<entt::id_type, InstancedDrawDesc> treeInstancedDrawDescs;
	/// Bone matrices of animated meshes, refilled at every \ref PrepareDraw. Boned meshes without an entry are drawn
	/// in their rest pose.
	std::map<entt::id_type, std::vector<glm::mat4>> animatedBoneMatrices;
	/// The hand is scaled by a negative factor to mirror it, which turns its faces round
	bool handMirrored {false};

	/// Not an actual vertex buffer, but a dynamic general purpose buffer which
	/// stores uniform data as a GPU-side copy of \ref _instanceUniforms and
	/// which is populated in \ref PrepareDraw and consumed in \ref DrawModels.
	/// This buffer will resize if the size of \ref _instanceUniforms exceeds
	/// its allocated size. It will never shrink.
	/// The values stored are a list of uniforms (model matrix) needed for both
	/// the instances of entities and their bounding boxes.
	graphics::DynamicVertexBufferHandle instanceUniformBuffer;

	/// Dynamic buffer for tree instance data (contains both matrix and sway params)
	graphics::DynamicVertexBufferHandle treeInstanceUniformBuffer;

	/// Kept for backward compatibility, will be removed once shader is updated
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
