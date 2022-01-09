/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "DebugLines.h"

#include "Mesh.h"
#include "VertexBuffer.h"

#include <array>

namespace openblack::graphics
{

std::unique_ptr<DebugLines> DebugLines::CreateDebugLines(const Vertex* data, uint32_t vertexCount)
{
	VertexDecl decl;
	decl.reserve(2);
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(4), VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::Color0, static_cast<uint8_t>(4), VertexAttrib::Type::Float);

	auto vertexBuffer = new VertexBuffer("DebugLines", data, vertexCount, decl);
	bgfx::frame();
	auto mesh = std::make_unique<Mesh>(vertexBuffer, nullptr, Mesh::Topology::LineList);
	bgfx::frame();

	return std::unique_ptr<DebugLines>(new DebugLines(std::move(mesh)));
}

std::unique_ptr<DebugLines> DebugLines::CreateCross()
{
	static const std::array<Vertex, 6> cross = {
	    Vertex {glm::vec4(0.5f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
	    Vertex {glm::vec4(-0.5f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
	    Vertex {glm::vec4(0.0f, 0.5f, 0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
	    Vertex {glm::vec4(0.0f, -0.5f, 0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
	    Vertex {glm::vec4(0.0f, 0.0f, 0.5f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)},
	    Vertex {glm::vec4(0.0f, 0.0f, -0.5f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)},
	};

	return CreateDebugLines(cross.data(), static_cast<uint32_t>(cross.size()));
}

std::unique_ptr<DebugLines> DebugLines::CreateBox(const glm::vec4& color)
{
	std::array<Vertex, 24> box = {
	    Vertex {glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), color},  Vertex {glm::vec4(0.5f, 0.5f, -0.5f, 1.0f), color},
	    Vertex {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), color}, Vertex {glm::vec4(0.5f, -0.5f, -0.5f, 1.0f), color},
	    Vertex {glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), color},  Vertex {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), color},
	    Vertex {glm::vec4(0.5f, 0.5f, -0.5f, 1.0f), color},   Vertex {glm::vec4(0.5f, -0.5f, -0.5f, 1.0f), color},

	    Vertex {glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), color},  Vertex {glm::vec4(-0.5f, 0.5f, 0.5f, 1.0f), color},
	    Vertex {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), color}, Vertex {glm::vec4(-0.5f, -0.5f, 0.5f, 1.0f), color},
	    Vertex {glm::vec4(0.5f, 0.5f, -0.5f, 1.0f), color},   Vertex {glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), color},
	    Vertex {glm::vec4(0.5f, -0.5f, -0.5f, 1.0f), color},  Vertex {glm::vec4(0.5f, -0.5f, 0.5f, 1.0f), color},

	    Vertex {glm::vec4(-0.5f, 0.5f, 0.5f, 1.0f), color},   Vertex {glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), color},
	    Vertex {glm::vec4(-0.5f, -0.5f, 0.5f, 1.0f), color},  Vertex {glm::vec4(0.5f, -0.5f, 0.5f, 1.0f), color},
	    Vertex {glm::vec4(-0.5f, 0.5f, 0.5f, 1.0f), color},   Vertex {glm::vec4(-0.5f, -0.5f, 0.5f, 1.0f), color},
	    Vertex {glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), color},    Vertex {glm::vec4(0.5f, -0.5f, 0.5f, 1.0f), color},
	};

	return CreateDebugLines(box.data(), static_cast<uint32_t>(box.size()));
}

std::unique_ptr<DebugLines> DebugLines::CreateLine(const glm::vec4& from, const glm::vec4& to, const glm::vec4& color)
{
	std::array<Vertex, 2> line = {
	    Vertex {from, color},
	    Vertex {to, color},
	};

	return CreateDebugLines(line.data(), static_cast<uint32_t>(line.size()));
}

DebugLines::DebugLines(std::unique_ptr<Mesh>&& mesh)
    : _mesh(std::move(mesh))
{
}

DebugLines::~DebugLines() = default;

} // namespace openblack::graphics
