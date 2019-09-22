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
#include <map>
#include <string>

#include <glm/glm.hpp>
#include <bgfx/bgfx.h>

namespace openblack::graphics
{
class Texture2D;

class ShaderProgram
{
  public:
	enum class Type
	{
		Vertex,
		Fragment,
		Compute,
	};

	ShaderProgram() = delete;
	ShaderProgram(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
	~ShaderProgram();

	void SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture);
	void SetUniformValue(const char* uniformName, const void* value);

	[[nodiscard]] bgfx::ProgramHandle GetRawHandle() const { return _program; }

  private:
	bgfx::ProgramHandle _program;
	std::map<std::string, bgfx::UniformHandle> _uniforms;
};

} // namespace openblack::graphics
