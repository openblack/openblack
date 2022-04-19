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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/vec_swizzle.hpp>
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
	auto& boneSpans = l3d.GetBoneSpan(meshIndex);

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
	if (_flags.hasBones)
	{
		for (auto& primitive : primitiveSpan)
		{
			uint32_t vertexOffset = 0;
			for (uint32_t i = 0; i < primitive.numGroups; ++i)
			{
				for (uint32_t j = 0; j < vertexGroupSpans[i].vertexCount; ++j)
				{
					auto matrix = glm::identity<glm::mat4>();
					for (uint32_t parent = vertexGroupSpans[i].boneIndex; parent != std::numeric_limits<uint32_t>::max();
					     parent = boneSpans[parent].parent)
					{
						const auto& bone = boneSpans[parent];
						const auto orientation = glm::make_mat3(bone.orientation);
						const auto translation = glm::make_vec3(&bone.position.x);
						const auto local = glm::translate(glm::identity<glm::mat4>(), translation) * glm::mat4(orientation);
						matrix = local * matrix;
					}

					const auto& vertex = verticesSpan[vertexOffset + j];
					const auto position = glm::xyz(matrix * glm::vec4(glm::make_vec3(&vertex.position.x), 1.0f));
					_boundingBox.maxima = glm::max(_boundingBox.maxima, position);
					_boundingBox.minima = glm::min(_boundingBox.minima, position);
				}
				vertexOffset += vertexGroupSpans[i].vertexCount;
			}
		}
	}
	else
	{
		for (uint32_t i = 0; i < nVertices; i++)
		{
			const auto position = glm::make_vec3(&verticesSpan[i].position.x);
			_boundingBox.maxima = glm::max(_boundingBox.maxima, position);
			_boundingBox.minima = glm::min(_boundingBox.minima, position);
		}
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
		verticesMemAccess[i].pos = glm::make_vec3(&verticesSpan[i].position.x);
		verticesMemAccess[i].uv = glm::make_vec2(&verticesSpan[i].texCoord.x);
		// TODO(bwrsandman): build normals from mesh
		verticesMemAccess[i].norm = glm::make_vec3(&verticesSpan[i].normal.x);
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
