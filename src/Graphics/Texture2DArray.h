#pragma once
#ifndef OPENBLACK_GRAPHICS_TEXTURE2DARRAY_H
#define OPENBLACK_GRAPHICS_TEXTURE2DARRAY_H

#include <Graphics/Texture2D.h>
#include <vector>

namespace OpenBlack {
namespace Graphics {

class Texture2DArray : public Texture {
public:
	Texture2DArray() = delete;
	Texture2DArray(GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat);

	void SetTexture(GLsizei layer, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* textureData);

	inline void Bind() { glBindTexture(GL_TEXTURE_2D_ARRAY, _textureID); }

	const GLsizei GetWidth() const { return _width; }
	const GLsizei GetHeight() const { return _height; }
private:
	GLsizei _width;
	GLsizei _height;
	GLsizei _depth;
	GLenum _internalFormat;
};

}
}

#endif
