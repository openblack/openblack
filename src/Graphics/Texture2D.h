#pragma once
#ifndef OPENBLACK_TEXTURE2D_H
#define OPENBLACK_TEXTURE2D_H

#include <Graphics/Texture.h>

namespace OpenBlack {
namespace Graphics {

class Texture2D : public Texture {
public:
	Texture2D() = delete;
	Texture2D(GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const void* textureData);

	inline void Bind() { glBindTexture(GL_TEXTURE_2D, _textureID); }

	const GLsizei GetWidth() const { return _width; }
	const GLsizei GetHeight() const { return _height; }
private:
	GLsizei _width;
	GLsizei _height;
	GLenum _internalFormat;
	GLenum _format;
	GLenum _type;
};

}
}

#endif
