/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "L3DSubMesh.h"

#include <algorithm>

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "Graphics/IndexBuffer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/VertexBuffer.h"
#include "L3DMesh.h"

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

/// A vertex of a mesh with lightmaps, with its lightmap coordinates
struct LightmappedL3DVertex
{
	EnhancedL3DVertex vertex;
	glm::vec2 lightmapUv;
};
static_assert(sizeof(LightmappedL3DVertex) == sizeof(EnhancedL3DVertex) + sizeof(glm::vec2));

L3DSubMesh::L3DSubMesh(L3DMesh& mesh) noexcept
    : _l3dMesh(mesh)
{
}

L3DSubMesh::~L3DSubMesh() noexcept = default;

std::optional<float> L3DSubMesh::Pick(glm::vec3 origin, glm::vec3 direction) const
{
	if (_pickTriangles.empty())
	{
		return std::nullopt;
	}
	// Past the submesh's box, the ray can't meet it
	const auto inverse = 1.0f / direction;
	const auto near = (_boundingBox.minima - origin) * inverse;
	const auto far = (_boundingBox.maxima - origin) * inverse;
	const float enter = glm::compMax(glm::min(near, far));
	const float leave = glm::compMin(glm::max(near, far));
	if (leave < 0.0f || enter > leave)
	{
		return std::nullopt;
	}
	std::optional<float> nearest;
	for (size_t i = 0; i + 2 < _pickTriangles.size(); i += 3)
	{
		// Möller and Trumbore's test, from either side, as LH3D picks
		const auto& a = _pickTriangles[i];
		const auto edge1 = _pickTriangles[i + 1] - a;
		const auto edge2 = _pickTriangles[i + 2] - a;
		const auto across = glm::cross(direction, edge2);
		const float determinant = glm::dot(edge1, across);
		if (std::abs(determinant) < 1e-12f)
		{
			continue;
		}
		const float inverseDeterminant = 1.0f / determinant;
		const auto fromCorner = origin - a;
		const float u = glm::dot(fromCorner, across) * inverseDeterminant;
		if (u < 0.0f || u > 1.0f)
		{
			continue;
		}
		const auto up = glm::cross(fromCorner, edge1);
		const float v = glm::dot(direction, up) * inverseDeterminant;
		if (v < 0.0f || u + v > 1.0f)
		{
			continue;
		}
		const float distance = glm::dot(edge2, up) * inverseDeterminant;
		if (distance > 0.0f && (!nearest.has_value() || distance < *nearest))
		{
			nearest = distance;
		}
	}
	return nearest;
}

