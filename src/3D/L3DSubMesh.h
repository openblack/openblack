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

#pragma once

#include "AxisAlignedBoundingBox.h"
#include "../Graphics/RenderPass.h"

#include <bgfx/bgfx.h>
#include <glm/fwd.hpp>
#include <l3d_file.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace openblack
{
class L3DMesh;

namespace graphics
{
class Mesh;
class ShaderProgram;
} // namespace graphics

class L3DSubMesh
{
	struct Primitive
	{
		uint32_t skinID;
		uint32_t indicesOffset;
		uint32_t indicesCount;
	};

public:
	explicit L3DSubMesh(L3DMesh& mesh);
	~L3DSubMesh();

	void Load(const l3d::L3DFile& l3d, uint32_t meshIndex);
	void Submit(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const graphics::ShaderProgram& program,
	            uint64_t state, uint32_t rgba = 0, bool preserveState = false) const;
	void Submit(graphics::RenderPass viewId, const bgfx::DynamicVertexBufferHandle& instanceBuffer, uint32_t instanceStart,
	            uint32_t instanceCount, const graphics::ShaderProgram& program, uint64_t state, uint32_t rgba = 0,
	            bool preserveState = false) const;

	[[nodiscard]] openblack::l3d::L3DSubmeshHeader::Flags GetFlags() const { return _flags; }
	[[nodiscard]] graphics::Mesh& GetMesh() const;
	[[nodiscard]] AxisAlignedBoundingBox GetBoundingBox() const { return _boundingBox; }

private:
	void Submit_(graphics::RenderPass viewId, const glm::mat4* modelMatrix,
	             const bgfx::DynamicVertexBufferHandle* instanceBuffer, uint32_t instanceStart, uint32_t instanceCount,
	             const graphics::ShaderProgram& program, uint64_t state, uint32_t rgba = 0, bool preserveState = false) const;

	L3DMesh& _l3dMesh;

	openblack::l3d::L3DSubmeshHeader::Flags _flags;

	std::unique_ptr<graphics::Mesh> _mesh;
	std::vector<Primitive> _primitives;

	AxisAlignedBoundingBox _boundingBox;
};
} // namespace openblack
