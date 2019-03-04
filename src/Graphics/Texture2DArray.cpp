#include <Graphics/Texture2DArray.h>

using OpenBlack::Graphics::Texture2DArray;

Texture2DArray::Texture2DArray(GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat)
	: _width(width), _height(height), _depth(depth), _internalFormat(internalFormat)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, _textureID);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, _internalFormat, _width, _height, _depth);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Texture2DArray::SetTexture(GLsizei layer, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * textureData)
{
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, width, height, 1, format, type, textureData);
}
