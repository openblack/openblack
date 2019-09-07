/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Graphics/DebugLines.h>

#include <array>

#include <glm/gtx/transform.hpp>

#include <Graphics/OpenGL.h>
#include <Graphics/VertexBuffer.h>

using namespace openblack::Graphics;

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
	uint32_t gpuVertexArray;
	glGenVertexArrays(1, &gpuVertexArray);
	glBindVertexArray(gpuVertexArray);

	auto gpuVertexBuffer = std::make_unique<VertexBuffer>(nullptr, DEBUG_DRAW_VERTEX_BUFFER_SIZE, sizeof(Vertex), GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 3));

	glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return std::unique_ptr<DebugLines>(new DebugLines(std::move(gpuVertexBuffer), gpuVertexArray, vertexCount));
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
	glBindVertexArray(_arrayHandle);

	GLuint vbo = _gpuVertexBuffer->GetVBO();
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	program.SetUniformValue("u_model", _model);
	glDrawArrays(GL_LINES, 0, _vertexCount);
}

DebugLines::DebugLines(std::unique_ptr<VertexBuffer> &&gpuVertexBuffer, uint32_t arrayHandle, uint32_t vertexCount)
	: _gpuVertexBuffer(std::move(gpuVertexBuffer))
	, _arrayHandle(arrayHandle)
	, _vertexCount(vertexCount)
	, _model()
{
}

DebugLines::~DebugLines()
{
	glDeleteVertexArrays(1, &_arrayHandle);
}
