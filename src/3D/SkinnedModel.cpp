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
#include <Game.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

using namespace OpenBlack;
using namespace OpenBlack::Graphics;

enum class L3DFlag : uint32_t
{
	Unknown1 = 1 << 0,
	Unknown2 = 1 << 1,
	Unknown3 = 1 << 2,
	Unknown4 = 1 << 3,
	Unknown5 = 1 << 4,
	Unknown6 = 1 << 5,
	Unknown7 = 1 << 6,
	Unknown8 = 1 << 7,
	HasBones = 1 << 8,
	Unknown10 = 1 << 9,
	Unknown11 = 1 << 10,
	Unknown12 = 1 << 11,
	Unknown13 = 1 << 12,
	NoDraw = 1 << 13,
	Unknown15 = 1 << 14,
	ContainsLandscapeFeature = 1 << 15,
	Unknown17 = 1 << 16,
	Unknown18 = 1 << 17,
	ContainsUV2 = 1 << 18,
	ContainsNameData = 1 << 19,
	ContainsExtraMetrics = 1 << 20,
	ContainsEBone = 1 << 21,
	ContainsTnLData = 1 << 22,
	ContainsNewEP = 1 << 23,
	Unknown25 = 1 << 24,
	Unknown26 = 1 << 25,
	Unknown27 = 1 << 26,
	Unknown28 = 1 << 27,
	Unknown29 = 1 << 28,
	Unknown30 = 1 << 29,
	Unknown31 = 1 << 30,
};

struct L3DHeader
{
	uint32_t flags; // L3DFlag
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

struct L3D_Mesh
{
	uint32_t flags; // nodraw, transparent

	uint32_t numSubMeshes;
	uint32_t subMeshOffset; // L3D_SubMesh
	uint32_t numBones;
	uint32_t bonesOffset;
};

struct L3D_SubMesh
{
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

struct L3D_Vertex
{
	float position[3];
	float texCoords[2];
	float normal[3];
};

struct L3D_Triangle
{
	uint16_t indices[3];
};

struct LH3D_BoneVert
{
	uint16_t nVertices;
	uint16_t boneIndex;
};

struct L3D_Skin
{
	uint32_t skinID;
	uint16_t data[256 * 256]; // RGBA4444
};

struct L3D_Bone
{
	int32_t parentBone;  // -1 = root;
	int32_t childBone;   // -1 = no children
	int32_t siblingBone; // -1 = no siblings

