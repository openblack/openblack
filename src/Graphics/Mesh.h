/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <memory>
#include <optional>

#include "Graphics/IndexBuffer.h"
#include "GraphicsHandle.h"
#include "RenderPass.h"
#include "VertexBuffer.h"

namespace bgfx
{
struct DynamicVertexBufferHandle;
}

namespace openblack::graphics
{

class IndexBuffer;
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

	explicit Mesh(VertexBufferUniquePtr&& vertexBuffer, IndexBufferUniquePtr&& indexBuffer = nullptr,
	              Topology topology = Topology::TriangleList) noexcept;
	~Mesh() noexcept;

	[[nodiscard]] const VertexBuffer& GetVertexBuffer() const;
	[[nodiscard]] const IndexBuffer& GetIndexBuffer() const;
	[[nodiscard]] bool IsIndexed() const;

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
		std::optional<DynamicVertexBufferHandle> instanceBuffer;
		uint32_t instanceStart;
		uint32_t instanceCount;
		uint64_t state;
		uint32_t rgba;
		uint8_t skip;
		bool preserveState;
	};

	void Draw(const DrawDesc& desc) const;

protected:
	VertexBufferUniquePtr _vertexBuffer;
	IndexBufferUniquePtr _indexBuffer;

private:
	Topology _topology;
};

} // namespace openblack::graphics
