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

#include <imgui.h>

#include <Gui.h>
#include <3D/Water.h>
#include <Graphics/FrameBuffer.h>
#include <Graphics/Mesh.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/Texture2D.h>

using namespace openblack;

Water::Water()
{
	_reflectionFrameBuffer = std::make_unique<FrameBuffer>("Reflection", 1024, 1024, graphics::Format::RGBA8, graphics::Format::Depth24Stencil8);
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

	static const uint16_t indices[6] = { 2, 1, 0, 0, 3, 2 };

	auto vertexBuffer = new VertexBuffer("Water", points, 4, decl);
	auto indexBuffer  = new IndexBuffer("Water", indices, 6, IndexBuffer::Type::Uint16);

	_mesh = std::make_unique<Mesh>(vertexBuffer, indexBuffer, graphics::Mesh::Topology::TriangleList);
}

void Water::Draw(graphics::RenderPass viewId, const ShaderProgram& program) const
{
	program.SetTextureSampler("s_reflection", 0, _reflectionFrameBuffer->GetColorAttachment());

	uint64_t state = 0u
		| BGFX_STATE_DEFAULT
	;

	_mesh->Draw(viewId, program, state, 0);
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
