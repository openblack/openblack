/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Graphics/DebugLines.h>

namespace openblack::Entities
{
struct RenderContext
{
	RenderContext():
	    instanceUniformBuffer(BGFX_INVALID_HANDLE),
	    dirty(true),
	    hasBoundingBoxes(false) {};
	~RenderContext()
	{
		if (bgfx::isValid(instanceUniformBuffer))
		{
			bgfx::destroy(instanceUniformBuffer);
		}
	}
	std::unique_ptr<graphics::DebugLines> streams;
	std::unique_ptr<graphics::DebugLines> boundingBox;

	struct InstancedDrawDesc
	{
		InstancedDrawDesc(uint32_t offset, uint32_t count):
		    offset(offset), count(count) {}
		uint32_t offset;
		uint32_t count;
	};

	/// A list of cpu-side uniforms which is refilled at every \ref PrepareDraw.
	/// This vector will resize to the number of instances it manages
	/// but in practice, it should only grow its reserved memory.
	/// If debug bounding boxes are enabled, it will double in size to fit all
	/// bounding boxes in the second half of the list.
	std::vector<glm::mat4> instanceUniforms;
	/// Stores information for rendering which is prepared at \ref PrepareDraw.
	std::map<MeshId, const InstancedDrawDesc> instancedDrawDescs;
	/// Not an actual vertex buffer, but a dynamic general purpose buffer which
	/// stores uniform data as a GPU-side copy of \ref _instanceUniforms and
	/// which is populated in \ref PrepareDraw and consumed in \ref DrawModels.
	/// This buffer will resize if the size of \ref _instanceUniforms exceeds
	/// its allocated size. It will never shrink.
	/// The values stored are a list of uniforms (model matrix) needed for both
	/// the instances of entities and their bounding boxes.
	bgfx::DynamicVertexBufferHandle instanceUniformBuffer;

	bool dirty;
	bool hasBoundingBoxes;
};

using TownId   = int;
using StreamId = int;

struct RegistryContext
{
	RenderContext renderContext;
	std::unordered_map<StreamId, entt::entity> streams;
	std::unordered_map<TownId, entt::entity> towns;
};
} // namespace openblack::Entities
