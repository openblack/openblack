#include "MeshPack.h"

#include <stdint.h>
#include <stdexcept>

using namespace OpenBlack;

// Just enough for us to parse
struct L3DSMiniHeader {
	char magic[4];
	uint32_t unknown;
	uint32_t l3dSize;
};

struct G3DHiResTexture {
	uint32_t size;
	uint32_t id;
	uint32_t type;

	uint32_t dxSize;
	// surface desc shit.
};

struct DDSurfaceDesc {
	uint32_t size;
	uint32_t flags;
	uint32_t height;
	uint32_t width;
};

MeshPack::MeshPack(OSFile* file) : m_meshCount(0)
{
	// LiOnHeAd and a block header, but we already know it's high res textures
	file->Seek(8, LH_SEEK_MODE::Set);

	int totalTextures = 110;

	Textures = new GLuint[totalTextures];
	glGenTextures(totalTextures, Textures);

	for (int i = 0; i < totalTextures; i++)
	{
		// skip block header
		file->Seek(36, LH_SEEK_MODE::Current);

		G3DHiResTexture* hiresTexture = new G3DHiResTexture;
		file->Read(hiresTexture, sizeof(G3DHiResTexture));

		void* surfaceDesc = malloc(hiresTexture->dxSize - 4);
		file->Read(surfaceDesc, hiresTexture->dxSize - 4);

		DDSurfaceDesc* desc = (DDSurfaceDesc*)surfaceDesc;

		glBindTexture(GL_TEXTURE_2D, Textures[hiresTexture->id - 1]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (hiresTexture->type == 1)
		{
			glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
				desc->width, desc->height, 0, ((desc->width + 3) / 4)*((desc->height + 3) / 4) * 8, (uint8_t*)surfaceDesc + desc->size);
		}
		else if (hiresTexture->type == 2)
		{
			glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
				desc->width, desc->height, 0, ((desc->width + 3) / 4)*((desc->height + 3) / 4) * 16, (uint8_t*)surfaceDesc + desc->size);
		}

		free(surfaceDesc);
	}

	// each high res texture is actually a lionhead block...
	// char textureid[32];
	// uint32_t size;
	// AWKWARD...............

	LHSegment meshesSegment;
	file->GetSegment("MESHES", &meshesSegment, true);

	printf("Meshes Segment %s of %d bytes\n", meshesSegment.Name, meshesSegment.SegmentSize);

	uint8_t* data = (uint8_t*)meshesSegment.SegmentData;
	uint32_t* meshCount = (uint32_t*)(data + 4);
	uint32_t* meshOffsets = (uint32_t*)(data + 8);

	m_meshCount = (int)meshCount;

	printf("%d meshes\n", *meshCount);

	Models = new L3DModel*[*meshCount];

	for (int i = 0; i < *meshCount; i++)
	{
		L3DSMiniHeader* header = (L3DSMiniHeader*)(data + meshOffsets[i]);

		L3DModel* model = new L3DModel();
		model->LoadFromL3D(data + meshOffsets[i], header->l3dSize);

		Models[i] = model;
	}
}

int MeshPack::GetMeshCount() {
	return m_meshCount;
}
