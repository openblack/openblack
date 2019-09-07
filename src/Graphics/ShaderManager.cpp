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

#include <Graphics/ShaderManager.h>

using namespace openblack::Graphics;

ShaderManager::~ShaderManager()
{
	// delete all mapped shaders
	ShaderMap::iterator iter;
	for (iter = _shaderPrograms.begin(); iter != _shaderPrograms.end(); ++iter)
		delete iter->second;

	_shaderPrograms.clear();
}

ShaderProgram* openblack::Graphics::ShaderManager::LoadShader(const std::string& name, const std::string& vertexShaderFile, const std::string& fragmentShaderFile)
{
	ShaderMap::iterator i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
		return i->second;

	ShaderProgram* program = new ShaderProgram(vertexShaderFile, fragmentShaderFile);
	_shaderPrograms[name]  = program;
	return program;
}

ShaderProgram* openblack::Graphics::ShaderManager::GetShader(const std::string& name)
{
	ShaderMap::iterator i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
		return i->second;

	// todo: return an empty shader?
	return nullptr;
}
