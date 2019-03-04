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

#include <Graphics/ShaderProgram.h>

using OpenBlack::Graphics::ShaderProgram;

ShaderProgram::ShaderProgram(const std::string &vertexSource, const std::string &fragmentSource)
{
	_shaderVertex = createSubShader(GL_VERTEX_SHADER, vertexSource);
	_shaderFragment = createSubShader(GL_FRAGMENT_SHADER, fragmentSource);

	_shaderProgram = glCreateProgram();
	glAttachShader(_shaderProgram, _shaderVertex);
	glAttachShader(_shaderProgram, _shaderFragment);
	glLinkProgram(_shaderProgram);

	GLint linkStatus;
	glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == GL_FALSE)
	{
		glDeleteProgram(_shaderProgram);
		glDeleteShader(_shaderVertex);
		glDeleteShader(_shaderFragment);

		throw std::runtime_error("Shader link failed");
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

GLenum ShaderProgram::createSubShader(GLenum type, const std::string& source)
{
	GLenum shader = glCreateShader(type);

	const GLchar* glsource = (const GLchar*)source.c_str();

	glShaderSource(shader, 1, &glsource, nullptr);
	glCompileShader(shader);

	GLint compileStatus = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus == GL_FALSE)
	{
		GLint infoLogLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

		// todo: I don't like malloc..
		char* log = (char*)malloc(infoLogLen);
		glGetShaderInfoLog(shader, infoLogLen, &infoLogLen, log);
		std::fprintf(stderr, "ShaderProgram compile errors: \n %s\n", log);
		free(log);

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}
