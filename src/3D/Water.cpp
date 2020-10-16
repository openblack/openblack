/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
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
#include "Gui.h"

#include <glm/vec2.hpp>
#include <imgui.h>

using namespace openblack;
using namespace openblack::graphics;

Water::Water()
{
	_reflectionFrameBuffer =
	    std::make_unique<FrameBuffer>("Reflection", 1024, 1024, graphics::Format::RGBA8, graphics::Format::Depth24Stencil8);

	// water texture (256x256 RAW RGB 24bpp)
	auto& filesystem = Game::instance()->GetFileSystem();
	auto const& waterTextureData = filesystem.ReadAll(filesystem.TexturePath() / "Sky.raw");
	const uint16_t textureWidth = 256, textureHeight = 256;

	_texture = std::make_unique<Texture2D>("Water");
	_texture->Create(textureWidth, textureHeight, 1, Format::RGB8, Wrapping::ClampEdge, waterTextureData.data(),
	                 waterTextureData.size());

	createMesh();
}

void Water::createMesh()
{
	VertexDecl decl;
	decl.reserve(1);
	decl.emplace_back(VertexAttrib::Attribute::Position, 2, VertexAttrib::Type::Float);

	static const glm::vec2 points[] = {
	    glm::vec2(-70000.0f, 70000.0f),
	    glm::vec2(70000.0f, 70000.0f),
	    glm::vec2(70000.0f, -70000.0f),
	    glm::vec2(-70000.0f, -70000.0f),
	};

	static const uint16_t indices[6] = {2, 1, 0, 0, 3, 2};

	auto vertexBuffer = new VertexBuffer("Water", points, 4, decl);
	auto indexBuffer = new IndexBuffer("Water", indices, 6, IndexBuffer::Type::Uint16);

	_mesh = std::make_unique<Mesh>(vertexBuffer, indexBuffer, graphics::Mesh::Topology::TriangleList);
}

void Water::DebugGUI()
{
	ImGui::Begin("Water Debug");
	ImGui::Image(_reflectionFrameBuffer->GetColorAttachment().GetNativeHandle(), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::End();
}

graphics::FrameBuffer& Water::GetFrameBuffer() const
{
	return *_reflectionFrameBuffer;
}
