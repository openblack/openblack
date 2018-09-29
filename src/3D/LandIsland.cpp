#include "LandIsland.h"

#include <Common/OSFile.h>
#include <3D/oldLH3DIsland.h>

#include <stdexcept>

using namespace OpenBlack;

LandIsland::LandIsland()
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

    uint32_t blockCount, materialCount, countryCount, lowResTextureCount, blockSize, matSize, countrySize;
    uint8_t blockIndex[1024];

    file->Read(&blockCount, 4);
    file->Read(&blockIndex, 1024);
    file->Read(&materialCount, 4);
    file->Read(&countryCount, 4);
    file->Read(&blockSize, 4);
    file->Read(&matSize, 4);
    file->Read(&countrySize, 4);
    file->Read(&lowResTextureCount, 4);

	m_LowResTextureArray = std::make_unique<Texture2DArray>();
    for (uint32_t i = 0; i < lowResTextureCount; i++)
    {
        uint32_t textureSize;
        file->Seek(16, LH_SEEK_MODE::Current);
        file->Read(&textureSize, 4);
        file->Seek(textureSize - 4, LH_SEEK_MODE::Current);

		// GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    }

	blockCount--; // take away a block from the count, because it's not in the file?
	m_LandBlocks = std::make_unique<LandBlock*[]>(blockCount);
	for (uint32_t i = 0; i < blockCount; i++)
	{
		auto block = new LandBlock();

		m_LandBlocks[i] = block;
		file->Seek(blockSize, LH_SEEK_MODE::Current);
	}

	file->Seek(countryCount * countrySize, LH_SEEK_MODE::Current);

	m_MaterialArray = std::make_unique<Texture2DArray>();
    for (uint32_t i = 0; i < materialCount; i++)
    {
        uint16_t terrainType;
        uint16_t* textureData = new uint16_t[256 * 256];

        file->Read(&terrainType, 2);
        file->Read((void*)textureData, 256*256*sizeof(uint16_t));

        AddTexture(textureData);

        delete textureData;
    }
    m_MaterialArray->Create();
    
    file->Seek(65536, LH_SEEK_MODE::Current); // Noisemap
    file->Seek(65536, LH_SEEK_MODE::Current); // Bumpmap

	// Read 2709680/2711300 (1620 bytes left..)
    printf("Read %d/%d\n", file->Position(), fileSize);

    file->Close();
}

void LandIsland::Draw()
{

}

void LandIsland::AddTexture(uint16_t* data)
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
