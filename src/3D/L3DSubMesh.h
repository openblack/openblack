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

#include <cstdint>
#include <memory>

namespace openblack
{
class IStream;
class L3DMesh;

namespace graphics
{
class VertexBuffer;
class IndexBuffer;
class ShaderProgram;
}

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
	L3DSubMesh(L3DMesh& mesh);
	~L3DSubMesh();

	void Load(IStream& stream);
	void Draw(uint8_t viewId, const L3DMesh& mesh, ShaderProgram& program, uint64_t state, uint32_t rgba = 0) const;
  private:
	L3DMesh& _l3dMesh;

	L3DSubMeshFlags _flags;

	std::unique_ptr<VertexBuffer> _vertexBuffer;
	std::unique_ptr<IndexBuffer> _indexBuffer;
	std::vector<Primitive> _primitives;
};
} // namespace openblack
