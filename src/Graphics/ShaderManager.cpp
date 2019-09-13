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

#include <Graphics/ShaderManager.h>

#include <3D/Camera.h>

namespace openblack::graphics
{

ShaderManager::~ShaderManager()
{
	// delete all mapped shaders
	ShaderMap::iterator iter;
	for (iter = _shaderPrograms.begin(); iter != _shaderPrograms.end(); ++iter)
		delete iter->second;

	_shaderPrograms.clear();
}

ShaderProgram* ShaderManager::LoadShader(const std::string& name, const std::string& vertexShaderFile, const std::string& fragmentShaderFile)
{
	ShaderMap::iterator i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
		return i->second;

	ShaderProgram* program = new ShaderProgram(vertexShaderFile, fragmentShaderFile);
	_shaderPrograms[name]  = program;
	return program;
}

ShaderProgram* ShaderManager::GetShader(const std::string& name)
{
	ShaderMap::iterator i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
		return i->second;

	// todo: return an empty shader?
	return nullptr;
}

void ShaderManager::SetCamera(const Camera &camera)
{
	for (auto& [key, program]: _shaderPrograms)
	{
		program->SetUniformValue("u_viewProj", camera.GetViewProjectionMatrix());
	}
}

} // namespace openblack::graphics
