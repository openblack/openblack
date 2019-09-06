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

#pragma once

#include <Graphics/VertexBuffer.h>
#include <glm/glm.hpp>
#include <vector>

#define DEBUG_DRAW_VERTEX_BUFFER_SIZE 4096

namespace OpenBlack
{
namespace Graphics
{
class DebugDraw
{
  public:
	static void Init();
	static void Shutdown();
	static void DrawDebugLines();

	/* methods to add to draw queue */
	static void Line(glm::vec3 point1, glm::vec3 point2, glm::vec3 color);
	static void Box(glm::vec3 center, glm::vec3 color, glm::vec3 size);
	static void Cross(const glm::vec3 &center, float size);

  protected:
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 col;
	};

	static int vertexCount;
	static Vertex vertexBuffer[DEBUG_DRAW_VERTEX_BUFFER_SIZE];

	static GLuint gpuVertexArray;
	static std::unique_ptr<VertexBuffer> gpuVertexBuffer;
};

} // namespace Graphics
} // namespace OpenBlack
