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

#pragma once

#include <glm/glm.hpp>

#include <Graphics/RenderPass.h>

namespace openblack
{

namespace graphics
{
class FrameBuffer;
class Mesh;
class ShaderProgram;
}

class Water
{
  public:
	Water();
	~Water() = default;

	[[nodiscard]] glm::vec4 GetReflectionPlane() const { return glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); };

	void Draw(graphics::RenderPass viewId, const graphics::ShaderProgram& program) const;
	[[nodiscard]] graphics::FrameBuffer& GetFrameBuffer() const;
	void DebugGUI();

  private:
	void createMesh();

	std::unique_ptr<graphics::Mesh> _mesh;
	std::unique_ptr<graphics::FrameBuffer> _reflectionFrameBuffer;
};

} // namespace openblack
