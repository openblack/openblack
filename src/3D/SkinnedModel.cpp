/* OpenBlack - A reimplementation of Lionhead's Black & White.
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

#include <3D/SkinnedModel.h>
#include <Common/OSFile.h>

#include <stdint.h>
#include <stdexcept>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>


using namespace OpenBlack;
using namespace OpenBlack::Graphics;

struct L3DHeader {
	uint32_t flags;

	/*
	flags & 0x1 = ? (bit 31)
	flags & 0x2 = ? (bit 30)
	flags & 0x4 = ? (bit 29)
	flags & 0x8 = ? (bit 28)
	flags & 0x10 = ? (bit 27)
	flags & 0x20 = ? (bit 26)
	flags & 0x40 = ? (bit 25)
	flags & 0x80 = ? (bit 24)
	flags & 0x100 = LH3DMesh::IsBoned (bit 23)
	flags & 0x200 = ? (bit 22)
	flags & 0x400 = ? (bit 21)
	flags & 0x800 = ? (bit 20)
	flags & 0x1000 = ? (bit 19)
	flags & 0x2000 = nodraw? (bit 18)
	flags & 0x4000 = ? (bit 17)
	flags & 0x8000 = LH3DMesh::IsContainsLandscapeFeature (bit 16)
	flags & 0x10000 = ? (bit 15)
	flags & 0x20000 = ? (bit 14)
	flags & 0x40000 = LH3DMesh::IsContainsUV2 (bit 13)
	flags & 0x80000 = LH3DMesh::IsContainsNameData (bit 12)
	flags & 0x100000 = LH3DMesh::IsContainsExtraMetrics (bit 11)
	flags & 0x200000 = LH3DMesh::IsContainsEBone (bit 10)
	flags & 0x400000 = LH3DMesh::IsContainsTnLData (bit 9)
	flags & 0x800000 = LH3DMesh::IsContainsNewEP (bit 8)
	flags & 0x1000000 = ? (bit 7)
	flags & 0x2000000 = ? (bit 6)
	flags & 0x4000000 = ? (bit 5)
	flags & 0x8000000 = ? (bit 4)
	flags & 0x10000000 = ? (bit 3)
	flags & 0x20000000 = ? (bit 2)
	flags & 0x40000000 = ? (bit 1)
	flags & 0x80000000 = ? (bit 0)
	*/

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
	uint32_t flags; // nodraw, transparent

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

struct LH3D_BoneVert
{
	uint16_t nVertices;
	uint16_t boneIndex;
};

struct L3D_Skin {
	uint32_t skinID;
	uint16_t data[256 * 256]; // RGBA4444
};

struct L3D_Bone {
	int32_t parentBone; // -1 = root;
	int32_t childBone; // -1 = no children
	int32_t siblingBone; // -1 = no siblings

	glm::vec3 rotXAxis;
	glm::vec3 rotYAxis;
	glm::vec3 rotZAxis;
	glm::vec3 position;
};

struct SkinnedModel_Vertex {
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 norm;
	uint32_t bone;
};

void SkinnedModel::LoadFromFile(const std::string &fileName)
{
	size_t meshSize;
	char* mesh = OSFile::ReadAll((fileName).c_str(), &meshSize);
	LoadFromL3D(mesh, meshSize);
	delete[] mesh;
}

