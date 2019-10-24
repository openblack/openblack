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

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

using namespace openblack::graphics;

namespace openblack::graphics
{

class ShaderProgram;

class Mesh
{
  public:
	enum class Topology
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
	};

	explicit Mesh(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer = nullptr, Topology topology = Topology::TriangleList);
	~Mesh();

	[[nodiscard]] const VertexBuffer& GetVertexBuffer() const;
	[[nodiscard]] const IndexBuffer& GetIndexBuffer() const;

	[[nodiscard]] Topology GetTopology() const noexcept;

	void Draw(uint8_t viewId, const openblack::graphics::ShaderProgram &program, uint64_t state, uint32_t rgba = 0) const;
	void Draw(uint8_t viewId, const openblack::graphics::ShaderProgram &program, uint32_t count, uint32_t startIndex, uint64_t state, uint32_t rgba = 0) const;

  protected:
	std::unique_ptr<VertexBuffer> _vertexBuffer;
	std::unique_ptr<IndexBuffer> _indexBuffer;

  private:
	Topology _topology;
};

} // namespace openblack
