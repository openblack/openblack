#include "Texture2DArray.h"

using namespace OpenBlack::Graphics;

void Texture2DArray::AddTexture(const TextureDef2D& textureData)
{
	m_textures.push_back(textureData);
}

void Texture2DArray::Create()
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, _textureID);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 256, 256, m_textures.size());
	//glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB5_A1, 256, 256, m_textures.size(), 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, NULL);

	printf("building 2d array with %d textures\n", m_textures.size());
	for (unsigned int i = 0; i < m_textures.size(); ++i)
	{
		TextureDef2D tex = m_textures[i];
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, tex.width, tex.height, 1, tex.format, tex.type, tex.data);
	}

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Texture2DArray::Bind()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, _textureID);
}

void Texture2DArray::Unbind()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, GL_NONE);
}

/*

void TextureArray::SetData()
{
	GLExt::glTexImage3D(m_data.m_target,
		m_data.m_level,
		m_data.m_internalFormat,
		m_data.m_width,
		m_data.m_height,
		m_textures.size(),
		m_data.m_border,
		m_data.m_format,
		m_data.m_type,
		NULL);

	Texture2D::Data::Buffer b((m_data.m_width * m_data.m_height) * m_data.m_bytesPerPixel, 0);

	for (unsigned int i = 0; i < m_textures.size(); ++i)
	{
		Texture2D* texture = m_textures[i];
		const Texture2D::Data& data = texture->GetData();
		const unsigned char* bufferPointer = NULL;

		if (data.m_bytesPerPixel != m_data.m_bytesPerPixel)
		{
			continue;
		}

		if (data.m_width != m_data.m_width ||
			data.m_height != m_data.m_height)
		{
			Util::ScaleImageNearestNeighbor(&data.m_buffer[0], &b[0], data.m_width, data.m_height, m_data.m_width, m_data.m_height, m_data.m_bytesPerPixel);

			bufferPointer = &b[0];
		}
		else
		{
			bufferPointer = &data.m_buffer[0];
		}

		GLExt::glTexSubImage3D(m_data.m_target,
			m_data.m_level,
			0,
			0,
			i,
			m_data.m_width,
			m_data.m_height,
			1,
			data.m_format,
			data.m_type,
			bufferPointer);
	}
}
*/
