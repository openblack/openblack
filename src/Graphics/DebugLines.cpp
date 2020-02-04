/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "DebugLines.h"

#include "Mesh.h"
#include "VertexBuffer.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <array>

namespace openblack::graphics
{

std::unique_ptr<DebugLines> DebugLines::CreateDebugLines(const Vertex* data, uint32_t vertexCount)
{
	VertexDecl decl;
	decl.reserve(2);
	decl.emplace_back(VertexAttrib::Attribute::Position, 4, VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::Color0, 4, VertexAttrib::Type::Float);

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

	return CreateDebugLines(cross.data(), cross.size());
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

	return CreateDebugLines(box.data(), box.size());
}

std::unique_ptr<DebugLines> DebugLines::CreateLine(const glm::vec4& from, const glm::vec4& to, const glm::vec4& color)
{
	std::array<Vertex, 2> line = {
	    Vertex {from, color},
	    Vertex {to, color},
	};

	return CreateDebugLines(line.data(), line.size());
}

void DebugLines::SetPose(const glm::vec3& center, const glm::vec3& size)
{
	_model = glm::translate(center) * glm::scale(size);
}

void DebugLines::Draw(RenderPass viewId, const ShaderProgram& program) const
{
	bgfx::setTransform(&_model);

	Mesh::DrawDesc desc = {
	    /*viewId =*/viewId,
	    /*program =*/program,
	    /*start =*/_mesh->GetVertexBuffer().GetCount(),
	    /*offset =*/0,
	    /*instanceBuffer =*/nullptr,
	    /*instanceStart =*/0,
	    /*instanceCount =*/1,
	    /*state =*/0u | BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES,
	    /*rgba =*/0,
	    /*skip =*/Mesh::SkipState::SkipNone,
	    /*preserveState =*/false,
	};
	_mesh->Draw(desc);
}

void DebugLines::Draw(RenderPass viewId, const bgfx::DynamicVertexBufferHandle& instanceBuffer, uint32_t instanceStart,
                      uint32_t instanceCount, const ShaderProgram& program) const
{
	Mesh::DrawDesc desc = {
	    /*viewId =*/viewId,
	    /*program =*/program,
	    /*count =*/_mesh->GetVertexBuffer().GetCount(),
	    /*offset =*/0,
	    /*instanceBuffer =*/&instanceBuffer,
	    /*instanceStart =*/instanceStart,
	    /*instanceCount =*/instanceCount,
	    /*state =*/0u | BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES,
	    /*rgba =*/0,
	    /*skip =*/Mesh::SkipState::SkipNone,
	    /*preserveState =*/false,
	};
	_mesh->Draw(desc);
}

DebugLines::DebugLines(std::unique_ptr<Mesh>&& mesh): _mesh(std::move(mesh)), _model(1.0f) {}

DebugLines::~DebugLines() = default;

} // namespace openblack::graphics
