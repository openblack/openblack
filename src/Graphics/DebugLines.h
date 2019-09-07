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

#include <cstdint>
#include <memory>

#include <glm/glm.hpp>
#include "ShaderProgram.h"

namespace openblack
{
namespace graphics
{
class VertexBuffer;
class DebugLines
{
  public:
	static std::unique_ptr<DebugLines> CreateCross();
	std::unique_ptr<DebugLines> CreateBox(const glm::vec3& color);
	std::unique_ptr<DebugLines> CreateLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);

	virtual ~DebugLines();

	void Draw(ShaderProgram &program);

	void SetPose(const glm::vec3 &center, float size);

  protected:
	static std::unique_ptr<DebugLines> CreateDebugLines(uint32_t size, const void* data, uint32_t vertexCount);
	DebugLines(std::unique_ptr<VertexBuffer>&& gpuVertexBuffer, uint32_t arrayHandle, uint32_t vertexCount);

	std::unique_ptr<VertexBuffer> _gpuVertexBuffer;
	uint32_t _arrayHandle;
	uint32_t _vertexCount;
	glm::mat4 _model;
};

} // namespace graphics
} // namespace openblack
