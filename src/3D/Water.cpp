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

	// water texture
	auto const& waterTextureData = Game::instance()->GetFileSystem().ReadAll("Data/Textures/Sky.raw");

	_texture = std::make_unique<Texture2D>("Water");
	_texture->Create(256, 256, 1, Format::RGB8, Wrapping::ClampEdge, waterTextureData.data(), waterTextureData.size());

	createMesh();
}

void Water::createMesh()
{
	VertexDecl decl;
	decl.reserve(1);
	decl.emplace_back(VertexAttrib::Attribute::Position, 2, VertexAttrib::Type::Float);

	static const glm::vec2 points[] = {
	    glm::vec2(-1.0f, 1.0f),
	    glm::vec2(1.0f, 1.0f),
	    glm::vec2(1.0f, -1.0f),
	    glm::vec2(-1.0f, -1.0f),
	};

	static const uint16_t indices[6] = {2, 1, 0, 0, 3, 2};

	auto vertexBuffer = new VertexBuffer("Water", points, 4, decl);
	auto indexBuffer = new IndexBuffer("Water", indices, 6, IndexBuffer::Type::Uint16);

	_mesh = std::make_unique<Mesh>(vertexBuffer, indexBuffer, graphics::Mesh::Topology::TriangleList);
}

void Water::Draw(graphics::RenderPass viewId, const ShaderProgram& program) const
{
	program.SetTextureSampler("s_diffuse", 0, *_texture);
	program.SetTextureSampler("s_reflection", 1, _reflectionFrameBuffer->GetColorAttachment());

	Mesh::DrawDesc desc = {
	    /*viewId =*/viewId,
	    /*program =*/program,
	    /*count =*/_mesh->GetIndexBuffer().GetCount(),
	    /*offset =*/0,
	    /*instanceBuffer =*/nullptr,
	    /*instanceStart =*/0,
	    /*instanceCount =*/1,
	    /*state =*/BGFX_STATE_DEFAULT,
	    /*rgba =*/0,
	    /*skip =*/Mesh::SkipState::SkipNone,
	    /*preserveState =*/false,
	};
	_mesh->Draw(desc);
}

void Water::DebugGUI()
{
	ImGui::Begin("Water Debug");
	ImGui::Image(_reflectionFrameBuffer->GetColorAttachment().GetNativeHandle(), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Image(_texture->GetNativeHandle(), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::End();
}

graphics::FrameBuffer& Water::GetFrameBuffer() const
{
	return *_reflectionFrameBuffer;
}
