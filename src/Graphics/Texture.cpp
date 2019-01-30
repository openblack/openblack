#include "Texture.h"

#include <cassert>

using namespace OpenBlack::Graphics;

Texture::Texture() : _textureID(0)
{
	glGenTextures(1, &_textureID);
	assert(_textureID);
}

Texture::~Texture()
{
	if (_textureID != 0)
		glDeleteTextures(1, &_textureID);
}

GLuint Texture::GetHandle() const
{
	return _textureID;
}
