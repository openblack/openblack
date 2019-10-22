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
	void Submit(uint8_t viewId, ShaderProgram& program, uint64_t state, uint32_t rgba = 0, bool preserveState = false) const;

	uint8_t GetLOD() const { return static_cast<uint8_t>(_flags & 0x7); } // 29-32 (3)
	uint8_t GetStatus() const { return static_cast<uint8_t>((_flags >> 3) & 0x3F); } // 22-28 (6)
	bool IsWindow() const { return static_cast<bool>(_flags & 0x1000); } // 19
	bool IsPhysics() const { return static_cast<bool>(_flags & 0x2000); } // 18
  private:
	L3DMesh& _l3dMesh;

	uint32_t _flags;

	std::unique_ptr<VertexBuffer> _vertexBuffer;
	std::unique_ptr<IndexBuffer> _indexBuffer;
	std::vector<Primitive> _primitives;
};
} // namespace openblack
