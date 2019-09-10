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

#include <3D/L3DSubMesh.h>

#include <3D/L3DMesh.h>
#include <Common/IStream.h>
#include <Game.h>
#include <3D/MeshPack.h>

namespace openblack
{

struct L3DVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 norm;
};

L3DSubMesh::L3DSubMesh()
{
}

L3DSubMesh::~L3DSubMesh()
{
}

void L3DSubMesh::Load(IStream& stream)
{
	// IsPhysics: 18
	// IsWindow: 19
	// Status: 22-28 (6)
	// GetLOD: 29-32 (3)

	struct
	{
		uint32_t flags;
		uint32_t numPrimitives;
		uint32_t primitivesOffset;
		uint32_t numBones;
		uint32_t bonesOffset;
	} header;

	stream.Read(&header);

	// load up some buffers, todo: preallocate
	std::vector<L3DVertex> verticies;
	std::vector<uint16_t> indices;

	// load primitives first
	std::vector<uint32_t> offsets(header.numPrimitives);
	stream.Seek(header.primitivesOffset, SeekMode::Begin);
	stream.Read(offsets.data(), offsets.size() * sizeof(uint32_t));

	int primVertOffset  = 0;
	int primIndexOffset = 0;
	for (int i = 0; i < header.numPrimitives; i++)
	{
		struct
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
		} primitive;

		stream.Seek(offsets[i], SeekMode::Begin);
		stream.Read(&primitive);

		// todo: THIS IS SLOW
		stream.Seek(primitive.verticiesOffset, SeekMode::Begin);
		for (int j = 0; j < primitive.numVerticies; j++)
		{
			L3DVertex vertex;
			stream.Read(&vertex);
			verticies.push_back(vertex);
		}

		stream.Seek(primitive.trianglesOffset, SeekMode::Begin);
		for (int j = 0; j < primitive.numTriangles * 3; j++)
		{
			uint16_t index = stream.ReadValue<uint16_t>();
			indices.push_back(index + primVertOffset);
		}

		Primitive prim;
		prim.indicesCount  = primitive.numTriangles * 3;
		prim.indicesOffset = primIndexOffset;
		prim.skinID        = primitive.skinID;

		_primitives.push_back(prim);

		primVertOffset += primitive.numVerticies;
		primIndexOffset += primitive.numTriangles * 3;
	}

	VertexDecl decl;
	decl.reserve(3);
	decl.emplace_back(3, VertexAttrib::Type::Float);  // position
	decl.emplace_back(2, VertexAttrib::Type::Float);  // texture coordinate
	decl.emplace_back(3, VertexAttrib::Type::Float);  // normals

	// build our buffers
	auto vertexBuffer = new VertexBuffer(reinterpret_cast<const void*>(verticies.data()), verticies.size(), decl);
	auto indexBuffer  = new IndexBuffer(indices.data(), indices.size(), IndexBuffer::Type::Uint16);
	_mesh = std::make_unique<Mesh>(vertexBuffer, indexBuffer, Mesh::Topology::TriangleList);


	// uint32_t lod = (header.flags & 0xE0000000) >> 30;

	// spdlog::debug("flags: {:b}", header.flags);

	// offsets are local to stream :D

	//spdlog::debug("position: {}", stream.Position());
	//spdlog::debug("# prims: {} @ {}", header.numPrimitives, header.primitivesOffset);
	//spdlog::debug("# bones: {} @ {}", header.numBones, header.bonesOffset);
}

void L3DSubMesh::Draw(const L3DMesh& mesh, ShaderProgram& program) const
{
	if (!_mesh)
		return;

	auto const& skins = mesh.GetSkins();
	for (auto const& prim : _primitives)
	{
		if (prim.skinID != 0xFFFFFFFF)
		{
			if (skins.find(prim.skinID) != skins.end())
				skins.at(prim.skinID)->Bind(0);
			else
				Game::instance()->GetMeshPack().GetTexture(prim.skinID).Bind(0);
		}

		_mesh->Draw(prim.indicesCount, prim.indicesOffset);
	}
}

} // namespace openblack
