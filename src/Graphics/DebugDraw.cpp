/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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

#include <Graphics/DebugDraw.h>
#include <Graphics/OpenGL.h>

using namespace OpenBlack::Graphics;

GLuint DebugDraw::gpuVertexArray = 0;
std::unique_ptr<VertexBuffer> DebugDraw::gpuVertexBuffer;
DebugDraw::Vertex DebugDraw::vertexBuffer[DEBUG_DRAW_VERTEX_BUFFER_SIZE];
int DebugDraw::vertexCount = 0;

void DebugDraw::Init()
{
	glGenVertexArrays(1, &gpuVertexArray);
	glBindVertexArray(gpuVertexArray);

	gpuVertexBuffer = std::make_unique<VertexBuffer>(nullptr, DEBUG_DRAW_VERTEX_BUFFER_SIZE, sizeof(Vertex), GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(sizeof(float) * 3));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DebugDraw::Shutdown()
{
	glDeleteVertexArrays(1, &gpuVertexArray);
	gpuVertexBuffer.reset();
}

void DebugDraw::Line(glm::vec3 from, glm::vec3 to, glm::vec3 color)
{
	Vertex& v0 = vertexBuffer[vertexCount++];
	Vertex& v1 = vertexBuffer[vertexCount++];

	v0.pos = from;
	v0.col = color;

	v1.pos = to;
	v1.col = color;
}

void DebugDraw::DrawDebugLines()
{
	if (vertexCount == 0)
		return;

	glBindVertexArray(gpuVertexArray);

	GLuint vbo = gpuVertexBuffer->GetVBO();
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(Vertex), vertexBuffer);

	glDrawArrays(GL_LINES, 0, vertexCount);

	// reset
	vertexCount = 0;
}
