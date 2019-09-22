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
{
	_program = glCreateProgram();

	// lazy assert, todo: better error handling
	assert(_program != 0);

	auto const& vertexShaderSource   = Game::instance()->GetFileSystem().ReadAll(vertexSource);
	auto const& fragmentShaderSource = Game::instance()->GetFileSystem().ReadAll(fragmentSource);

	GLuint vertexShader   = createSubShader(Type::Vertex, std::string(reinterpret_cast<const char*>(vertexShaderSource.data()), vertexShaderSource.size()));
	GLuint fragmentShader = createSubShader(Type::Fragment, std::string(reinterpret_cast<const char*>(fragmentShaderSource.data()), fragmentShaderSource.size()));

	// lazy assert, todo: better error handling
	assert(vertexShader != 0 && fragmentShader != 0);

	glAttachShader(_program, vertexShader);
	glAttachShader(_program, fragmentShader);
	glLinkProgram(_program);

	GLint linkStatus = GL_FALSE;
	glGetProgramiv(_program, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == GL_FALSE)
	{
		GLint infoLogLen = 0;
		glGetShaderiv(_program, GL_INFO_LOG_LENGTH, &infoLogLen);

		char* infoLog = new char[infoLogLen];
		glGetProgramInfoLog(_program, infoLogLen, &infoLogLen, infoLog);
		spdlog::error("There are linking errors: {}", infoLog);
		delete[] infoLog;

		glDeleteProgram(_program);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return;
	}

	// we can delete these now they exist in the program
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	int uniformCount = -1;
	glGetProgramiv(_program, GL_ACTIVE_UNIFORMS, &uniformCount);
	for (int i = 0; i < uniformCount; i++)
	{
		int name_len = -1, num = -1;
		GLenum type = GL_ZERO;
		char name[64];
		glGetActiveUniform(_program, GLuint(i), sizeof(name) - 1, &name_len, &num, &type, name);
		name[name_len] = 0;

		_uniforms[std::string(name)] = glGetUniformLocation(_program, name);
	}
}

ShaderProgram::~ShaderProgram()
{
	if (_program != 0)
		glDeleteProgram(_program);
}

void ShaderProgram::SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture)
{
	glUseProgram(_program);
	glUniform1i(_uniforms[samplerName], bindPoint);
	texture.Bind(bindPoint);
}

void ShaderProgram::SetUniformValue(const char* uniformName, int value)
{
	glUseProgram(_program);
	glUniform1i(_uniforms[uniformName], value);
}

void ShaderProgram::SetUniformValue(const char* uniformName, float value)
{
	glUseProgram(_program);
	glUniform1f(_uniforms[uniformName], value);
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::vec2& v)
{
	glUseProgram(_program);
	glUniform2fv(_uniforms[uniformName], 1, glm::value_ptr(v));
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::vec3& v)
{
	glUseProgram(_program);
	glUniform3fv(_uniforms[uniformName], 1, glm::value_ptr(v));
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::vec4& v)
{
	glUseProgram(_program);
	glUniform4fv(_uniforms[uniformName], 1, glm::value_ptr(v));
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::mat3& m)
{
	glUseProgram(_program);
	glUniformMatrix3fv(_uniforms[uniformName], 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderProgram::SetUniformValue(const char* uniformName, const glm::mat4& m)
{
	glUseProgram(_program);
	glUniformMatrix4fv(_uniforms[uniformName], 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderProgram::SetUniformValue(const char* uniformName, size_t count, const glm::mat4* m)
{
	glUseProgram(_program);
	glUniformMatrix4fv(_uniforms[uniformName], count, GL_FALSE, glm::value_ptr(m[0]));
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