void SkinnedModel::LoadFromL3D(void* data_, size_t size) {
	uint8_t* buffer = static_cast<uint8_t*>(data_);
	if (buffer[0] != 'L' || buffer[1] != '3' || buffer[2] != 'D' || buffer[3] != '0') {
		throw std::runtime_error("Invalid L3D file");
	}

	L3DHeader* header = (L3DHeader*)(buffer + 4);
	uint32_t* meshOffsets = (uint32_t*)(buffer + header->meshListOffset);

	printf("loading mesh with %d meshes (only 1st will be loaded)\n", header->numMeshes);

	for (int m = 0; m < header->numMeshes; m++)
	{
		L3D_Mesh* mesh = (L3D_Mesh*)(buffer + meshOffsets[m]);

		_submeshes.reserve(mesh->numSubMeshes);

		uint32_t* submeshOffsets = (uint32_t*)(buffer + mesh->subMeshOffset);
		for (int sm = 0; sm < mesh->numSubMeshes; sm++)
		{
			L3D_SubMesh* subMesh = (L3D_SubMesh*)(buffer + submeshOffsets[sm]);

			L3D_Vertex* verticiesOffset = static_cast<L3D_Vertex*>((void*)(buffer + subMesh->verticiesOffset));
			L3D_Triangle* trianglesOffset = static_cast<L3D_Triangle*>((void*)(buffer + subMesh->trianglesOffset));
			LH3D_BoneVert* boneVertOffset = static_cast<LH3D_BoneVert*>((void*)(buffer + subMesh->boneVertLUTOffset));

			//Mesh* sub = new Mesh();

			VertexDecl decl(4);
			decl[0] = VertexAttrib(0, 3, GL_FLOAT, false, false, sizeof(SkinnedModel_Vertex), (GLvoid*)offsetof(SkinnedModel_Vertex, pos));
			decl[1] = VertexAttrib(1, 2, GL_FLOAT, false, false, sizeof(SkinnedModel_Vertex), (GLvoid*)offsetof(SkinnedModel_Vertex, uv));
			decl[2] = VertexAttrib(2, 3, GL_FLOAT, false, false, sizeof(SkinnedModel_Vertex), (GLvoid*)offsetof(SkinnedModel_Vertex, norm));
			decl[3] = VertexAttrib(3, 1, GL_INT,   true, false,  sizeof(SkinnedModel_Vertex), (GLvoid*)offsetof(SkinnedModel_Vertex, bone));

			// create our vertex buffer real quick
			std::vector<SkinnedModel_Vertex> verts(subMesh->numVerticies);
			for (int i = 0; i < subMesh->numVerticies; i++)
			{
				// epic gamer
				memcpy(&verts[i], &verticiesOffset[i], sizeof(L3D_Vertex));
			}

			int vertind = 0;
			for (int boneVert = 0; boneVert < subMesh->boneVertLUTSize; boneVert++)
				for (int vert = 0; vert < boneVertOffset[boneVert].nVertices; vert++)
					verts[vertind++].bone = boneVertOffset[boneVert].boneIndex;

			VertexBuffer *vertexBuffer = new VertexBuffer(verts.data(), subMesh->numVerticies, sizeof(SkinnedModel_Vertex));
			IndexBuffer *indexBuffer = new IndexBuffer(trianglesOffset, subMesh->numTriangles * 3, GL_UNSIGNED_SHORT);

			_submeshes.emplace_back(std::make_unique<Mesh>(vertexBuffer, indexBuffer, decl));
			_submeshSkinMap[sm] = subMesh->skinID;
		}

		/* copy bones into our own format */
		L3D_Bone* bones = (L3D_Bone*)(buffer + mesh->bonesOffset);
		_bones.resize(mesh->numBones);

		for (size_t i = 0; i < mesh->numBones; i++)
		{
			const L3D_Bone &bone = bones[i];
			SkinnedModel_Bone &dstBone = _bones[i];

			dstBone.parentBone = bone.parentBone;
			dstBone.childBone = bone.childBone;
			dstBone.siblingBone = bone.siblingBone;

			dstBone.position = bone.position;
			glm::mat3 boneRotationMatrix = glm::mat3({
				bone.rotXAxis[0], bone.rotYAxis[0], bone.rotZAxis[0],
				bone.rotXAxis[1], bone.rotYAxis[1], bone.rotZAxis[1],
				bone.rotXAxis[2], bone.rotYAxis[2], bone.rotZAxis[2],
			});

			dstBone.rotation = glm::quat_cast(boneRotationMatrix);
		}

		calculateBoneMatrices();
		
		// stop idk how we should handle more then 1 mesh yet!
		break;
	}

	// Inside packed meshes, there are no skins.
	uint32_t* skinOffsets = (uint32_t*)(buffer + header->skinListOffset);

	for (int s = 0; s < header->numSkins; s++) {
		L3D_Skin* skin = static_cast<L3D_Skin*>((void*)(buffer + skinOffsets[s]));
		_textures[skin->skinID] = std::make_unique<Texture2D>(256, 256, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, skin->data);
	}
}

void SkinnedModel::Draw(ShaderProgram* program) {
	program->SetUniformValue("u_boneMatrices[0]", _boneMatrices.size(), _boneMatrices.data());

	for (int i = 0; i < _submeshes.size(); i++)
	{
		if (_textures[_submeshSkinMap[i]] != nullptr)
			_textures[_submeshSkinMap[i]]->Bind(0);

		_submeshes[i]->Draw();
	}
}

void SkinnedModel::calculateBoneMatrices()
{
	_boneMatrices.resize(64);
	for (size_t i = 0; i < _bones.size(); i++)
	{
		SkinnedModel_Bone &bone = _bones[i];

		// only bones with parents need to be transformed by their parents
		if (bone.parentBone != -1)
		{
			const SkinnedModel_Bone parentBone = _bones[bone.parentBone];

			bone.position = parentBone.position + parentBone.rotation * bone.position;
			bone.rotation = parentBone.rotation * bone.rotation;
		}

		glm::mat4 mat = glm::mat4(1.0f);
		mat = glm::translate(mat, bone.position);
		mat = mat * glm::mat4_cast(bone.rotation);

		_boneMatrices[i] = mat;
	}
}
