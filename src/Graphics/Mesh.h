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

#include <Graphics/RenderPass.h>
#include <cstdint>
#include <memory>

namespace bgfx
{
struct DynamicVertexBufferHandle;
}

namespace openblack::graphics
{

class IndexBuffer;
class ShaderProgram;
class VertexBuffer;

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

	enum SkipState : uint8_t
	{
		SkipNone = 0b00000000,
		SkipRenderState = 0b00000001,
		SkipVertexBuffer = 0b00000010,
		SkipIndexBuffer = 0b00000100,
		SkipInstanceBuffer = 0b00001000,
	};

	struct DrawDesc
	{
		graphics::RenderPass viewId;
		const openblack::graphics::ShaderProgram& program;
		uint32_t count;
		uint32_t offset;
		const bgfx::DynamicVertexBufferHandle* instanceBuffer;
		uint32_t instanceStart;
		uint32_t instanceCount;
		uint64_t state;
		uint32_t rgba;
		uint8_t skip;
		bool preserveState;
	};

	void Draw(const DrawDesc& desc) const;

protected:
	std::unique_ptr<graphics::VertexBuffer> _vertexBuffer;
	std::unique_ptr<graphics::IndexBuffer> _indexBuffer;

private:
	Topology _topology;
};

} // namespace openblack::graphics
