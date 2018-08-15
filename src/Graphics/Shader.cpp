#include "Shader.h"

using namespace OpenBlack::Graphics;

Shader::Shader() :
	m_program(0),
	m_vert(0),
	m_frag(0)
{

}

Shader::~Shader()
{
	if (m_program != 0)
		glDeleteProgram(m_program);

	if (m_vert != 0)
		glDeleteShader(m_vert);

	if (m_frag != 0)
		glDeleteShader(m_frag);

	for (size_t i = 0; i < m_parameters.size(); ++i)
		free(m_parameters[i].name);
}

bool Shader::Create(const void* vert, const void* frag)
{
	m_vert = CreateSubShader(GL_VERTEX_SHADER, vert);

	if (m_vert == 0)
		return false;

	m_frag = CreateSubShader(GL_FRAGMENT_SHADER, frag);

	if (m_frag == 0)
		return false;

	m_program = glCreateProgram();
	glAttachShader(m_program, m_vert);
	glAttachShader(m_program, m_frag);
	glLinkProgram(m_program);

	GLint linkStatus;
	glGetProgramiv(m_program, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == GL_FALSE)
	{
		glDeleteProgram(m_program);
		m_program = 0;

		glDeleteShader(m_vert);
		m_vert = 0;

		glDeleteShader(m_frag);
		m_frag = 0;

		return false;
	}

	GLint activeUniforms;
	glGetProgramiv(m_program, GL_ACTIVE_UNIFORMS, &activeUniforms);

	for (int i = 0; i < activeUniforms; ++i)
	{
		GLint size;
		GLenum type;

		GLchar name[256];
		int nameLength = 0;

		glGetActiveUniform(m_program, i, sizeof(name), &nameLength, &size, &type, name);

		Uniform uniform;
		uniform.name = (char*)malloc(nameLength + 1);
		memcpy(uniform.name, name, nameLength + 1);

		uniform.location = glGetUniformLocation(m_program, name);
		//uniform.globalID = globals.AddProperty(name);
		m_parameters.push_back(uniform);
	}

	return true;
}

GLuint Shader::GetHandle() const {
	return m_program;
}

const Shader::Uniform* Shader::Uniforms() const
{
	return m_parameters.data();
}

size_t Shader::NumUniforms() const
{
	return m_parameters.size();
}

GLenum Shader::CreateSubShader(GLenum type, const void* source)
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