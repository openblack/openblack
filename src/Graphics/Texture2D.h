#pragma once
#ifndef OPENBLACK_TEXTURE2D_H
#define OPENBLACK_TEXTURE2D_H

#include "Texture.h"

namespace OpenBlack
{
	namespace Graphics
	{

		struct TextureDef2D
		{
			GLsizei width;
			GLsizei height;
			GLenum internalFormat;
			GLenum format;
			GLenum type;

			const void* data;
			//unsigned size;
		};

		class Texture2D : public Texture {
		public:
			bool Create(const TextureDef2D& textureData);

			void Bind();
			void Unbind();
		};

	}
}

#endif