#include <Graphics/Texture2D.h>

using namespace OpenBlack::Graphics;

Texture2D::Texture2D(GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const void* textureData)
	: _width(width), _height(height), _internalFormat(internalFormat), _format(format), _type(type)
{
	glBindTexture(GL_TEXTURE_2D, _textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, _width, _height, 0, _format, _type, textureData);

	// todo: expose this
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
