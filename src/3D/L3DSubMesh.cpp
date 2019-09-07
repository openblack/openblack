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
#include <3D/L3DSubMesh.h>
#include <Common/IStream.h>
#include <Game.h>
#include <3D/MeshPack.h>
#include <spdlog/spdlog.h>

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
	glGenVertexArrays(1, &_vertexArray);
}

L3DSubMesh::~L3DSubMesh()
{
	if (_vertexArray != 0)
		glDeleteVertexArrays(1, &_vertexArray);
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

	// build our buffers
	_vertexBuffer = std::make_unique<VertexBuffer>(reinterpret_cast<const void*>(verticies.data()), verticies.size(), sizeof(L3DVertex), GL_STATIC_DRAW);
	_indexBuffer  = std::make_unique<IndexBuffer>(indices.data(), indices.size(), GL_UNSIGNED_SHORT);

	glBindVertexArray(_vertexArray);
	_vertexBuffer->Bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(L3DVertex), reinterpret_cast<const void*>(offsetof(L3DVertex, pos)));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(L3DVertex), reinterpret_cast<const void*>(offsetof(L3DVertex, uv)));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(L3DVertex), reinterpret_cast<const void*>(offsetof(L3DVertex, norm)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer->GetIBO());
	glBindVertexArray(0);

	// uint32_t lod = (header.flags & 0xE0000000) >> 30;

	// spdlog::debug("flags: {:b}", header.flags);

	// offsets are local to stream :D

	//spdlog::debug("position: {}", stream.Position());
	//spdlog::debug("# prims: {} @ {}", header.numPrimitives, header.primitivesOffset);
	//spdlog::debug("# bones: {} @ {}", header.numBones, header.bonesOffset);
}

void L3DSubMesh::Draw(const L3DMesh& mesh, ShaderProgram& program) const
{
	if (_vertexBuffer == nullptr)
		return;

	auto const& skins = mesh.GetSkins();
	glBindVertexArray(_vertexArray);
	for (auto const& prim : _primitives)
	{
		if (prim.skinID != 0xFFFFFFFF)
		{
			glActiveTexture(GL_TEXTURE0);
			if (skins.find(prim.skinID) != skins.end())
				skins.at(prim.skinID)->Bind();
			else
				Game::instance()->GetMeshPack().GetTexture(prim.skinID).Bind();
		}

		glDrawElements(GL_TRIANGLES, prim.indicesCount, _indexBuffer->GetType(), (void*)(prim.indicesOffset * sizeof(uint16_t)));
	}
}

} // namespace openblack
