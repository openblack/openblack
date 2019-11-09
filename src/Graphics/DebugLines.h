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

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <memory>
#include <Graphics/RenderPass.h>

namespace openblack::graphics
{
class Mesh;
class ShaderProgram;

class DebugLines
{
  public:
	struct Vertex
	{
		glm::vec4 pos;
		glm::vec4 col;
	};

	static std::unique_ptr<DebugLines> CreateCross();
	static std::unique_ptr<DebugLines> CreateBox(const glm::vec4& color);
	static std::unique_ptr<DebugLines> CreateLine(const glm::vec4& from, const glm::vec4& to, const glm::vec4& color);
	static std::unique_ptr<DebugLines> CreateDebugLines(const Vertex* data, uint32_t vertexCount);

	virtual ~DebugLines();

	void Draw(graphics::RenderPass viewId, const ShaderProgram& program) const;
	void Draw(graphics::RenderPass viewId, const bgfx::DynamicVertexBufferHandle& instanceBuffer, uint32_t instanceStart, uint32_t instanceCount, const ShaderProgram& program) const;

	void SetPose(const glm::vec3& center, const glm::vec3& size);

  protected:
	explicit DebugLines(std::unique_ptr<Mesh>&& mesh);

	std::unique_ptr<Mesh> _mesh;
	glm::mat4 _model;
};

} // namespace openblack::graphics
