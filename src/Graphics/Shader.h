#pragma once
#ifndef OPENBLACK_SHADER_H
#define OPENBLACK_SHADER_H

#include "OpenGL.h"

#include <vector>

namespace OpenBlack {
namespace Graphics {

class Shader {
public:
	struct Uniform
	{
		char* name;
		GLint location;
		int globalID;
	};

	Shader();
	virtual ~Shader();
	bool Create(const char* vert, const char* frag);

	GLuint GetHandle() const;

	const Uniform* Uniforms() const;
	size_t NumUniforms() const;
private:
	GLuint _program;
	GLuint _vert;
	GLuint _frag;

	std::vector<Uniform> _parameters;

	GLenum createSubShader(GLenum type, const void* source);
};

}
}

#endif
