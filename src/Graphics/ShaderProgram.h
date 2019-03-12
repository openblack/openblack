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

#pragma once
#ifndef OPENBLACK_GRAPHICS_SHADERPROGRAM_H
#define OPENBLACK_GRAPHICS_SHADERPROGRAM_H

#include <Graphics/OpenGL.h>
#include <glm/glm.hpp>

#include <map>
#include <string>

namespace OpenBlack {
namespace Graphics {

class ShaderProgram {
public:
	ShaderProgram() = delete;
	ShaderProgram(const std::string &vertexSource, const std::string &fragmentSource);
	~ShaderProgram();

	void Bind();

	void SetUniformValue(std::string& uniformName, float value);
	void SetUniformValue(std::string& uniformName, const glm::vec2 & v);
	void SetUniformValue(std::string& uniformName, const glm::vec3 & v);
	void SetUniformValue(std::string& uniformName, const glm::vec4 & v);
	void SetUniformValue(std::string& uniformName, const glm::mat3 & m);
	void SetUniformValue(std::string& uniformName, const glm::mat4 & m);

	GLuint GetRawHandle() const { return _program; }
private:
	GLuint _program;
	std::map<std::string, GLint> _uniforms;

	GLuint createSubShader(GLenum type, const std::string& source);
};

}
}

#endif
