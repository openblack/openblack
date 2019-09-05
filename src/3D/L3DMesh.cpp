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

#include <3D/L3DMesh.h>
#include <3D/MeshPack.h>
#include <Common/FileSystem.h>
#include <Common/IStream.h>
#include <Game.h>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

using namespace OpenBlack;
using namespace OpenBlack::Graphics;

struct L3DSubMesh
{
	uint32_t flags; // nodraw, transparent
	uint32_t numPrimitives;
	uint32_t primitivesOffset;
	uint32_t numBones;
	uint32_t bonesOffset;
};

struct L3DPrimitive
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

struct L3DModel_Vertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 norm;
	uint32_t bone;
};

void L3DMesh::LoadFromFile(const std::string& fileName)
{
	spdlog::debug("Loading L3DMesh from file: {}", fileName);
	auto file = Game::instance()->GetFileSystem().Open(fileName, FileMode::Read);
	Load(*file);
}

void L3DMesh::Load(IStream& stream)
{
	constexpr const char kMagic[4] = { 'L', '3', 'D', '0' };
	char magic[4];
	stream.Read(magic, 4);

	// check L3D0 validity first..
	if (std::memcmp(magic, kMagic, 4) != 0)
	{
		spdlog::error("l3dmodel: invalid magic number (expected {:.4} got {:.4})", kMagic, magic);
		return; // todo: return an error moadel?
	}

	struct L3DHeader
	{
		L3DMeshFlags flags;
		uint32_t size;
		uint32_t submeshCount;
		uint32_t submeshPointersOffset;
		uint8_t padding[32]; // always zero
		uint32_t anotherOffset;
		uint32_t skinsCount;
		uint32_t skinsOffset;
		uint32_t pointsCount;
		uint32_t pointsListOffset;
		uint32_t extraDataOffset;
	} header;

	stream.Read<L3DHeader>(&header);

#ifdef _DEBUG
	assert(std::all_of(&header.padding[0], &header.padding[32], [](int i) { return i == 0; }));
#endif

	/*if (IsBoned())
		spdlog::debug("\tcontains bones");

	if (IsContainsLandscapeFeature())
		spdlog::debug("\tcontains landscape feature");

	if (IsContainsUV2())
		spdlog::debug("\tcontains uv2");

	if (IsContainsNameData())
		spdlog::debug("\tcontains name data");

	if (IsContainsEBone())
		spdlog::debug("\tcontains ebone");

	if (IsContainsExtraMetrics())
		spdlog::debug("\tcontains extra metrics");*/

	// read textures
	if (header.skinsCount > 0) {
		std::vector<uint32_t> offsets(header.skinsCount);

		stream.Seek(header.skinsOffset, SeekMode::Begin);
		stream.Read(offsets.data(), offsets.size() * sizeof(uint32_t));

		spdlog::debug("\tLoading {} skins", header.skinsCount);
		for (int i = 0; i < offsets.size(); i++) {
			uint32_t id = stream.ReadValue<uint32_t>();
			std::vector<uint16_t> data(256 * 256); // RGBA4444
			stream.Read(data.data(), 256 * 256 * sizeof(uint16_t));

			_skins[id] = std::make_unique<Texture2D>();
			_skins[id]->Create(data.data(), DataType::UnsignedShort4444Rev, Format::BGRA, 256, 256, InternalFormat::RGB5A1);
		}
	}

	// read submeshes
	if (header.submeshCount > 0) {
		std::vector<uint32_t> offsets(header.submeshCount);

		stream.Seek(header.submeshPointersOffset, SeekMode::Begin);
		stream.Read(offsets.data(), offsets.size() * sizeof(uint32_t));

		for (int i = 0; i < offsets.size(); i++) {
			stream.Seek(offsets[i], SeekMode::Begin);

			L3DSubMesh submesh;
			stream.Read(&submesh);

			spdlog::debug("\tSubMesh[{}] has {} primitives and {} bones", i, submesh.numPrimitives, submesh.numBones);
		}
	}

	/*if (header.pointsCount > 0) {
		spdlog::debug("\t{} points at {:#x}", header.pointsCount, header.pointsListOffset);

		file.Seek(basePosition + header.pointsListOffset, FileSeekMode::Begin);

		for (auto i = 0; i < header.pointsCount; i++) {
			glm::vec3 point;
			file.Read<float>(glm::value_ptr(point), 3);
			spdlog::debug("\t\tpoint[{}] = [{}, {}, {}]", i, point.x, point.y, point.z);
		}
	}*/

	//if (header.extraDataOffset != -1)
	//	spdlog::debug("\textraDataOffset: {:#x}", header.extraDataOffset);

	// firstly they load skins
	// if IsPacked LH3DTexture::GetThisTexture(skinListOffset[i])
	// else create the texture from offset

	// then submeshes
	// for each mesh in offset list
	// LH3DSubMesh::Create, which in turn create LH3DPrimitive::Create (should remove naming confusion)

	// size: 2520, num meshes: 1, offset: 76
	// size: 6100, num meshes: 4, offset: 100
}

