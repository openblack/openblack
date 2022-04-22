/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Water.h"

#include "Common/FileSystem.h"
#include "Game.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexBuffer.h"

#include <glm/vec2.hpp>

using namespace openblack;
using namespace openblack::graphics;

Water::Water()
{
	_reflectionFrameBuffer =
	    std::make_unique<FrameBuffer>("Reflection", static_cast<uint16_t>(1024), static_cast<uint16_t>(1024),
	                                  graphics::Format::RGBA8, graphics::Format::Depth24Stencil8);
	createMesh();
}

void Water::createMesh()
{
	VertexDecl decl;
	decl.reserve(1);
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(2), VertexAttrib::Type::Float);

	static constexpr std::array<glm::vec2, 4> points = {
	    glm::vec2(-70000.0f, 70000.0f),
	    glm::vec2(70000.0f, 70000.0f),
	    glm::vec2(70000.0f, -70000.0f),
	    glm::vec2(-70000.0f, -70000.0f),
	};

	static constexpr std::array<uint16_t, 6> indices = {2, 1, 0, 0, 3, 2};

	auto vertexBuffer = new VertexBuffer("Water", points.data(), static_cast<uint32_t>(points.size()), decl);
	auto indexBuffer =
	    new IndexBuffer("Water", indices.data(), static_cast<uint32_t>(indices.size()), IndexBuffer::Type::Uint16);

	_mesh = std::make_unique<Mesh>(vertexBuffer, indexBuffer, graphics::Mesh::Topology::TriangleList);
}

graphics::FrameBuffer& Water::GetFrameBuffer() const
{
	return *_reflectionFrameBuffer;
}
