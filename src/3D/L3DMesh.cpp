/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
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

using namespace openblack;
using namespace openblack::graphics;

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

L3DMesh::L3DMesh(const std::string& debugName) :
	_debugName(debugName) {

}

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

	_flags = header.flags;

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
		for (size_t i = 0; i < offsets.size(); i++) {
			uint32_t id = stream.ReadValue<uint32_t>();
			std::vector<uint16_t> data(256 * 256); // RGBA4444
			stream.Read(data.data(), 256 * 256 * sizeof(uint16_t));

			_skins[id] = std::make_unique<Texture2D>(_debugName.c_str());
			_skins[id]->Create(256, 256, 1, Format::RGBA4, Wrapping::ClampEdge, data.data(), data.size() * sizeof(data[0]));
		}
	}

	// read submeshes
	if (header.submeshCount > 0) {
		std::vector<uint32_t> offsets(header.submeshCount);

		stream.Seek(header.submeshPointersOffset, SeekMode::Begin);
		stream.Read(offsets.data(), offsets.size() * sizeof(uint32_t));

		_subMeshes.resize(offsets.size());
		for (size_t i = 0; i < offsets.size(); i++) {
			stream.Seek(offsets[i], SeekMode::Begin);
			_subMeshes[i] = std::make_unique<L3DSubMesh>(*this);
			_subMeshes[i]->Load(stream);
		}
		bgfx::frame();
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

void L3DMesh::Draw(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const ShaderProgram &program, uint32_t mesh, uint64_t state, uint32_t rgba) const
{
	if (mesh >= _subMeshes.size()) {
		//spdlog::warn("tried to draw submesh out of range ({}/{})", mesh, _subMeshes.size());
		mesh = _subMeshes.size() - 1;
	}

	_subMeshes[mesh]->Submit(viewId, modelMatrix, program, state, rgba, false);
}

void L3DMesh::Submit(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const ShaderProgram& program, uint64_t state, uint32_t rgba) const
{
	for (auto it = _subMeshes.begin(); it != _subMeshes.end(); ++it)
	{
		const L3DSubMesh& submesh = *it->get();
		submesh.Submit(viewId, modelMatrix, program, state, rgba, std::next(it) != _subMeshes.end());
	}
}

void L3DMesh::Submit(graphics::RenderPass viewId, const bgfx::DynamicVertexBufferHandle& instanceBuffer, uint32_t instanceStart, uint32_t instanceCount,
                     const ShaderProgram& program, uint64_t state, uint32_t rgba) const
{
	for (auto it = _subMeshes.begin(); it != _subMeshes.end(); ++it)
	{
		const L3DSubMesh& submesh = *it->get();
		submesh.Submit(viewId, instanceBuffer, instanceStart, instanceCount, program, state, rgba, std::next(it) != _subMeshes.end());
	}
}
