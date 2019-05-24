/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include "L3DModel.h"

#include <stdint.h>
#include <stdexcept>

#include <Game.h>

#include <Common/stb_image_write.h>
#include <3D/MeshPack.h>

using namespace OpenBlack;
using namespace OpenBlack::Graphics;

struct L3DHeader {
	uint32_t flags;
	uint32_t skinOffset;
	uint32_t numMeshes;
	uint32_t meshListOffset; // L3D_Mesh
	uint32_t unknown_1;
	uint32_t unknown_2;
	uint32_t unknown_3;
	uint32_t unknown_4;
	uint32_t unknown_5;
	uint32_t unknown_6;
	uint32_t unknown_7;
	uint32_t unknown_8;
	uint32_t unknown_9;
	uint32_t numSkins;
	uint32_t skinListOffset;
	uint32_t unknown_10;
	uint32_t unknown_11;
	uint32_t pSkinName;
};

struct L3D_Mesh {
	uint8_t unknown_1;
	uint8_t unknown_2; // 20: nodraw
	uint8_t unknown_3;
	uint8_t unknown_4; // e0: transparent

	uint32_t numSubMeshes;
	uint32_t subMeshOffset; // L3D_SubMesh
	uint32_t numBones;
	uint32_t bonesOffset;
};

struct L3D_SubMesh {
	uint32_t unknown_1;
	uint32_t unknown_2;
	int32_t skinID;
	uint32_t unknown_3;

	uint32_t numVerticies;
	uint32_t verticiesOffset;
	uint32_t numTriangles;
	uint32_t trianglesOffset;
	uint32_t boneVertLUTSize;
	uint32_t boneVertLUTOffset;
	uint32_t numVertexBlends;
	uint32_t vertexBlendsOffset;
};

struct L3D_Vertex {
	float position[3];
	float texCoords[2];
	float normal[3];
};

struct L3D_Triangle {
	uint16_t indices[3];
};

struct L3D_Skin {
	uint32_t skinID;
	uint16_t data[256 * 256]; // RGBA4444
};

struct L3D_Bone {
	int32_t parentBone; // -1 = root
	int32_t childBone; // -1 = no children
	int32_t siblingBone; // -1 = no siblings

	// rotation matrix
	float rotXAxis[3];
	float rotYAxis[3];
	float rotZAxis[3];

	// bone origin position
	float position[3];
};

L3DModel::L3DModel() {}

L3DModel::~L3DModel()
{
	// free textures
}

void L3DModel::LoadFromFile(const std::string &fileName)
{
	size_t meshSize;
	char* mesh = OSFile::ReadAll((fileName).c_str(), &meshSize);
	LoadFromL3D(mesh, meshSize, false);
	delete[] mesh;
}

void L3DModel::LoadFromL3D(void* data_, size_t size, bool pack) {
	uint8_t* buffer = static_cast<uint8_t*>(data_);
	if (buffer[0] != 'L' || buffer[1] != '3' || buffer[2] != 'D' || buffer[3] != '0') {
		throw std::runtime_error("Invalid L3D file");
	}

	// if our mesh is from AllMeshes.g3d make sure we keep track of it.
	m_bPackedMesh = pack;

	L3DHeader* header = (L3DHeader*)(buffer + 4);
	uint32_t* meshOffsets = (uint32_t*)(buffer + header->meshListOffset);

	for (int m = 0; m < header->numMeshes; m++)
	{
		L3D_Mesh* mesh = (L3D_Mesh*)(buffer + meshOffsets[m]);

		_submeshes = new Mesh*[mesh->numSubMeshes];
		_submeshSkinIds = new GLuint[mesh->numSubMeshes];
		_submeshCount = mesh->numSubMeshes;

		uint32_t* submeshOffsets = (uint32_t*)(buffer + mesh->subMeshOffset);
		for (int sm = 0; sm < mesh->numSubMeshes; sm++)
		{
			L3D_SubMesh* subMesh = (L3D_SubMesh*)(buffer + submeshOffsets[sm]);

			void* verticiesOffset = (void*)(buffer + subMesh->verticiesOffset);
			void* trianglesOffset = (void*)(buffer + subMesh->trianglesOffset);

			//Mesh* sub = new Mesh();

			VertexDecl decl(3);
			decl[0] = VertexAttrib(0, 3, GL_FLOAT, 32, (void*)0);
			decl[1] = VertexAttrib(1, 2, GL_FLOAT, 32, (void*)12);
			decl[2] = VertexAttrib(2, 3, GL_FLOAT, 32, (void*)20);

			VertexBuffer *vertexBuffer = new VertexBuffer(verticiesOffset, subMesh->numVerticies, sizeof(L3D_Vertex));
			IndexBuffer *indexBuffer = new IndexBuffer(trianglesOffset, subMesh->numTriangles * 3, GL_UNSIGNED_SHORT);

			Mesh* sub = new Mesh(vertexBuffer, indexBuffer, decl);
			_submeshes[sm] = sub; // sub;
			_submeshSkinIds[sm] = subMesh->skinID;
		}

		L3D_Bone* bones = (L3D_Bone*)(buffer + mesh->bonesOffset);
		for (int b = 0; b < mesh->numBones; b++)
		{
			L3D_Bone bone = bones[b];
		}

		// stop idk how we should handle more then 1 mesh yet!
		break;
	}

	// Inside packed meshes, there are no skins.
	uint32_t* skinOffsets = (uint32_t*)(buffer + header->skinListOffset);

	m_glSkins = new GLuint[header->numSkins];
	glGenTextures(header->numSkins, m_glSkins);

	for (int s = 0; s < header->numSkins; s++)
	{
		L3D_Skin* skin = (L3D_Skin*)(buffer + skinOffsets[s]);
		printf("%d skin = %d\n", s, skin->skinID);

		glBindTexture(GL_TEXTURE_2D, m_glSkins[s]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// 16bit BGRA to RGBA
		// todo: can we upload the raw data using glTexImage2D?
		uint8_t* data = new uint8_t[256 * 256 * 4];
		for (int i = 0; i < 256 * 256; i++) {
			uint16_t p = skin->data[i];

			uint8_t b = (p & 0x0F) * 17; // 0000 0000 0000 1111
			uint8_t g = ((p >> 4) &  0x0F) * 17; // 0000 0000 1111 0000
			uint8_t r = ((p >> 8) &  0x0F) * 17; // 0000 1111 0000 0000
			uint8_t a = ((p >> 12) & 0x0F) * 17; // 1111 0000 0000 0000

			data[i * 4 + 0] = r;
			data[i * 4 + 1] = g;
			data[i * 4 + 2] = b;
			data[i * 4 + 3] = a;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, data);

		delete[] data;
	}
}

void L3DModel::Draw() {
	for (int subMesh = 0; subMesh < _submeshCount; subMesh++) {
		// no texture no render (todo: handle actual nodraw flags)
		if (_submeshSkinIds[subMesh] == -1) {
			_submeshes[subMesh]->Draw();
			continue;
		}

		// todo: handle non meshpack textures
		if (m_bPackedMesh) {
			MeshPack meshPack = Game::instance()->GetMeshPack();
			glBindTexture(GL_TEXTURE_2D, meshPack.Textures[_submeshSkinIds[subMesh] - 1]);
			_submeshes[subMesh]->Draw();
			glBindTexture(GL_TEXTURE_2D, GL_NONE);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_glSkins[0]);
			_submeshes[subMesh]->Draw();
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
}

unsigned int L3DModel::GetSubMeshCount()
{
	return _submeshCount;
}
