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

#include <Graphics/ShaderProgram.h>
#include <map>
#include <string>

namespace openblack
{

class Camera;

namespace graphics
{

class ShaderManager
{
  public:
	ShaderManager() = default;
	~ShaderManager();

	ShaderProgram* LoadShader(const std::string& name, const std::string& vertexShaderFile, const std::string& fragmentShaderFile);
	ShaderProgram* GetShader(const std::string& name);

	void SetCamera(uint8_t viewId, const Camera &camera);

  private:
	typedef std::map<std::string, ShaderProgram*> ShaderMap;

	ShaderMap _shaderPrograms;
};

} // namespace graphics
} // namespace openblack