	glm::vec3 rotXAxis;
	glm::vec3 rotYAxis;
	glm::vec3 rotZAxis;
	glm::vec3 position;
};

struct SkinnedModel_Vertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 norm;
	uint32_t bone;
};

void SkinnedModel::LoadFromFile(const std::string& fileName)
{
	size_t meshSize;
	// char* mesh = OSFile::ReadAll((fileName).c_str(), &meshSize);
	// LoadFromL3D(mesh, meshSize);
	// delete[] mesh;
}

void SkinnedModel::LoadFromL3D(void* data_, size_t size)
{
	uint8_t* buffer = static_cast<uint8_t*>(data_);
	if (buffer[0] != 'L' || buffer[1] != '3' || buffer[2] != 'D' || buffer[3] != '0')
	{
		throw std::runtime_error("Invalid L3D file");
	}

	L3DHeader* header     = (L3DHeader*)(buffer + 4);
	uint32_t* meshOffsets = (uint32_t*)(buffer + header->meshListOffset);

	for (uint32_t m = 0; m < header->numMeshes; m++)
	{
		L3D_Mesh* mesh = (L3D_Mesh*)(buffer + meshOffsets[m]);

		_submeshes.reserve(mesh->numSubMeshes);

		uint32_t* submeshOffsets = (uint32_t*)(buffer + mesh->subMeshOffset);
		for (uint32_t sm = 0; sm < mesh->numSubMeshes; sm++)
		{
			L3D_SubMesh* subMesh = (L3D_SubMesh*)(buffer + submeshOffsets[sm]);

			L3D_Vertex* verticiesOffset   = static_cast<L3D_Vertex*>((void*)(buffer + subMesh->verticiesOffset));
			L3D_Triangle* trianglesOffset = static_cast<L3D_Triangle*>((void*)(buffer + subMesh->trianglesOffset));
			LH3D_BoneVert* boneVertOffset = static_cast<LH3D_BoneVert*>((void*)(buffer + subMesh->boneVertLUTOffset));

			VertexDecl decl(4);
			decl[0] = VertexAttrib(0, 3, GL_FLOAT, false, false, sizeof(SkinnedModel_Vertex), (GLvoid*)offsetof(SkinnedModel_Vertex, pos));
			decl[1] = VertexAttrib(1, 2, GL_FLOAT, false, false, sizeof(SkinnedModel_Vertex), (GLvoid*)offsetof(SkinnedModel_Vertex, uv));
			decl[2] = VertexAttrib(2, 3, GL_FLOAT, false, false, sizeof(SkinnedModel_Vertex), (GLvoid*)offsetof(SkinnedModel_Vertex, norm));
			decl[3] = VertexAttrib(3, 1, GL_INT, true, false, sizeof(SkinnedModel_Vertex), (GLvoid*)offsetof(SkinnedModel_Vertex, bone));

			// create our vertex buffer real quick
			std::vector<SkinnedModel_Vertex> verts(subMesh->numVerticies);
			for (uint32_t i = 0; i < subMesh->numVerticies; i++)
			{
				// epic gamer
				memcpy(&verts[i], &verticiesOffset[i], sizeof(L3D_Vertex));
			}

			int vertind = 0;
			for (uint32_t boneVert = 0; boneVert < subMesh->boneVertLUTSize; boneVert++)
				for (uint32_t vert = 0; vert < boneVertOffset[boneVert].nVertices; vert++)
					verts[vertind++].bone = boneVertOffset[boneVert].boneIndex;

			VertexBuffer* vertexBuffer = new VertexBuffer(verts.data(), subMesh->numVerticies, sizeof(SkinnedModel_Vertex));
			IndexBuffer* indexBuffer   = new IndexBuffer(trianglesOffset, subMesh->numTriangles * 3, GL_UNSIGNED_SHORT);

			_submeshes.emplace_back(std::make_unique<Mesh>(vertexBuffer, indexBuffer, decl));
			_submeshSkinMap[sm] = subMesh->skinID;
		}

		/* copy bones into our own format */
		L3D_Bone* bones = (L3D_Bone*)(buffer + mesh->bonesOffset);
		_bones.resize(mesh->numBones);

		for (size_t i = 0; i < mesh->numBones; i++)
		{
			const L3D_Bone& bone       = bones[i];
			SkinnedModel_Bone& dstBone = _bones[i];

			dstBone.parentBone  = bone.parentBone;
			dstBone.childBone   = bone.childBone;
			dstBone.siblingBone = bone.siblingBone;

			dstBone.position             = bone.position;
			glm::mat3 boneRotationMatrix = glm::mat3({
			    bone.rotXAxis[0],
			    bone.rotYAxis[0],
			    bone.rotZAxis[0],
			    bone.rotXAxis[1],
			    bone.rotYAxis[1],
			    bone.rotZAxis[1],
			    bone.rotXAxis[2],
			    bone.rotYAxis[2],
			    bone.rotZAxis[2],
			});

			dstBone.rotation = glm::quat_cast(boneRotationMatrix);
		}

		calculateBoneMatrices();

		// stop idk how we should handle more then 1 mesh yet!
		break;
	}

	// Inside packed meshes, there are no skins.
	uint32_t* skinOffsets = (uint32_t*)(buffer + header->skinListOffset);

	for (uint32_t s = 0; s < header->numSkins; s++)
	{
		L3D_Skin* skin          = static_cast<L3D_Skin*>((void*)(buffer + skinOffsets[s]));
		_textures[skin->skinID] = std::make_unique<Texture2D>(256, 256, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, skin->data);
	}
}

void SkinnedModel::Draw(ShaderProgram* program)
{
	program->SetUniformValue("u_boneMatrices[0]", _boneMatrices.size(), _boneMatrices.data());
	auto& textureBinds = Game::instance()->GetMeshPack().GetTextures();

	for (size_t i = 0; i < _submeshes.size(); i++)
	{
		if (_textures[_submeshSkinMap[i]] != nullptr)
			_textures[_submeshSkinMap[i]]->Bind(0);
			_submeshes[i]->Draw();
		}
		else if (i < textureBinds.size())
		{
			auto index       = _submeshSkinMap[i];

			// skip
			if (index == -1)
				continue;

			const auto& textureBind = textureBinds[index - 1];
			textureBind->Bind(0);
			_submeshes[i]->Draw();
		}
	}
}

void SkinnedModel::calculateBoneMatrices()
{
	_boneMatrices.resize(64, glm::mat4(1.0f));
	for (size_t i = 0; i < _bones.size(); i++)
	{
		SkinnedModel_Bone& bone = _bones[i];

		// only bones with parents need to be transformed by their parents
		if (bone.parentBone != -1)
		{
			const SkinnedModel_Bone parentBone = _bones[bone.parentBone];

			bone.position = parentBone.position + parentBone.rotation * bone.position;
			bone.rotation = parentBone.rotation * bone.rotation;
		}

		glm::mat4 mat = glm::mat4(1.0f);
		mat           = glm::translate(mat, bone.position);
		mat           = mat * glm::mat4_cast(bone.rotation);

		_boneMatrices[i] = mat;
	}
}
