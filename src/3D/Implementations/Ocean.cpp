/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Locator.h"
#define LOCATOR_IMPLEMENTATIONS

#include <glm/vec2.hpp>

#include "Graphics/FrameBuffer.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/RendererInterface.h"
#include "Graphics/Texture2d.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"
#include "Ocean.h"

using namespace openblack;
using namespace openblack::graphics;

Ocean::Ocean() noexcept
{
	_reflectionFrameBuffer = Locator::rendererInterface::value().CreateFrameBuffer(
	    "Reflection", {1024, 1024}, TextureFormat::RGBA8, TextureFormat::Depth24Stencil8);
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

	auto& renderer = Locator::rendererInterface::value();
	const auto memVtx = std::span(reinterpret_cast<const uint8_t*>(k_Points.data()), k_Points.size() * sizeof(k_Points[0]));
	auto vertexBuffer = renderer.CreateVertexBuffer("Water", memVtx, decl);
	const auto memIdx = std::span(reinterpret_cast<const uint8_t*>(k_Indices.data()), k_Indices.size() * sizeof(k_Indices[0]));
	auto indexBuffer = renderer.CreateIndexBuffer("Water", memIdx, IndexBuffer::Type::Uint16);

	_mesh = std::make_unique<Mesh>(std::move(vertexBuffer), std::move(indexBuffer), graphics::Mesh::Topology::TriangleList);
}
