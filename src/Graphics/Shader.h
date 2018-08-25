#pragma once
#ifndef OPENBLACK_SHADER_H
#define OPENBLACK_SHADER_H

#include "OpenGL.h"

#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace OpenBlack
{
	namespace Graphics
	{

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
			bool Create(const void* vert, const void* frag);

			GLuint GetHandle() const;

			const Uniform* Uniforms() const;
			size_t NumUniforms() const;
		private:
			GLuint m_program;
			GLuint m_vert;
			GLuint m_frag;

			std::vector<Uniform> m_parameters;

			GLenum CreateSubShader(GLenum type, const void* source);
		};
	}
}

#endif
