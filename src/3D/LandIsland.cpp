#include "LandIsland.h"

#include <Common/OSFile.h>

#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <Common/stb_image_write.h>

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

    uint32_t blockSize, matSize, countrySize;
    uint8_t blockIndex[1024];

    file->Read(&_blockCount, 4);
    file->Read(&blockIndex, 1024);
    file->Read(&_materialCount, 4);
    file->Read(&_countryCount, 4);
    file->Read(&blockSize, 4);
    file->Read(&matSize, 4);
    file->Read(&countrySize, 4);
    file->Read(&_lowresCount, 4);

	_lowResTextureArray = std::make_shared<Texture2DArray>();
    for (uint32_t i = 0; i < _lowresCount; i++)
    {
        uint32_t textureSize;
        file->Seek(16, LH_SEEK_MODE::Current);
        file->Read(&textureSize, 4);
        file->Seek(textureSize - 4, LH_SEEK_MODE::Current);

		// GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    }

	_blockCount--; // take away a block from the count, because it's not in the file?
	_landBlocks = std::make_unique<LandBlock[]>(_blockCount);
	for (uint32_t i = 0; i < _blockCount; i++)
	{
		char* blockData = new char[blockSize];
		file->Read(blockData, blockSize);

		auto block = &_landBlocks[i];

		block->LoadFromFile(blockData, blockSize);

		delete[] blockData;
	}

	_countries = std::make_unique<Country[]>(_countryCount);
	file->Read(_countries.get(), _countryCount * sizeof(Country));

	//file->Seek(_countryCount * countrySize, LH_SEEK_MODE::Current);

	_materialArray = std::make_shared<Texture2DArray>();
    for (uint32_t i = 0; i < _materialCount; i++)
    {
        uint16_t terrainType;
        uint16_t* textureData = new uint16_t[256 * 256];

        file->Read(&terrainType, 2);
        file->Read(textureData, 256*256*sizeof(uint16_t));

        addTexture(textureData);

        delete[] textureData;
    }
    _materialArray->Create();
    
    file->Seek(65536, LH_SEEK_MODE::Current); // Noisemap
    file->Seek(65536, LH_SEEK_MODE::Current); // Bumpmap

	// Read 2709680/2711300 (1620 bytes left..)
    printf("Read %d/%d\n", file->Position(), fileSize);

	printf("_blockCount: %d\n", _blockCount);
	printf("_materialCount: %d\n", _materialCount);
	printf("_countryCount: %d\n", _countryCount);

    file->Close();

	buildMesh();
}

const uint8_t LandIsland::GetAltitudeAt(glm::ivec2 vec) const
{
	return uint8_t();
}

const float LandIsland::GetHeightAt(glm::ivec2 vec) const
{
	return GetAltitudeAt(vec) * OPENBLACK_LANDISLAND_HEIGHT_UNIT;
}

void LandIsland::Draw()
{
	_mesh->Draw();
}

void LandIsland::buildMesh()
{
	if (_mesh != NULL) {
		printf("ERROR: LandIsland mesh already exists... (todo: rebuild)\n");
		return;
	}

	VertexDecl decl(7);
	decl[0] = VertexAttrib(0, 3, GL_FLOAT,			false, sizeof(LandVertex), nullptr); // position
	decl[1] = VertexAttrib(1, 2, GL_FLOAT,			false, sizeof(LandVertex), (void*)offsetof(LandVertex, textureCoords));
	decl[2] = VertexAttrib(2, 3, GL_FLOAT,			false, sizeof(LandVertex), (void*)offsetof(LandVertex, weight));
	decl[3] = VertexAttrib(3, 3, GL_UNSIGNED_BYTE,	true,  sizeof(LandVertex), (void*)offsetof(LandVertex, firstMaterialID));
	decl[4] = VertexAttrib(4, 3, GL_UNSIGNED_BYTE,	true,  sizeof(LandVertex), (void*)offsetof(LandVertex, secondMaterialID));
	decl[5] = VertexAttrib(5, 3, GL_UNSIGNED_BYTE,	true,  sizeof(LandVertex), (void*)offsetof(LandVertex, materialBlendCoefficient));
	decl[6] = VertexAttrib(6, 1, GL_FLOAT,			false, sizeof(LandVertex), (void*)offsetof(LandVertex, alpha));
	
	auto verts = getVerticies();

	printf("LandIsland made with %d verticies\n", verts.size());

	std::shared_ptr<VertexBuffer> vertexBuffer = std::make_shared<VertexBuffer>(verts.data(), verts.size(), sizeof(LandVertex));
	_mesh = std::make_unique<Mesh>(vertexBuffer, decl, GL_TRIANGLES);
}

