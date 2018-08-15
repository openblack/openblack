#include "Texture.h"

#include <cassert>

using namespace OpenBlack::Graphics;

Texture::Texture() : m_textureID(0)
{
	glGenTextures(1, &m_textureID);
	assert(m_textureID);
}

Texture::~Texture()
{
	if (m_textureID != 0)
		glDeleteTextures(1, &m_textureID);
}

GLuint Texture::GetHandle() const
{
	return m_textureID;
}