/*void L3DMesh::LoadFromL3D(const void* data_, size_t size)
{
	const uint8_t* buffer = static_cast<const uint8_t*>(data_);
	if (buffer[0] != 'L' || buffer[1] != '3' || buffer[2] != 'D' || buffer[3] != '0')
	{
		throw std::runtime_error("Invalid L3D file");
	}

	L3DHeader* header     = (L3DHeader*)(buffer + 4);
	uint32_t* meshOffsets = (uint32_t*)(buffer + header->meshListOffset);

	_flags = static_cast<L3DMeshFlags>(header->flags);

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
			decl[0] = VertexAttrib(0, 3, GL_FLOAT, false, false, sizeof(L3DModel_Vertex), (GLvoid*)offsetof(L3DModel_Vertex, pos));
			decl[1] = VertexAttrib(1, 2, GL_FLOAT, false, false, sizeof(L3DModel_Vertex), (GLvoid*)offsetof(L3DModel_Vertex, uv));
			decl[2] = VertexAttrib(2, 3, GL_FLOAT, false, false, sizeof(L3DModel_Vertex), (GLvoid*)offsetof(L3DModel_Vertex, norm));
			decl[3] = VertexAttrib(3, 1, GL_INT, true, false, sizeof(L3DModel_Vertex), (GLvoid*)offsetof(L3DModel_Vertex, bone));

			// create our vertex buffer real quick
			std::vector<L3DModel_Vertex> verts(subMesh->numVerticies);
			for (uint32_t i = 0; i < subMesh->numVerticies; i++)
			{
				// epic gamer
				memcpy(&verts[i], &verticiesOffset[i], sizeof(L3D_Vertex));
			}

			int vertind = 0;
			for (uint32_t boneVert = 0; boneVert < subMesh->boneVertLUTSize; boneVert++)
				for (uint32_t vert = 0; vert < boneVertOffset[boneVert].nVertices; vert++)
					verts[vertind++].bone = boneVertOffset[boneVert].boneIndex;

			VertexBuffer* vertexBuffer = new VertexBuffer(verts.data(), subMesh->numVerticies, sizeof(L3DModel_Vertex));
			IndexBuffer* indexBuffer   = new IndexBuffer(trianglesOffset, subMesh->numTriangles * 3, GL_UNSIGNED_SHORT);

			_submeshes.emplace_back(std::make_unique<Mesh>(vertexBuffer, indexBuffer, decl));
			_submeshSkinMap[sm] = subMesh->skinID;
		}

		// copy bones into our own format
		L3D_Bone* bones = (L3D_Bone*)(buffer + mesh->bonesOffset);
		_bones.resize(mesh->numBones);

		for (size_t i = 0; i < mesh->numBones; i++)
		{
			const L3D_Bone& bone   = bones[i];
			L3DModel_Bone& dstBone = _bones[i];

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
		_textures[skin->skinID] = std::make_unique<Texture2D>();
		_textures[skin->skinID]->Create(skin->data, DataType::UnsignedShort4444Rev, Format::BGRA, 256, 256, InternalFormat::RGB5A1);
	}
}*/

void L3DMesh::Draw(ShaderProgram* program) const
{
	// program->SetUniformValue("u_boneMatrices[0]", _boneMatrices.size(), _boneMatrices.data());
	auto& meshPackTextures = Game::instance()->GetMeshPack().GetTextures();

	glActiveTexture(GL_TEXTURE0);

	for (size_t i = 0; i < _submeshes.size(); i++)
	{
		auto const& submesh = _submeshes[i];
		auto skinID         = _submeshSkinMap.at(i);

		// assumption: texture should be set before, this isn't a nodraw.
		if (skinID == -1)
		{
			submesh->Draw();
			continue;
		}

		// try to find a texture locally or in mesh pack (todo: there are flags?)
		if (_skins.find(skinID) != _skins.end())
			_skins.at(skinID)->Bind();
		else if (i < meshPackTextures.size())
			meshPackTextures.at(skinID)->Bind();

		submesh->Draw();
	}
}

void L3DMesh::calculateBoneMatrices()
{
	_boneMatrices.resize(64, glm::mat4(1.0f));
	for (size_t i = 0; i < _bones.size(); i++)
	{
		L3DModel_Bone& bone = _bones[i];

		// only bones with parents need to be transformed by their parents
		if (bone.parentBone != -1)
		{
			const L3DModel_Bone parentBone = _bones[bone.parentBone];

			bone.position = parentBone.position + parentBone.rotation * bone.position;
			bone.rotation = parentBone.rotation * bone.rotation;
		}

		glm::mat4 mat = glm::mat4(1.0f);
		mat           = glm::translate(mat, bone.position);
		mat           = mat * glm::mat4_cast(bone.rotation);

		_boneMatrices[i] = mat;
	}
}
