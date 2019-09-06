/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <Graphics/Mesh.h>
#include <Graphics/VertexBuffer.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/ShaderProgram.h>

namespace OpenBlack
{
class IStream;

enum class L3DSubMeshFlags : uint32_t
{
	Unknown1 = 1 << 0, // 0x1      (31)
};

class L3DSubMesh
{
  public:
	L3DSubMesh();
	~L3DSubMesh();

	void Load(IStream& stream);
	void Draw(ShaderProgram& program) const;
  private:
	L3DSubMeshFlags _flags;

	GLuint _vertexArray;
	std::unique_ptr<VertexBuffer> _vertexBuffer;
	std::unique_ptr<IndexBuffer> _indexBuffer;
};
} // namespace OpenBlack
