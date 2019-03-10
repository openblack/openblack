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

#include <map>
#include <string>

namespace OpenBlack {
namespace Graphics {

class ShaderProgram {
public:
	ShaderProgram() = delete;
	ShaderProgram(const std::string &vertexSource, const std::string &fragmentSource);
	~ShaderProgram();

	GLuint GetHandle() const { return _shaderProgram; }
private:
	GLuint _shaderProgram;
	GLuint _shaderVertex;
	GLuint _shaderFragment;

	std::map<std::string, GLint> _uniforms;

	GLenum createSubShader(GLenum type, const std::string& source);
};

}
}

#endif
