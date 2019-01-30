#include "Texture2D.h"

#include <cassert>

using namespace OpenBlack::Graphics;

bool Texture2D::Create(const TextureDef2D& textureData)
{
	glBindTexture(GL_TEXTURE_2D, _textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, textureData.internalFormat, textureData.width, textureData.height, 0, textureData.format, textureData.type, textureData.data);

	// todo: expose this
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return true;
}

void Texture2D::Bind()
{
	glBindTexture(GL_TEXTURE_2D, _textureID);
}

void Texture2D::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
}