bool L3DSubMesh::Load(const l3d::L3DFile& l3d, uint32_t meshIndex) noexcept
{
	const auto& header = l3d.GetSubmeshHeaders()[meshIndex];
	const auto primitiveSpan = l3d.GetPrimitiveSpan(meshIndex);
	const auto& verticesSpan = l3d.GetVertexSpan(meshIndex);
	const auto& indexSpan = l3d.GetIndexSpan(meshIndex);

	_flags = header.flags;

	// Count vertices and indices
	uint32_t nVertices = 0;
	uint32_t nIndices = 0;
	for (auto& primitive : primitiveSpan)
	{
		nVertices += primitive.numVertices;
		nIndices += primitive.numTriangles * 3;
	}

	_meshIndex = meshIndex;
	BoundVertices(l3d, meshIndex);

	if (nVertices == 0)
	{
		return false;
	}

	// The UV2 block's lightmap coordinates run over the vertices of every submesh in turn, and it has a lightmap for
	// each submesh, without a skin where there is none
	const auto& lightmapCoordinates = l3d.GetLightmapCoordinates();
	const auto vertexOffset = static_cast<size_t>(verticesSpan.data() - l3d.GetVertices().data());
	_hasLightmapCoordinates = !lightmapCoordinates.empty() && vertexOffset + nVertices <= lightmapCoordinates.size();
	if (_hasLightmapCoordinates && meshIndex < l3d.GetLightmaps().size() && l3d.GetLightmaps()[meshIndex].material.skinID != 0)
	{
		_lightmapSkinID = l3d.GetLightmaps()[meshIndex].material.skinID;
	}

	if (meshIndex < l3d.GetSubmeshNames().size())
	{
		const auto& name = l3d.GetSubmeshNames()[meshIndex];
		_name.assign(name.name.begin(), std::find(name.name.begin(), name.name.end(), '\0'));
		const auto point = [](const l3d::L3DPoint& p) { return glm::vec3(p.x, p.y, p.z); };
		_frame.toMesh = glm::mat4(glm::vec4(point(name.frameAxes[0]), 0.0f), glm::vec4(point(name.frameAxes[1]), 0.0f),
		                          glm::vec4(point(name.frameAxes[2]), 0.0f), glm::vec4(point(name.frameOrigin), 1.0f));
		_frame.min = point(name.frameMin);
		_frame.max = point(name.frameMax);
		if (name.jointIndex >= 0 && name.jointIndex < 0x100)
		{
			_joint = Joint {
			    .index = static_cast<uint32_t>(name.jointIndex),
			    .pivot = glm::vec3(name.jointPivot.x, name.jointPivot.y, name.jointPivot.z),
			};
		}
	}

	// Get vertices
	const bgfx::Memory* verticesMem = PackVertices(l3d, meshIndex);

	if (nIndices == 0)
	{
		return false;
	}

	// Get Indices
	const bgfx::Memory* indicesMem = bgfx::alloc(sizeof(uint16_t) * nIndices);
	auto* indices = reinterpret_cast<uint16_t*>(indicesMem->data);

	uint16_t startIndex = 0;
	uint16_t startVertex = 0;
	for (auto& primitive : primitiveSpan)
	{
		// Fix indices for merged vertex buffer
		for (uint32_t j = 0; j < primitive.numTriangles * 3; j++)
		{
			indices[startIndex + j] = indexSpan[startIndex + j] + startVertex;
		}

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
		        {false, false, L3DSubMesh::Primitive::BlendMode::Disabled, false, false}, // 0xe
		        {true, true, L3DSubMesh::Primitive::BlendMode::Standard, true, true},     // TexturedChromaAlpha
		        {false, true, L3DSubMesh::Primitive::BlendMode::Standard, true, true},    // TexturedChromaAlphaNz
		        {false, false, L3DSubMesh::Primitive::BlendMode::Disabled, false, false}, // 0x11
		        {true, true, L3DSubMesh::Primitive::BlendMode::Standard, false, true},    // ChromaJustZ
		    }};

		assert(static_cast<uint32_t>(primitive.material.type) != 0xe);
		assert(static_cast<uint32_t>(primitive.material.type) != 0x11);
		const auto& lutEntry = materialTypeLut.at(static_cast<uint32_t>(primitive.material.type));

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
		    (primitive.material.cullMode & 1U) != 0,
		});

		// The temple's rooms, the meshes with lightmaps, keep their triangles for the hand to find where the cursor
		// points at them
		if (_hasLightmapCoordinates && !_flags.hasBones)
		{
			for (uint32_t j = 0; j < primitive.numTriangles * 3; j++)
			{
				const auto vertex = indexSpan[startIndex + j] + startVertex;
				if (vertex < verticesSpan.size())
				{
					_pickTriangles.push_back(glm::make_vec3(&verticesSpan[vertex].position.x));
				}
			}
			_pickTriangles.resize(_pickTriangles.size() - (_pickTriangles.size() % 3));
		}
		startVertex += static_cast<uint16_t>(primitive.numVertices);
		startIndex += static_cast<uint16_t>(primitive.numTriangles * 3);
	}

	VertexDecl decl;
	decl.reserve(5);
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::TexCoord0, static_cast<uint8_t>(2), VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::Normal, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::Indices, static_cast<uint8_t>(2), VertexAttrib::Type::Int16);
	if (_hasLightmapCoordinates)
	{
		decl.emplace_back(VertexAttrib::Attribute::TexCoord3, static_cast<uint8_t>(2), VertexAttrib::Type::Float);
	}

	// build our buffers
	auto* vertexBuffer = new VertexBuffer(_l3dMesh.GetDebugName(), verticesMem, decl, _l3dMesh.IsDynamic());
	auto* indexBuffer = new IndexBuffer(_l3dMesh.GetDebugName(), indicesMem, IndexBuffer::Type::Uint16);
	_mesh = std::make_unique<graphics::Mesh>(vertexBuffer, indexBuffer);

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "{} submesh {} with {} verts and {} indices", _l3dMesh.GetDebugName(), meshIndex,
	                    nVertices, nIndices);
	return true;
}

