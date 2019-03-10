/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <Graphics/ShaderProgram.h>

using OpenBlack::Graphics::ShaderProgram;

ShaderProgram::ShaderProgram(const std::string &vertexSource, const std::string &fragmentSource)
{
	_shaderProgram = glCreateProgram();

	// lazy assert, todo: better error handling
	assert(_shaderProgram != 0);

	_shaderVertex = createSubShader(GL_VERTEX_SHADER, vertexSource);
	_shaderFragment = createSubShader(GL_FRAGMENT_SHADER, fragmentSource);

	// lazy assert, todo: better error handling
	assert(_shaderVertex != 0 && _shaderFragment != 0);

	glAttachShader(_shaderProgram, _shaderVertex);
	glAttachShader(_shaderProgram, _shaderFragment);
	glLinkProgram(_shaderProgram);

	GLint linkStatus = GL_FALSE;
	glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == GL_FALSE)
	{
		GLint infoLogLen = 0;
		glGetShaderiv(_shaderProgram, GL_INFO_LOG_LENGTH, &infoLogLen);

		char* infoLog = new char[infoLogLen];
		glGetProgramInfoLog(_shaderProgram, infoLogLen, &infoLogLen, infoLog);
		std::fprintf(stderr, "ShaderProgram linking errors:\n%s\n", infoLog); // throw an exception?
		delete[] infoLog;

		glDeleteProgram(_shaderProgram);
		glDeleteShader(_shaderVertex);
		glDeleteShader(_shaderFragment);

		return;
	}

	int uniformCount = -1;
	glGetProgramiv(_shaderProgram, GL_ACTIVE_UNIFORMS, &uniformCount);
	for (int i = 0; i < uniformCount; i++)
	{
		int name_len = -1, num = -1;
		GLenum type = GL_ZERO;
		char name[64];
		glGetActiveUniform(_shaderProgram, GLuint(i), sizeof(name) - 1, &name_len, &num, &type, name);
		name[name_len] = 0;

		_uniforms[std::string(name)] = glGetUniformLocation(_shaderProgram, name);
	}
}

ShaderProgram::~ShaderProgram()
{
	if (_shaderProgram != 0)
		glDeleteProgram(_shaderProgram);

	if (_shaderVertex != 0)
		glDeleteShader(_shaderVertex);

	if (_shaderFragment != 0)
		glDeleteShader(_shaderFragment);
}

GLuint ShaderProgram::createSubShader(GLenum type, const std::string& source)
{
	GLuint shader = glCreateShader(type);

	const GLchar* glsource = (const GLchar*)source.c_str();
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
		std::fprintf(stderr, "ShaderProgram compile errors:\n%s\n", infoLog); // throw an exception?
		delete[] infoLog;

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}
