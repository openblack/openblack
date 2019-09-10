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

#include <Graphics/DebugLines.h>

#include <array>

#include <glm/gtx/transform.hpp>

#include <Graphics/OpenGL.h>
#include <Graphics/VertexBuffer.h>
#include "Mesh.h"

using namespace openblack::graphics;

namespace
{
struct Vertex
{
  glm::vec3 pos;
  glm::vec3 col;
};
}

std::unique_ptr<DebugLines> DebugLines::CreateDebugLines(uint32_t size, const void* data, uint32_t vertexCount)
{
	VertexDecl decl;
	decl.reserve(2);
	decl.emplace_back(3, VertexAttrib::Type::Float); // position
	decl.emplace_back(3, VertexAttrib::Type::Float); // color

	auto vertexBuffer = new VertexBuffer(data, vertexCount, decl);
	auto mesh = std::make_unique<Mesh>(vertexBuffer, GL_LINES);

	return std::unique_ptr<DebugLines>(new DebugLines(std::move(mesh)));
}

std::unique_ptr<DebugLines> DebugLines::CreateCross()
{
	static const std::array<Vertex, 6> cross = {
		Vertex {glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
		Vertex {glm::vec3(-0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
		Vertex {glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
		Vertex {glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
		Vertex {glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f)},
		Vertex {glm::vec3(0.0f, 0.0f, -0.5f), glm::vec3(0.0f, 0.0f, 1.0f)},
	};

	return CreateDebugLines(cross.size() * sizeof(cross[0]), cross.data(), cross.size());
}

std::unique_ptr<DebugLines> DebugLines::CreateBox(const glm::vec3 &color)
{
	std::array<Vertex, 24> box = {
		Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), color},
		Vertex{glm::vec3(0.5f, 0.5f, -0.5f), color},
		Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), color},
		Vertex{glm::vec3(0.5f, -0.5f, -0.5f), color},
		Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), color},
		Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), color},
		Vertex{glm::vec3(0.5f, 0.5f, -0.5f), color},
		Vertex{glm::vec3(0.5f, -0.5f, -0.5f), color},

		Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), color},
		Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), color},
		Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), color},
		Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), color},
		Vertex{glm::vec3(0.5f, 0.5f, -0.5f), color},
		Vertex{glm::vec3(0.5f, 0.5f, 0.5f), color},
		Vertex{glm::vec3(0.5f, -0.5f, -0.5f), color},
		Vertex{glm::vec3(0.5f, -0.5f, 0.5f), color},

		Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), color},
		Vertex{glm::vec3(0.5f, 0.5f, 0.5f), color},
		Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), color},
		Vertex{glm::vec3(0.5f, -0.5f, 0.5f), color},
		Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), color},
		Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), color},
		Vertex{glm::vec3(0.5f, 0.5f, 0.5f), color},
		Vertex{glm::vec3(0.5f, -0.5f, 0.5f), color},
	};

	return CreateDebugLines(box.size() * sizeof(box[0]), box.data(), box.size());
}

std::unique_ptr<DebugLines> DebugLines::CreateLine(const glm::vec3& from, const glm::vec3 &to, const glm::vec3 &color)
{
	std::array<Vertex, 2> line = {
		Vertex{from, color},
		Vertex{to, color},
	};

	return CreateDebugLines(line.size() * sizeof(line[0]), line.data(), line.size());
}

void DebugLines::SetPose(const glm::vec3 &center, float size)
{
	_model = glm::translate(center) * glm::scale(glm::vec3(size, size, size));
}

void DebugLines::Draw(ShaderProgram& program)
{
	program.SetUniformValue("u_model", _model);
	_mesh->Draw();
}

DebugLines::DebugLines(std::unique_ptr<Mesh> &&mesh)
	: _mesh(std::move(mesh))
	, _model()
{
}

DebugLines::~DebugLines() = default;
