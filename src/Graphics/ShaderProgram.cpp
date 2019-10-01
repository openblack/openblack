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

#include <Graphics/ShaderProgram.h>

#include <glm/gtc/type_ptr.hpp>

#include <Common/FileSystem.h>
#include <Graphics/Texture2D.h>
#include <Game.h>

namespace openblack::graphics
{

ShaderProgram::ShaderProgram(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
	: _program(BGFX_INVALID_HANDLE)
{
	std::string suffix;
	switch (bgfx::getRendererType())
	{
		case bgfx::RendererType::Noop:
		case bgfx::RendererType::Direct3D9:
			suffix = ".dx9";
			break;
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12:
			suffix = ".dx11";
			break;
		case bgfx::RendererType::Metal:
			suffix = ".metal";
			break;
		case bgfx::RendererType::OpenGL:
			suffix = ".glsl";
			break;
		case bgfx::RendererType::OpenGLES:
			suffix = ".essl";
			break;
		case bgfx::RendererType::Vulkan:
			suffix = ".spv";
			break;
		default:
			throw std::runtime_error("Could not associate shader for renderer type.");
			break;
	}

	uint16_t numShaderUniforms = 0;
	bgfx::UniformInfo info = {};
	std::vector<bgfx::UniformHandle> uniforms;

	auto const& vertexShaderSource = Game::instance()->GetFileSystem().ReadAll(vertexSource + suffix);
	auto vs_mem = bgfx::makeRef(vertexShaderSource.data(), vertexShaderSource.size());
	auto vs_shader = bgfx::createShader(vs_mem);
	bgfx::frame();
	numShaderUniforms = bgfx::getShaderUniforms(vs_shader);
	uniforms.resize(numShaderUniforms);
	bgfx::getShaderUniforms(vs_shader, uniforms.data(), uniforms.size());
	for (uint16_t i = 0; i < numShaderUniforms; ++i)
	{
		bgfx::getUniformInfo(uniforms[i], info);
		_uniforms.emplace(std::string(info.name), uniforms[i]);
	}
	bgfx::setName(vs_shader, (name + "_vs").c_str());

	auto const& fragmentShaderSource = Game::instance()->GetFileSystem().ReadAll(fragmentSource + suffix);
	auto fs_mem = bgfx::makeRef(fragmentShaderSource.data(), fragmentShaderSource.size());
	auto fs_shader = bgfx::createShader(fs_mem);
	numShaderUniforms = bgfx::getShaderUniforms(fs_shader);
	uniforms.resize(numShaderUniforms);
	bgfx::getShaderUniforms(fs_shader, uniforms.data(), uniforms.size());
	for (uint16_t i = 0; i < numShaderUniforms; ++i)
	{
		bgfx::getUniformInfo(uniforms[i], info);
		_uniforms.emplace(std::string(info.name), uniforms[i]);
	}
	bgfx::setName(vs_shader, (name + "_fs").c_str());

	_program = bgfx::createProgram(vs_shader, fs_shader, true);
	bgfx::frame();
}

ShaderProgram::~ShaderProgram()
{
	if (bgfx::isValid(_program))
		bgfx::destroy(_program);
}

void ShaderProgram::SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture)
{
	bgfx::setTexture(bindPoint, _uniforms.at(samplerName), texture.GetNativeHandle());
}

void ShaderProgram::SetUniformValue(const char* uniformName, const void* value)
{
	bgfx::setUniform(_uniforms.at(uniformName), value);
}

} // namespace openblack::graphics
