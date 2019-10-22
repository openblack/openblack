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

#include <spdlog/spdlog.h>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>

#include <3D/L3DMesh.h>
#include <3D/MeshPack.h>
#include <Common/IStream.h>
#include <Game.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/VertexBuffer.h>

namespace openblack
{

struct L3DVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 norm;
};

L3DSubMesh::L3DSubMesh(L3DMesh& mesh):
    _l3dMesh(mesh)
{
}

L3DSubMesh::~L3DSubMesh()
{
}

struct L3DPrimitive
{
	uint32_t unknown_1;
	uint32_t unknown_2;
	uint32_t skinID;
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

void L3DSubMesh::Load(IStream& stream)
{
	struct
	{
		uint32_t flags;
		uint32_t numPrimitives;
		uint32_t primitivesOffset;
		uint32_t numBones;
		uint32_t bonesOffset;
	} header;

	stream.Read(&header);

	_flags = header.flags;

	// load primitives first
	std::vector<uint32_t> offsets(header.numPrimitives);
	stream.Seek(header.primitivesOffset, SeekMode::Begin);
	stream.Read(offsets.data(), offsets.size() * sizeof(uint32_t));

	std::vector<L3DPrimitive> l3dPrims(header.numPrimitives);

	std::size_t nVertices = 0, nIndices = 0;
	for (uint32_t i = 0; i < header.numPrimitives; i++)
	{
		stream.Seek(offsets[i], SeekMode::Begin);
		stream.Read(&l3dPrims[i]);

		nVertices += l3dPrims[i].numVerticies;
		nIndices += l3dPrims[i].numTriangles * 3;
	}

	const bgfx::Memory* verticesMem = bgfx::alloc(sizeof(L3DVertex) * nVertices);
	const bgfx::Memory* indicesMem  = bgfx::alloc(sizeof(uint16_t) * nIndices);

	auto vertices = (L3DVertex*)verticesMem->data;
	auto indices  = (uint16_t*)indicesMem->data;

	uint32_t startIndex = 0, startVertex = 0;
	_boundingBox.maxima = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	_boundingBox.minima = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	for (const auto& prim : l3dPrims)
	{
		stream.Seek(prim.verticiesOffset, SeekMode::Begin);
		for (uint32_t j = 0; j < prim.numVerticies; j++)
		{
			stream.Read(&vertices[startVertex + j]);
			_boundingBox.maxima.x = glm::max(_boundingBox.maxima.x, vertices[startVertex + j].pos.x);
			_boundingBox.maxima.y = glm::max(_boundingBox.maxima.y, vertices[startVertex + j].pos.y);
			_boundingBox.maxima.z = glm::max(_boundingBox.maxima.z, vertices[startVertex + j].pos.z);
			_boundingBox.minima.x = glm::min(_boundingBox.minima.x, vertices[startVertex + j].pos.x);
			_boundingBox.minima.y = glm::min(_boundingBox.minima.y, vertices[startVertex + j].pos.y);
			_boundingBox.minima.z = glm::min(_boundingBox.minima.z, vertices[startVertex + j].pos.z);
		}

		stream.Seek(prim.trianglesOffset, SeekMode::Begin);
		for (uint32_t j = 0; j < prim.numTriangles * 3; j++)
			indices[startIndex + j] = stream.ReadValue<uint16_t>() + startVertex;

		Primitive p;
		p.indicesCount  = prim.numTriangles * 3;
		p.indicesOffset = startIndex;
		p.skinID        = prim.skinID;

		_primitives.emplace_back(p);

		startVertex += prim.numVerticies;
		startIndex += prim.numTriangles * 3;
	}

	VertexDecl decl;
	decl.reserve(3);
	decl.emplace_back(VertexAttrib::Attribute::Position, 3, VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::TexCoord0, 2, VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::Normal, 3, VertexAttrib::Type::Float);

	// build our buffers
	_vertexBuffer = std::make_unique<VertexBuffer>(_l3dMesh.GetDebugName(), verticesMem, decl);
	_indexBuffer  = std::make_unique<IndexBuffer>(_l3dMesh.GetDebugName(), indicesMem, IndexBuffer::Type::Uint16);

	spdlog::debug("{} with {} verts and {} indices", _l3dMesh.GetDebugName(), nVertices, nIndices);

	// uint32_t lod = (header.flags & 0xE0000000) >> 30;

	// spdlog::debug("flags: {:b}", header.flags);

	// offsets are local to stream :D

	//spdlog::debug("position: {}", stream.Position());
	//spdlog::debug("# prims: {} @ {}", header.numPrimitives, header.primitivesOffset);
	//spdlog::debug("# bones: {} @ {}", header.numBones, header.bonesOffset);
}

void L3DSubMesh::Submit(uint8_t viewId, const glm::mat4& modelMatrix, const ShaderProgram& program, uint64_t state, uint32_t rgba, bool preserveState) const
{
	if (!_vertexBuffer || !_indexBuffer)
		return;

	// ignore physics meshes
	if (IsPhysics())
		return;

	auto const& skins = _l3dMesh.GetSkins();

	auto getTexture = [&skins](uint32_t skinID) -> const Texture2D* {
		if (skinID != 0xFFFFFFFF)
		{
			if (skins.find(skinID) != skins.end())
				return skins.at(skinID).get();
			else
				return &Game::instance()->GetMeshPack().GetTexture(skinID);
		}
		return nullptr;
	};

	bool lastPreserveState = false;
	for (auto it = _primitives.begin(); it != _primitives.end(); ++it)
	{
		const Primitive& prim = *it;

		const bool hasNext = std::next(it) != _primitives.end();

		const Texture2D* texture = getTexture(prim.skinID);
		const Texture2D* nextTexture = !hasNext ? nullptr : getTexture(std::next(it)->skinID);

		bool primitivePreserveState = texture == nextTexture && (preserveState || hasNext);

		if (!lastPreserveState)
		{
			bgfx::setTransform(&modelMatrix);
			bgfx::setState(state, rgba);
			_vertexBuffer->Bind();
			if (texture)
			{
				program.SetTextureSampler("s_diffuse", 0, *texture);
			}
		}

		_indexBuffer->Bind(prim.indicesCount, prim.indicesOffset);
		bgfx::submit(viewId, program.GetRawHandle(), 0, primitivePreserveState);
		lastPreserveState = primitivePreserveState;
	}
}

uint32_t L3DSubMesh::GetVertexCount() const
{
	return _vertexBuffer->GetCount();
}

uint32_t L3DSubMesh::GetIndexCount() const
{
	return _indexBuffer->GetCount();
}

} // namespace openblack
