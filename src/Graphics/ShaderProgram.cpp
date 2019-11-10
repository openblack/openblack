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

#include <Common/FileSystem.h>
#include <Game.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/Texture2D.h>
#include <glm/gtc/type_ptr.hpp>

namespace openblack::graphics
{

ShaderProgram::ShaderProgram(const std::string& name, bgfx::ShaderHandle&& vertexShader, bgfx::ShaderHandle&& fragmentShader)
    : _program(BGFX_INVALID_HANDLE)
{
	uint16_t numShaderUniforms = 0;
	bgfx::UniformInfo info = {};
	std::vector<bgfx::UniformHandle> uniforms;

	numShaderUniforms = bgfx::getShaderUniforms(vertexShader);
	uniforms.resize(numShaderUniforms);
	bgfx::getShaderUniforms(vertexShader, uniforms.data(), uniforms.size());
	for (uint16_t i = 0; i < numShaderUniforms; ++i)
	{
		bgfx::getUniformInfo(uniforms[i], info);
		_uniforms.emplace(std::string(info.name), uniforms[i]);
	}

	numShaderUniforms = bgfx::getShaderUniforms(fragmentShader);
	uniforms.resize(numShaderUniforms);
	bgfx::getShaderUniforms(fragmentShader, uniforms.data(), uniforms.size());
	for (uint16_t i = 0; i < numShaderUniforms; ++i)
	{
		bgfx::getUniformInfo(uniforms[i], info);
		_uniforms.emplace(std::string(info.name), uniforms[i]);
	}

	_program = bgfx::createProgram(vertexShader, fragmentShader, true);
	bgfx::frame();
}

ShaderProgram::~ShaderProgram()
{
	if (bgfx::isValid(_program))
		bgfx::destroy(_program);
}

void ShaderProgram::SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture) const
{
	bgfx::setTexture(bindPoint, _uniforms.at(samplerName), texture.GetNativeHandle());
}

void ShaderProgram::SetUniformValue(const char* uniformName, const void* value) const
{
	bgfx::setUniform(_uniforms.at(uniformName), value);
}

} // namespace openblack::graphics