std::vector<LandVertex> LandIsland::getVerticies()
{
	std::vector<LandVertex> verts;

	// 16*16 quads * 2 tris * 3 verts = 1536
	auto maxVerts = _blockCount * 1536;
	verts.reserve(maxVerts);

	printf("making verts for %d blocks\n", _blockCount);
	for (auto b = 0; b < _blockCount; b++)
	{
		LandBlock* block = &_landBlocks[b];
		float mapx = block->GetMapPosition()->x;
		float mapz = block->GetMapPosition()->y;

		for (int x = 0; x < 16; x++)
		{
			for (int y = 0; y < 16; y++)
			{
				LandCell tl = block->GetCells()[(y + 0) * 17 + (x + 0)];
				LandCell tr = block->GetCells()[(y + 0) * 17 + (x + 1)];
				LandCell bl = block->GetCells()[(y + 1) * 17 + (x + 0)];
				LandCell br = block->GetCells()[(y + 1) * 17 + (x + 1)];

				glm::vec3 pTL(mapx + ((x + 0) * 10.0f), tl.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT, mapz + ((y + 0) * 10.0f));
				glm::vec3 pTR(mapx + ((x + 1) * 10.0f), tr.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT, mapz + ((y + 0) * 10.0f));
				glm::vec3 pBL(mapx + ((x + 0) * 10.0f), bl.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT, mapz + ((y + 1) * 10.0f));
				glm::vec3 pBR(mapx + ((x + 1) * 10.0f), br.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT, mapz + ((y + 1) * 10.0f));

				glm::vec2 uvTL(x / 16.0f, y / 16.0f);
				glm::vec2 uvTR((x+1) / 16.0f, y / 16.0f);
				glm::vec2 uvBL(x / 16.0f, (y+1) / 16.0f);
				glm::vec2 uvBR((x+1) / 16.0f, (y+1) / 16.0f);

				auto tlMat = _countries[tl.Country()].MapMaterial[tl.Altitude()];
				auto trMat = _countries[tr.Country()].MapMaterial[tr.Altitude()];
				auto blMat = _countries[bl.Country()].MapMaterial[bl.Altitude()];
				auto brMat = _countries[br.Country()].MapMaterial[br.Altitude()];

				// triangle one: TL -> TR -> BR
				verts.push_back(LandVertex(pTL, uvTL, glm::vec3(1.0f, 0.0f, 0.0f),
					tlMat.FirstMaterialIndex, trMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
					tlMat.SecondMaterialIndex, trMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
					tlMat.Coeficient, trMat.Coeficient, brMat.Coeficient,
					tl.Alpha()
				));
				verts.push_back(LandVertex(pTR, uvTR, glm::vec3(0.0f, 1.0f, 0.0f),
					tlMat.FirstMaterialIndex, trMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
					tlMat.SecondMaterialIndex, trMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
					tlMat.Coeficient, trMat.Coeficient, brMat.Coeficient,
					tr.Alpha()
				));
				verts.push_back(LandVertex(pBR, uvBR, glm::vec3(0.0f, 0.0f, 1.0f),
					tlMat.FirstMaterialIndex, trMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
					tlMat.SecondMaterialIndex, trMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
					tlMat.Coeficient, trMat.Coeficient, brMat.Coeficient,
					br.Alpha()
				));

				// triangle two: BR -> BL -> TL
				verts.push_back(LandVertex(pBR, uvBR, glm::vec3(0.0f, 0.0f, 1.0f),
					tlMat.FirstMaterialIndex, blMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
					tlMat.SecondMaterialIndex, blMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
					tlMat.Coeficient, blMat.Coeficient, brMat.Coeficient,
					br.Alpha()
				));
				verts.push_back(LandVertex(pBL, uvBL, glm::vec3(0.0f, 1.0f, 0.0f),
					tlMat.FirstMaterialIndex, blMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
					tlMat.SecondMaterialIndex, blMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
					tlMat.Coeficient, blMat.Coeficient, brMat.Coeficient,
					bl.Alpha()
				));
				verts.push_back(LandVertex(pTL, uvTL, glm::vec3(1.0f, 0.0f, 0.0f),
					tlMat.FirstMaterialIndex, blMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
					tlMat.SecondMaterialIndex, blMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
					tlMat.Coeficient, blMat.Coeficient, brMat.Coeficient,
					tl.Alpha()
				));
			}
		}
	}
	
	return verts;
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

    _materialArray->AddTexture(tex);
}

void LandIsland::DumpTextures()
{
	/*
		Dumps Textures from VRAM using a FBO (Works on OpenGL 3.2+)
		A better way would be glGetTexSubImage from OpenGL 4.5.
	*/

	GLuint textureID = _materialArray->GetHandle();

	GLuint fboID = 0;
	glGenFramebuffers(1, &fboID);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID);

	for (int i = 0; i < _materialCount; i++) {
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
