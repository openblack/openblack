/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#include "Primitive.h"

#include <array>

#include <bgfx/bgfx.h>
#include <glm/vec2.hpp>

#include "Mesh.h"
#include "VertexBuffer.h"

using namespace openblack::graphics;

std::unique_ptr<Mesh> Primitive::CreatePlane()
{
	// TODO(bwrsandman): Check if vertexID can be used in shader or remove uv
	struct Vertex
	{
		glm::vec2 pos;
	};

	static const std::array<Vertex, 6> vertices = {
	    Vertex {glm::vec2 {-1.0f, -1.0f}}, Vertex {glm::vec2 {1.0f, 1.0f}},  Vertex {glm::vec2 {-1.0f, 1.0f}},

	    Vertex {glm::vec2 {-1.0f, -1.0f}}, Vertex {glm::vec2 {1.0f, -1.0f}}, Vertex {glm::vec2 {1.0f, 1.0f}},
	};

	VertexDecl decl;
	decl.reserve(2);
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(2), VertexAttrib::Type::Float);

	auto* vertexBuffer = new VertexBuffer("Plane", vertices.data(), static_cast<uint32_t>(vertices.size()), decl);
	bgfx::frame();
	auto mesh = std::make_unique<Mesh>(vertexBuffer, nullptr, Mesh::Topology::TriangleList);
	bgfx::frame();

	return mesh;
}
