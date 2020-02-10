/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "L3DSubMesh.h"

#include "Game.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/VertexBuffer.h"
#include "L3DMesh.h"
#include "MeshPack.h"

#include <glm/gtx/component_wise.hpp>
#include <spdlog/spdlog.h>

using namespace openblack::graphics;

namespace openblack
{

struct L3DVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 norm;
};

L3DSubMesh::L3DSubMesh(L3DMesh& mesh)
    : _l3dMesh(mesh)
{
}

L3DSubMesh::~L3DSubMesh() {}

void L3DSubMesh::Load(const l3d::L3DFile& l3d, uint32_t meshIndex)
{
	auto& header = l3d.GetSubmeshHeaders()[meshIndex];
	auto primitiveSpan = l3d.GetPrimitiveSpan(meshIndex);
	auto& verticesSpan = l3d.GetVertexSpan(meshIndex);
	auto& indexSpan = l3d.GetIndexSpan(meshIndex);

	_flags = header.flags;

	// Count vertices and indices
	uint32_t nVertices = 0;
	uint32_t nIndices = 0;
	for (auto& primitive : primitiveSpan)
	{
		nVertices += primitive.numVertices;
		nIndices += primitive.numTriangles * 3;
	}

	// Construct bounding box
	_boundingBox.maxima = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	_boundingBox.minima = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	for (uint32_t j = 0; j < nVertices; j++)
	{
		_boundingBox.maxima.x = glm::max(_boundingBox.maxima.x, verticesSpan[j].position.x);
		_boundingBox.maxima.y = glm::max(_boundingBox.maxima.y, verticesSpan[j].position.y);
		_boundingBox.maxima.z = glm::max(_boundingBox.maxima.z, verticesSpan[j].position.z);
		_boundingBox.minima.x = glm::min(_boundingBox.minima.x, verticesSpan[j].position.x);
		_boundingBox.minima.y = glm::min(_boundingBox.minima.y, verticesSpan[j].position.y);
		_boundingBox.minima.z = glm::min(_boundingBox.minima.z, verticesSpan[j].position.z);
	}

	// Get vertices
	// TODO: make ref for vertices
	const bgfx::Memory* verticesMem = bgfx::alloc(sizeof(L3DVertex) * nVertices);
	std::memcpy(verticesMem->data, verticesSpan.data(), verticesMem->size);

	// Get Indices
	const bgfx::Memory* indicesMem = bgfx::alloc(sizeof(uint16_t) * nIndices);
	auto indices = (uint16_t*)indicesMem->data;

	uint32_t startIndex = 0;
	uint32_t startVertex = 0;
	for (auto& primitive : primitiveSpan)
	{
		// Fix indices for merged vertex buffer
		for (uint32_t j = 0; j < primitive.numTriangles * 3; j++)
			indices[startIndex + j] = indexSpan[startIndex + j] + startVertex;

		_primitives.emplace_back(Primitive {primitive.skinID, startIndex, primitive.numTriangles * 3});

		startVertex += primitive.numVertices;
		startIndex += primitive.numTriangles * 3;
	}

	VertexDecl decl;
	decl.reserve(3);
	decl.emplace_back(VertexAttrib::Attribute::Position, 3, VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::TexCoord0, 2, VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::Normal, 3, VertexAttrib::Type::Float);

	// build our buffers
	auto vertexBuffer = new VertexBuffer(_l3dMesh.GetDebugName(), verticesMem, decl);
	auto indexBuffer = new IndexBuffer(_l3dMesh.GetDebugName(), indicesMem, IndexBuffer::Type::Uint16);
	_mesh = std::make_unique<graphics::Mesh>(vertexBuffer, indexBuffer);

	spdlog::debug("{} submesh {} with {} verts and {} indices", _l3dMesh.GetDebugName(), meshIndex, nVertices, nIndices);
}

void L3DSubMesh::Submit(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const ShaderProgram& program, uint64_t state,
                        uint32_t rgba, bool preserveState) const
{
	Submit_(viewId, &modelMatrix, nullptr, 0, 1, program, state, rgba, preserveState);
}

void L3DSubMesh::Submit(graphics::RenderPass viewId, const bgfx::DynamicVertexBufferHandle& instanceBuffer,
                        uint32_t instanceStart, uint32_t instanceCount, const graphics::ShaderProgram& program, uint64_t state,
                        uint32_t rgba, bool preserveState) const
{
	Submit_(viewId, nullptr, &instanceBuffer, instanceStart, instanceCount, program, state, rgba, preserveState);
}

void L3DSubMesh::Submit_(graphics::RenderPass viewId, const glm::mat4* modelMatrix,
                         const bgfx::DynamicVertexBufferHandle* instanceBuffer, uint32_t instanceStart, uint32_t instanceCount,
                         const ShaderProgram& program, uint64_t state, uint32_t rgba, bool preserveState) const
{
	if (!_mesh)
		return;

	// ignore physics meshes
	if (_flags.isPhysics)
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

	Mesh::DrawDesc desc = {
	    /*viewId =*/viewId,
	    /*program =*/program,
	    /*count =*/0,
	    /*offset =*/0,
	    /*instanceBuffer =*/instanceBuffer,
	    /*instanceStart =*/instanceStart,
	    /*instanceCount =*/instanceCount,
	    /*state =*/state,
	    /*rgba =*/rgba,
	    /*skip =*/Mesh::SkipState::SkipNone,
	    /*preserveState =*/preserveState,
	};

	bool lastPreserveState = false;
	for (auto it = _primitives.begin(); it != _primitives.end(); ++it)
	{
		const Primitive& prim = *it;

		const bool hasNext = std::next(it) != _primitives.end();

		const Texture2D* texture = getTexture(prim.skinID);
		const Texture2D* nextTexture = !hasNext ? nullptr : getTexture(std::next(it)->skinID);

		bool primitivePreserveState = texture == nextTexture && (preserveState || hasNext);

		desc.skip = Mesh::SkipState::SkipNone;
		if (!lastPreserveState)
		{
			if (modelMatrix)
			{
				bgfx::setTransform(modelMatrix);
			}
			if (texture)
			{
				program.SetTextureSampler("s_diffuse", 0, *texture);
			}
		}
		else
		{
			desc.skip |= Mesh::SkipState::SkipRenderState;
			desc.skip |= Mesh::SkipState::SkipVertexBuffer;
		}

		desc.count = prim.indicesCount;
		desc.offset = prim.indicesOffset;
		desc.preserveState = primitivePreserveState;

		_mesh->Draw(desc);
		lastPreserveState = primitivePreserveState;
	}
}

Mesh& L3DSubMesh::GetMesh() const
{
	return *_mesh;
}

} // namespace openblack
