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

#include <cassert>
#include <cstring>

#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include <Common/FileSystem.h>
#include <Graphics/OpenGL.h>
#include <Graphics/Texture2D.h>
#include <Game.h>

namespace openblack::graphics
{

ShaderProgram::ShaderProgram(const std::string& vertexSource, const std::string& fragmentSource)
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
			suffix = ".spirv";
			break;
		default:
			throw std::runtime_error("Could not associate shader for renderer type.");
			break;
	}

	auto const& vertexShaderSource   = Game::instance()->GetFileSystem().ReadAll(vertexSource + suffix);
	auto const& fragmentShaderSource = Game::instance()->GetFileSystem().ReadAll(fragmentSource + suffix);

	auto vs_mem = bgfx::makeRef(vertexShaderSource.data(), vertexShaderSource.size());
	auto fs_mem = bgfx::makeRef(fragmentShaderSource.data(), fragmentShaderSource.size());
	auto vs_shader = bgfx::createShader(vs_mem);
	auto fs_shader = bgfx::createShader(fs_mem);
	_program = bgfx::createProgram(vs_shader, fs_shader);
	bgfx::frame();
}

ShaderProgram::~ShaderProgram()
{
	bgfx::destroy(_program);
}

void ShaderProgram::SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture)
{
//	glUseProgram(_program);
//	glUniform1i(_uniforms[samplerName], bindPoint);
	texture.Bind(bindPoint);
}

void ShaderProgram::SetUniformValue(const char* uniformName, int value)
{
//	glUseProgram(_program);
//	glUniform1i(_uniforms[uniformName], value);
}

void ShaderProgram::SetUniformValue(const char* uniformName, float value)
{
//	glUseProgram(_program);
//	glUniform1f(_uniforms[uniformName], value);
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::vec2& v)
{
//	glUseProgram(_program);
//	glUniform2fv(_uniforms[uniformName], 1, glm::value_ptr(v));
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::vec3& v)
{
//	glUseProgram(_program);
//	glUniform3fv(_uniforms[uniformName], 1, glm::value_ptr(v));
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::vec4& v)
{
//	glUseProgram(_program);
//	glUniform4fv(_uniforms[uniformName], 1, glm::value_ptr(v));
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::mat3& m)
{
//	glUseProgram(_program);
//	glUniformMatrix3fv(_uniforms[uniformName], 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::mat4& m)
{
//	glUseProgram(_program);
//	glUniformMatrix4fv(_uniforms[uniformName], 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderProgram::SetUniformValue(const char* uniformName, size_t count, const glm::mat4* m)
{
//	glUseProgram(_program);
//	glUniformMatrix4fv(_uniforms[uniformName], count, GL_FALSE, glm::value_ptr(m[0]));
}

uint32_t ShaderProgram::createSubShader(Type type, const std::string& source)
{
	static const std::array<GLenum, 3> types = {
		GL_VERTEX_SHADER,
		GL_FRAGMENT_SHADER,
		0x91B9, // GL_COMPUTE_SHADER, core in 4.3
	};
	GLuint shader = glCreateShader(types[static_cast<uint32_t>(type)]);

	auto glsource = reinterpret_cast<const GLchar*>(source.c_str());
	glShaderSource(shader, 1, &glsource, nullptr);
	glCompileShader(shader);

	GLint compileStatus = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus == GL_FALSE)
	{
		GLint infoLogLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

		char* infoLog = new char[infoLogLen];
		glGetShaderInfoLog(shader, infoLogLen, &infoLogLen, infoLog);
		spdlog::error("There are compile errors: {}", infoLog);
		delete[] infoLog;

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

} // namespace openblack::graphics
