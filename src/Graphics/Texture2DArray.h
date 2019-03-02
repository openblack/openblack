#pragma once
#ifndef OPENBLACK_TEXTURE2DARRAY_H
#define OPENBLACK_TEXTURE2DARRAY_H

#include "Texture2D.h"
#include <vector>

namespace OpenBlack {
namespace Graphics {

class Texture2DArray : public Texture {
public:
	void AddTexture(const TextureDef2D& textureData);
	void Create();

	void Bind();
	void Unbind();
protected:
	std::vector<TextureDef2D> _textureDefinitions;
};

}
}

#endif
