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

#include <Graphics/Shader.h>

using namespace OpenBlack::Graphics;

Shader::Shader() :
	_program(0),
	_vert(0),
	_frag(0)
{

}

Shader::~Shader()
{
	if (_program != 0)
		glDeleteProgram(_program);

	if (_vert != 0)
		glDeleteShader(_vert);

	if (_frag != 0)
		glDeleteShader(_frag);

	for (size_t i = 0; i < _parameters.size(); ++i)
		free(_parameters[i].name);
}

bool Shader::Create(const char* vert, const char* frag)
{
	_vert = createSubShader(GL_VERTEX_SHADER, vert);

	if (_vert == 0)
		return false;

	_frag = createSubShader(GL_FRAGMENT_SHADER, frag);

	if (_frag == 0)
		return false;

	_program = glCreateProgram();
	glAttachShader(_program, _vert);
	glAttachShader(_program, _frag);
	glLinkProgram(_program);

	GLint linkStatus;
	glGetProgramiv(_program, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == GL_FALSE)
	{
		glDeleteProgram(_program);
		_program = 0;

		glDeleteShader(_vert);
		_vert = 0;

		glDeleteShader(_frag);
		_frag = 0;

		return false;
	}

	GLint activeUniforms;
	glGetProgramiv(_program, GL_ACTIVE_UNIFORMS, &activeUniforms);

	for (int i = 0; i < activeUniforms; ++i)
	{
		GLint size;
		GLenum type;

		GLchar name[256];
		int nameLength = 0;

		glGetActiveUniform(_program, i, sizeof(name), &nameLength, &size, &type, name);

		Uniform uniform;
		uniform.name = (char*)malloc(nameLength + 1);
		memcpy(uniform.name, name, nameLength + 1);

		uniform.location = glGetUniformLocation(_program, name);
		//uniform.globalID = globals.AddProperty(name);
		_parameters.push_back(uniform);
	}

	return true;
}

GLuint Shader::GetHandle() const {
	return _program;
}

const Shader::Uniform* Shader::Uniforms() const
{
	return _parameters.data();
}

size_t Shader::NumUniforms() const
{
	return _parameters.size();
}

GLenum Shader::createSubShader(GLenum type, const void* source)
{
	GLenum shader = glCreateShader(type);

	const GLchar* glsource = (const GLchar*)source;

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
		std::fprintf(stderr, "Shader compile errors: \n %s\n", log);
		free(log);

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}
