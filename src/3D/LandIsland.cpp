#include "LandIsland.h"

#include <Common/OSFile.h>

#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <Common/stb_image_write.h>

using namespace OpenBlack;

LandIsland::LandIsland() : m_Mesh(0)
{

}

LandIsland::~LandIsland()
{
}

void LandIsland::LoadFromDisk(std::string fileName)
{
    if (!OSFile::Exists(fileName.c_str()))
        throw std::runtime_error("Land file does not exist.");

    OSFile* file = new OSFile();
    file->Open(fileName.c_str(), LH_FILE_MODE::Read);
    size_t fileSize = file->Size();

    uint32_t blockSize, matSize, countrySize;
    uint8_t blockIndex[1024];

    file->Read(&m_blockCount, 4);
    file->Read(&blockIndex, 1024);
    file->Read(&m_materialCount, 4);
    file->Read(&m_countryCount, 4);
    file->Read(&blockSize, 4);
    file->Read(&matSize, 4);
    file->Read(&countrySize, 4);
    file->Read(&m_lowresCount, 4);

	m_LowResTextureArray = std::make_unique<Texture2DArray>();
    for (uint32_t i = 0; i < m_lowresCount; i++)
    {
        uint32_t textureSize;
        file->Seek(16, LH_SEEK_MODE::Current);
        file->Read(&textureSize, 4);
        file->Seek(textureSize - 4, LH_SEEK_MODE::Current);

		// GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    }

	m_blockCount--; // take away a block from the count, because it's not in the file?
	m_LandBlocks = std::make_unique<LandBlock[]>(m_blockCount);
	for (uint32_t i = 0; i < m_blockCount; i++)
	{
		void* blockData = malloc(blockSize);
		file->Read(blockData, blockSize);

		auto block = &m_LandBlocks[i];

		block->LoadFromFile(blockData, blockSize);

		free(blockData);

	}

	file->Seek(m_countryCount * countrySize, LH_SEEK_MODE::Current);

	m_MaterialArray = std::make_unique<Texture2DArray>();
    for (uint32_t i = 0; i < m_materialCount; i++)
    {
        uint16_t terrainType;
        uint16_t* textureData = new uint16_t[256 * 256];

        file->Read(&terrainType, 2);
        file->Read((void*)textureData, 256*256*sizeof(uint16_t));

        addTexture(textureData);

        delete textureData;
    }
    m_MaterialArray->Create();
    
    file->Seek(65536, LH_SEEK_MODE::Current); // Noisemap
    file->Seek(65536, LH_SEEK_MODE::Current); // Bumpmap

	// Read 2709680/2711300 (1620 bytes left..)
    printf("Read %d/%d\n", file->Position(), fileSize);

	printf("m_blockCount: %d\n", m_blockCount);
	printf("m_materialCount: %d\n", m_materialCount);
	printf("m_countryCount: %d\n", m_countryCount);

    file->Close();

	buildMesh();
}

void LandIsland::Draw()
{
	m_Mesh->Render();
}

void LandIsland::buildMesh()
{
	if (m_Mesh != NULL) {
		printf("ERROR: LandIsland mesh already exists... (todo: rebuild)\n");
		return;
	}

	// we could use GL_QUADS?
	m_Mesh = new Mesh(GL_TRIANGLES, GL_STATIC_DRAW);

	VertexDecl decl(5);
	decl[0] = VertexAttrib(0, 3, GL_FLOAT, 25, (void*)0); // position
	decl[1] = VertexAttrib(1, 3, GL_UNSIGNED_BYTE, 25, (void*)12); // color
	decl[2] = VertexAttrib(2, 2, GL_FLOAT, 25, (void*)15); // uv
	decl[3] = VertexAttrib(3, 1, GL_UNSIGNED_BYTE, 25, (void*)23); // uv
	decl[4] = VertexAttrib(4, 1, GL_UNSIGNED_BYTE, 25, (void*)24); // uv

	m_Mesh->SetVertexDecl(decl);

	auto verts = getVerticies();
	auto indices = getIndices();

	printf("LandIsland made with %d verticies and %d indices\n", verts.size(), indices.size());

	m_Mesh->Create(verts.data(), verts.size() * sizeof(LandVertex), indices.data(), indices.size() * sizeof(uint16_t));
}

std::vector<LandVertex> LandIsland::getVerticies()
{
	std::vector<LandVertex> verts;

	auto maxVerts = m_blockCount * (17 * 17);
	verts.reserve(maxVerts);

	printf("making verts for %d blocks\n", m_blockCount);
	for (auto b = 0; b < m_blockCount; b++)
	{
		LandBlock* block = &m_LandBlocks[b];

		for (int cx = 0; cx < 17; cx++)
		{
			for (int cy = 0; cy < 17; cy++)
			{
				LandCell cell = block->GetCells()[cy * 17 + cx];

				LandVertex vert;
				vert.position[0] = block->GetMapPosition()->x + (cx * 10.0f);
				vert.position[1] = cell.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT;
				vert.position[2] = block->GetMapPosition()->y + (cy * 10.0f);
				vert.color[0] = 1.0f;
				vert.color[1] = 1.0f;
				vert.color[2] = 1.0f;
				vert.uv[0] = (float)cx / (float)17;
				vert.uv[1] = (float)cy / (float)17;
				vert.country = cell.Country();
				vert.altitude = cell.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT;

				//printf("Vert [%.1f, %.1f, %.1f] - alt %d\n", vert.position[0], vert.position[1], vert.position[2], vert.altitude);

				verts.push_back(vert);
			}
		}
	}
	
	return verts;
}

std::vector<uint16_t> LandIsland::getIndices()
{
	std::vector<uint16_t> indices;

	// 16*16 quads * 2 tris * 3 indices = 1536
	auto maxIndices = m_blockCount * 1536;
	indices.reserve(maxIndices);

	printf("making idx for %d blocks\n", m_blockCount);
	for (auto b = 0; b < m_blockCount; b++)
	{
		LandBlock block = m_LandBlocks[b];
		auto vertexOffset = b * 289; // each block has 289 verts.

		for (int cx = 0; cx < 16; cx++)
		{
			for (int cy = 0; cy < 16; cy++)
			{
				indices.push_back(vertexOffset + cy * 17 + cx); // top left
				indices.push_back(vertexOffset + cy * 17 + cx + 1); // top right
				indices.push_back(vertexOffset + (cy + 1) * 17 + cx); // bottom left

				indices.push_back(vertexOffset + cy * 17 + cx + 1); // top right
				indices.push_back(vertexOffset + (cy + 1) * 17 + cx + 1); // bottom right
				indices.push_back(vertexOffset + (cy + 1) * 17 + cx); // bottom left
			}
		}
	}

	return indices;
}

void LandIsland::addTexture(uint16_t* data)
{
    TextureDef2D tex;
    tex.width = 256;
    tex.height = 256;
    tex.format = GL_RGBA;
    tex.internalFormat = GL_RGBA;
    tex.type = GL_UNSIGNED_INT_8_8_8_8;
    tex.data = new uint8_t[256 * 256 * 4];

	// convert the data from RGB5 to RGB8
    for (int i = 0; i < 256 * 256; i++)
    {
        uint16_t col = data[i];

        uint8_t r = (col & 0x7C00) >> 10;
        uint8_t g = (col & 0x3E0) >> 5;
        uint8_t b = (col & 0x1F);

        ((uint8_t*)tex.data)[i * 4 + 3] = r << 3; // 5
        ((uint8_t*)tex.data)[i * 4 + 2] = g << 3; // 5
        ((uint8_t*)tex.data)[i * 4 + 1] = b << 3; // 5
        ((uint8_t*)tex.data)[i * 4 + 0] = 255;
    }

    m_MaterialArray->AddTexture(tex);
}

void LandIsland::DumpTextures()
{
	/*
		Dumps Textures from VRAM using a FBO (Works on OpenGL 3.2+)
		A better way would be glGetTexSubImage from OpenGL 4.5.
	*/

	GLuint textureID = m_MaterialArray->GetHandle();

	GLuint fboID = 0;
	glGenFramebuffers(1, &fboID);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID);

	for (int i = 0; i < m_materialCount; i++) {
		uint8_t* pixels = new uint8_t[256 * 256 * 4];

		glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureID, 0, i);
		glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		auto filename = "dump/landtex" + std::to_string(i) + ".png";

		printf("Writing texture %d to %s\n", i, filename.c_str());
		stbi_write_png(filename.c_str(), 256, 256, 4, pixels, 256 * 4);

		delete pixels;
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fboID);
}
