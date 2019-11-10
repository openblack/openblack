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

#include <Graphics/RenderPass.h>
#include <bgfx/bgfx.h>
#include <cstdint>
#include <glm/fwd.hpp>
#include <memory>
#include <vector>

namespace openblack
{
class IStream;
class L3DMesh;

namespace graphics
{
class Mesh;
class ShaderProgram;
} // namespace graphics

enum class L3DSubMeshFlags : uint32_t
{
	Unknown1 = 1 << 0, // 0x1      (31)
};

class L3DSubMesh
{
	struct Primitive
	{
		uint32_t skinID;
		uint32_t indicesOffset;
		uint32_t indicesCount;
	};

public:
#pragma pack(push, 1)
	struct alignas(4) HeaderFlag
	{
		uint32_t lod : 3;
		uint32_t status : 6;
		uint32_t unknown1 : 3; // always 0b0101
		uint32_t isWindow : 1;
		uint32_t isPhysics : 1;
		uint32_t unknown2 : 16; // always 0, probably padding on 32 bits
	};
#pragma pack(pop)
	static_assert(sizeof(HeaderFlag) == 4);

	explicit L3DSubMesh(L3DMesh& mesh);
	~L3DSubMesh();

	void Load(IStream& stream);
	void Submit(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const graphics::ShaderProgram& program,
	            uint64_t state, uint32_t rgba = 0, bool preserveState = false) const;
	void Submit(graphics::RenderPass viewId, const bgfx::DynamicVertexBufferHandle& instanceBuffer, uint32_t instanceStart,
	            uint32_t instanceCount, const graphics::ShaderProgram& program, uint64_t state, uint32_t rgba = 0,
	            bool preserveState = false) const;

	[[nodiscard]] HeaderFlag GetFlags() const { return _flags; }
	[[nodiscard]] graphics::Mesh& GetMesh() const;
	[[nodiscard]] AxisAlignedBoundingBox GetBoundingBox() const { return _boundingBox; }

private:
	void Submit_(graphics::RenderPass viewId, const glm::mat4* modelMatrix,
	             const bgfx::DynamicVertexBufferHandle* instanceBuffer, uint32_t instanceStart, uint32_t instanceCount,
	             const graphics::ShaderProgram& program, uint64_t state, uint32_t rgba = 0, bool preserveState = false) const;

	L3DMesh& _l3dMesh;

	HeaderFlag _flags;

	std::unique_ptr<graphics::Mesh> _mesh;
	std::vector<Primitive> _primitives;

	AxisAlignedBoundingBox _boundingBox;
};
} // namespace openblack
