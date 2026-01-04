/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "Ocean.h"

#include <bgfx/bgfx.h>
#include <glm/vec2.hpp>

#include "Graphics/FrameBuffer.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexBuffer.h"

using namespace openblack;
using namespace openblack::graphics;

Ocean::Ocean() noexcept
{
	_reflectionFrameBuffer =
	    std::make_unique<FrameBuffer>("Reflection", static_cast<uint16_t>(1024), static_cast<uint16_t>(1024),
	                                  graphics::TextureFormat::RGBA8, graphics::TextureFormat::Depth24Stencil8);
	CreateMesh();
}
Ocean::~Ocean() noexcept = default;

void Ocean::CreateMesh()
{
	VertexDecl decl;
	decl.reserve(1);
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(2), VertexAttrib::Type::Float);

	static constexpr std::array<glm::vec2, 4> k_Points = {
	    glm::vec2(-70000.0f, 70000.0f),
	    glm::vec2(70000.0f, 70000.0f),
	    glm::vec2(70000.0f, -70000.0f),
	    glm::vec2(-70000.0f, -70000.0f),
	};

	static constexpr std::array<uint16_t, 6> k_Indices = {2, 1, 0, 0, 3, 2};

	const auto* mem = bgfx::makeRef(k_Points.data(), static_cast<uint32_t>(k_Points.size() * sizeof(k_Points[0])));
	auto* vertexBuffer = new VertexBuffer("Water", mem, decl);
	auto* indexBuffer =
	    new IndexBuffer("Water", k_Indices.data(), static_cast<uint32_t>(k_Indices.size()), IndexBuffer::Type::Uint16);

	_mesh = std::make_unique<Mesh>(vertexBuffer, indexBuffer, graphics::Mesh::Topology::TriangleList);
}
