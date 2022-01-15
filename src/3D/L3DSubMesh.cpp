/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
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

struct EnhancedL3DVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 norm;
	glm::i16vec2 index;
};

L3DSubMesh::L3DSubMesh(L3DMesh& mesh)
    : _l3dMesh(mesh)
{
}

L3DSubMesh::~L3DSubMesh() {}

bool L3DSubMesh::Load(const l3d::L3DFile& l3d, uint32_t meshIndex)
{
	auto& header = l3d.GetSubmeshHeaders()[meshIndex];
	auto primitiveSpan = l3d.GetPrimitiveSpan(meshIndex);
	auto& verticesSpan = l3d.GetVertexSpan(meshIndex);
	auto& indexSpan = l3d.GetIndexSpan(meshIndex);
	auto& vertexGroupSpans = l3d.GetVertexGroupSpan(meshIndex);

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

	if (nVertices == 0)
	{
		return false;
	}

	// Get vertices
	const bgfx::Memory* verticesMem = bgfx::alloc(sizeof(EnhancedL3DVertex) * nVertices);
	auto* verticesMemAccess = reinterpret_cast<EnhancedL3DVertex*>(verticesMem->data);
	for (uint32_t i = 0; i < nVertices; ++i)
	{
		verticesMemAccess[i].pos.x = verticesSpan[i].position.x;
		verticesMemAccess[i].pos.y = verticesSpan[i].position.y;
		verticesMemAccess[i].pos.z = verticesSpan[i].position.z;
		verticesMemAccess[i].uv.x = verticesSpan[i].texCoord.x;
		verticesMemAccess[i].uv.y = verticesSpan[i].texCoord.y;
		verticesMemAccess[i].norm.x = verticesSpan[i].normal.x;
		verticesMemAccess[i].norm.y = verticesSpan[i].normal.y;
		verticesMemAccess[i].norm.z = verticesSpan[i].normal.z;
		verticesMemAccess[i].index.x = -1;
		verticesMemAccess[i].index.y = -1;
	}

	if (nIndices == 0)
	{
		return false;
	}

	// Get Indices
	const bgfx::Memory* indicesMem = bgfx::alloc(sizeof(uint16_t) * nIndices);
	auto indices = (uint16_t*)indicesMem->data;

	// Fill bone index
	uint32_t vertexIndex = 0;
	for (auto& vertexGroupSpan : vertexGroupSpans)
	{
		for (uint32_t i = 0; i < vertexGroupSpan.vertexCount; ++i)
		{
			verticesMemAccess[vertexIndex].index[0] = vertexGroupSpan.boneIndex;
			verticesMemAccess[vertexIndex].index[1] = -1;
			vertexIndex++;
		}
	}

	uint16_t startIndex = 0;
	uint16_t startVertex = 0;
	for (auto& primitive : primitiveSpan)
	{
		// Fix indices for merged vertex buffer
		for (uint32_t j = 0; j < primitive.numTriangles * 3; j++)
			indices[startIndex + j] = indexSpan[startIndex + j] + startVertex;

		struct MaterialTypeLutEntry
		{
			bool depthWrite;
			bool alphaTest;
			L3DSubMesh::Primitive::BlendMode blend;
			bool modulateAlpha;  ///< Multiply ouput alpha by a uniform
			bool thresholdAlpha; ///< Dismiss fragments below a certain threshold
		};
		static const std::array<MaterialTypeLutEntry, static_cast<uint32_t>(l3d::L3DMaterial::Type::_Count)> materialTypeLut = {
		    {
		        {true, false, L3DSubMesh::Primitive::BlendMode::Disabled, false, false},  // Smooth
		        {true, false, L3DSubMesh::Primitive::BlendMode::Standard, false, false},  // SmoothAlpha
		        {true, false, L3DSubMesh::Primitive::BlendMode::Disabled, false, false},  // Textured
		        {true, false, L3DSubMesh::Primitive::BlendMode::Standard, true, false},   // TexturedAlpha
		        {true, false, L3DSubMesh::Primitive::BlendMode::Standard, false, false},  // AlphaTextured
		        {true, false, L3DSubMesh::Primitive::BlendMode::Standard, true, false},   // AlphaTexturedAlpha
		        {false, false, L3DSubMesh::Primitive::BlendMode::Standard, true, false},  // AlphaTexturedAlphaNz
		        {false, false, L3DSubMesh::Primitive::BlendMode::Standard, false, false}, // SmoothAlphaNz
		        {false, false, L3DSubMesh::Primitive::BlendMode::Standard, true, false},  // TexturedAlphaNz
		        {true, true, L3DSubMesh::Primitive::BlendMode::Standard, false, true},    // TexturedChroma
		        {true, true, L3DSubMesh::Primitive::BlendMode::Additive, true, true},     // AlphaTexturedAlphaAdditiveChroma
		        {false, true, L3DSubMesh::Primitive::BlendMode::Additive, true, true},    // AlphaTexturedAlphaAdditiveChromaNz
		        {true, false, L3DSubMesh::Primitive::BlendMode::Additive, true, false},   // AlphaTexturedAlphaAdditive
		        {false, false, L3DSubMesh::Primitive::BlendMode::Additive, true, false},  // AlphaTexturedAlphaAdditiveNz
		        {0, 0, L3DSubMesh::Primitive::BlendMode::Disabled, 0, 0},                 // 0xe
		        {true, true, L3DSubMesh::Primitive::BlendMode::Standard, true, true},     // TexturedChromaAlpha
		        {false, true, L3DSubMesh::Primitive::BlendMode::Standard, true, true},    // TexturedChromaAlphaNz
		        {0, 0, L3DSubMesh::Primitive::BlendMode::Disabled, 0, 0},                 // 0x11
		        {true, true, L3DSubMesh::Primitive::BlendMode::Standard, false, true},    // ChromaJustZ
		    }};

		assert(static_cast<uint32_t>(primitive.material.type) != 0xe);
		assert(static_cast<uint32_t>(primitive.material.type) != 0x11);
		const auto& lutEntry = materialTypeLut[static_cast<uint32_t>(primitive.material.type)];

		// TODO(bwrsandman): Interpret cull mode, color byte ordering and render mode, then store in primitive
		_primitives.emplace_back(Primitive {
		    primitive.material.skinID,
		    startIndex,
		    primitive.numTriangles * 3,
		    lutEntry.depthWrite,
		    lutEntry.alphaTest,
		    lutEntry.blend,
		    lutEntry.modulateAlpha,
		    lutEntry.thresholdAlpha,
		    primitive.material.alphaCutoutThreshold / 255.0f,
		});

		startVertex += static_cast<uint16_t>(primitive.numVertices);
		startIndex += static_cast<uint16_t>(primitive.numTriangles * 3);
	}

	VertexDecl decl;
	decl.reserve(4);
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::TexCoord0, static_cast<uint8_t>(2), VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::Normal, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::Indices, static_cast<uint8_t>(2), VertexAttrib::Type::Int16);

	// build our buffers
	auto vertexBuffer = new VertexBuffer(_l3dMesh.GetDebugName(), verticesMem, decl);
	auto indexBuffer = new IndexBuffer(_l3dMesh.GetDebugName(), indicesMem, IndexBuffer::Type::Uint16);
	_mesh = std::make_unique<graphics::Mesh>(vertexBuffer, indexBuffer);

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "{} submesh {} with {} verts and {} indices", _l3dMesh.GetDebugName(), meshIndex,
	                    nVertices, nIndices);
	return true;
}

Mesh& L3DSubMesh::GetMesh() const
{
	return *_mesh;
}

} // namespace openblack