void L3DSubMesh::BoundVertices(const l3d::L3DFile& l3d, uint32_t meshIndex)
{
	const auto primitiveSpan = l3d.GetPrimitiveSpan(meshIndex);
	const auto& verticesSpan = l3d.GetVertexSpan(meshIndex);
	const auto& vertexGroupSpans = l3d.GetVertexGroupSpan(meshIndex);
	const auto& boneSpans = l3d.GetBoneSpan(meshIndex);
	uint32_t nVertices = 0;
	for (const auto& primitive : primitiveSpan)
	{
		nVertices += primitive.numVertices;
	}
	_boundingBox.maxima = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	_boundingBox.minima = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	if (_flags.hasBones)
	{
		for (auto& primitive : primitiveSpan)
		{
			uint32_t vertexOffset = 0;
			for (uint32_t i = 0; i < primitive.numGroups; ++i)
			{
				auto matrix = glm::identity<glm::mat4>();
				for (uint32_t parent = vertexGroupSpans[i].boneIndex; parent != std::numeric_limits<uint32_t>::max();
				     parent = boneSpans[parent].parent)
				{
					const auto& bone = boneSpans[parent];
					const auto orientation = glm::make_mat3(bone.orientation.data());
					const auto translation = glm::make_vec3(&bone.position.x) * orientation;
					const auto local = glm::translate(glm::mat4(orientation), translation);
					matrix = local * matrix;
				}

				for (uint32_t j = 0; j < vertexGroupSpans[i].vertexCount; ++j)
				{
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
}

const bgfx::Memory* L3DSubMesh::PackVertices(const l3d::L3DFile& l3d, uint32_t meshIndex) const
{
	const auto& verticesSpan = l3d.GetVertexSpan(meshIndex);
	const auto& vertexGroupSpans = l3d.GetVertexGroupSpan(meshIndex);
	const auto& lightmapCoordinates = l3d.GetLightmapCoordinates();
	const auto vertexOffset = static_cast<size_t>(verticesSpan.data() - l3d.GetVertices().data());
	uint32_t nVertices = 0;
	for (const auto& primitive : l3d.GetPrimitiveSpan(meshIndex))
	{
		nVertices += primitive.numVertices;
	}

	const auto stride = _hasLightmapCoordinates ? sizeof(LightmappedL3DVertex) : sizeof(EnhancedL3DVertex);
	const bgfx::Memory* verticesMem = bgfx::alloc(static_cast<uint32_t>(stride * nVertices));
	const auto vertexAt = [verticesMem, stride](uint32_t i) {
		return reinterpret_cast<EnhancedL3DVertex*>(verticesMem->data + (stride * i));
	};
	for (uint32_t i = 0; i < nVertices; ++i)
	{
		auto& vertex = *vertexAt(i);
		vertex.pos = glm::make_vec3(&verticesSpan[i].position.x);
		vertex.uv = glm::make_vec2(&verticesSpan[i].texCoord.x);
		// TODO(bwrsandman): build normals from mesh
		vertex.norm = glm::make_vec3(&verticesSpan[i].normal.x);
		vertex.index.x = -1;
		vertex.index.y = -1;
		if (_hasLightmapCoordinates)
		{
			const auto& lightmapUv = lightmapCoordinates[vertexOffset + i];
			reinterpret_cast<LightmappedL3DVertex*>(&vertex)->lightmapUv = glm::vec2(lightmapUv.x, lightmapUv.y);
		}
	}

	// Fill bone index
	uint32_t vertexIndex = 0;
	for (const auto& vertexGroupSpan : vertexGroupSpans)
	{
		for (uint32_t i = 0; i < vertexGroupSpan.vertexCount; ++i)
		{
			vertexAt(vertexIndex)->index[0] = static_cast<int16_t>(vertexGroupSpan.boneIndex);
			vertexAt(vertexIndex)->index[1] = -1;
			vertexIndex++;
		}
	}
	return verticesMem;
}

void L3DSubMesh::UpdateVertices(const l3d::L3DFile& l3d) noexcept
{
	BoundVertices(l3d, _meshIndex);
	_mesh->GetVertexBuffer().Update(PackVertices(l3d, _meshIndex));
}

Mesh& L3DSubMesh::GetMesh() const
{
	return *_mesh;
}

} // namespace openblack
