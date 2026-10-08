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
#include <limits>

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "Graphics/IndexBuffer.h"
#include "Graphics/RenderModes.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexBuffer.h"
#include "L3DMesh.h"
#include "VertexBlend.h"

using namespace openblack::graphics;

namespace openblack
{

struct EnhancedL3DVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 norm;
	/// The bone, the vertex it is blended towards or -1, and how far in 32767ths
	glm::i16vec4 index;
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
		// Möller and Trumbore's test, from either side, as the game picks
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

	// A model that doesn't move keeps its vertices and how its primitives share them, for effects that crawl over its
	// surface
	_surfacePoints.clear();
	_surfacePrimitives.clear();
	if (!_flags.hasBones)
	{
		_surfacePoints.reserve(verticesSpan.size());
		for (const auto& vertex : verticesSpan)
		{
			_surfacePoints.push_back(
			    {.position = glm::make_vec3(&vertex.position.x), .normal = glm::make_vec3(&vertex.normal.x)});
		}
		uint32_t first = 0;
		for (const auto& primitive : primitiveSpan)
		{
			_surfacePrimitives.push_back({.first = first, .count = primitive.numVertices});
			first += primitive.numVertices;
		}
	}

	// Get vertices
	const bgfx::Memory* verticesMem = PackVertices(l3d, meshIndex);
	if (_flags.hasBones)
	{
		CreateBlendSource(verticesMem, nVertices);
	}

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

		const auto& mode = graphics::render_modes::Desc(static_cast<graphics::render_modes::Mode>(primitive.material.type));
		const auto blend = [&mode] {
			using graphics::render_modes::Blend;
			switch (mode.blend)
			{
			case Blend::Standard:
				return Primitive::BlendMode::Standard;
			case Blend::Additive:
				return Primitive::BlendMode::Additive;
			case Blend::JustZ:
				return Primitive::BlendMode::JustZ;
			case Blend::Disabled:
				break;
			}
			return Primitive::BlendMode::Disabled;
		}();

		// TODO(bwrsandman): Interpret cull mode, color byte ordering and render mode, then store in primitive
		_primitives.emplace_back(Primitive {
		    primitive.material.skinID,
		    startIndex,
		    primitive.numTriangles * 3,
		    mode.zWrite,
		    mode.alphaTest,
		    blend,
		    mode.alphaModulate,
		    mode.alphaTest,
		    primitive.material.alphaCutoutThreshold / 255.0f,
		    (primitive.material.cullMode & 1U) != 0,
		    static_cast<uint32_t>(primitive.material.type),
		});

		// The temple's rooms, the meshes with lightmaps, keep their triangles for the hand to find where the cursor
		// points at them
		if (_hasLightmapCoordinates && !_flags.hasBones)
		{
			for (uint32_t j = 0; j < primitive.numTriangles * 3; j++)
			{
				const auto vertex = indexSpan[startIndex + j] + startVertex;
				if (vertex >= 0 && static_cast<size_t>(vertex) < verticesSpan.size())
				{
					_pickTriangles.push_back(glm::make_vec3(&verticesSpan[vertex].position.x));
				}
			}
			_pickTriangles.resize(_pickTriangles.size() - (_pickTriangles.size() % 3));
		}
		startVertex += static_cast<uint16_t>(primitive.numVertices);
		startIndex += static_cast<uint16_t>(primitive.numTriangles * 3);
	}

	// Every model keeps its vertices as the file holds them for the physics, boned or not, with each vertex's bone
	{
		const auto count = std::min<size_t>(nVertices, verticesSpan.size());
		_bodyGeometry.positions.clear();
		_bodyGeometry.positions.reserve(count);
		_bodyGeometry.uvs.clear();
		_bodyGeometry.uvs.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			_bodyGeometry.positions.emplace_back(verticesSpan[i].position.x, verticesSpan[i].position.y,
			                                     verticesSpan[i].position.z);
			_bodyGeometry.uvs.emplace_back(verticesSpan[i].texCoord.x, verticesSpan[i].texCoord.y);
		}
		_bodyGeometry.indices.assign(indices, indices + nIndices);
		_bodyGeometry.bones.clear();
		if (_flags.hasBones)
		{
			// Each primitive's vertices are moved by its groups' bones in turn, a run of vertices each
			const auto& groups = l3d.GetVertexGroupSpan(meshIndex);
			size_t group = 0;
			for (const auto& primitive : primitiveSpan)
			{
				size_t inPrimitive = 0;
				for (uint32_t g = 0; g < primitive.numGroups && group < groups.size(); ++g, ++group)
				{
					for (uint32_t v = 0; v < groups[group].vertexCount && inPrimitive < primitive.numVertices; ++v)
					{
						_bodyGeometry.bones.push_back(groups[group].boneIndex);
						++inPrimitive;
					}
				}
				// Vertices no group names belong to no bone
				for (; inPrimitive < primitive.numVertices; ++inPrimitive)
				{
					_bodyGeometry.bones.push_back(k_NoBone);
				}
			}
			_bodyGeometry.bones.resize(count, k_NoBone);
		}
	}

	// A model that doesn't move by bones keeps its triangles for the flames set on it and the pieces it breaks into
	if (!_flags.hasBones)
	{
		const auto count = std::min<size_t>(nVertices, verticesSpan.size());
		_surface.positions.reserve(count);
		_surface.uvs.reserve(count);
		_surface.normals.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			_surface.positions.emplace_back(verticesSpan[i].position.x, verticesSpan[i].position.y, verticesSpan[i].position.z);
			_surface.uvs.emplace_back(verticesSpan[i].texCoord.x, verticesSpan[i].texCoord.y);
			_surface.normals.emplace_back(verticesSpan[i].normal.x, verticesSpan[i].normal.y, verticesSpan[i].normal.z);
		}
		_surface.indices.assign(indices, indices + nIndices);
	}

	VertexDecl decl;
	decl.reserve(5);
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::TexCoord0, static_cast<uint8_t>(2), VertexAttrib::Type::Float);
	decl.emplace_back(VertexAttrib::Attribute::Normal, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	// The bone index, morph partner vertex and morph weight reach the vertex shader as integers (ivec4 a_indices), so the
	// attribute's format matches the shader's input on every backend
	decl.emplace_back(VertexAttrib::Attribute::Indices, static_cast<uint8_t>(4), VertexAttrib::Type::Int16,
	                  /*normalized=*/false, /*asInt=*/true);
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
		vertex.index = glm::i16vec4(-1, -1, 0, 0);
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
			vertexIndex++;
		}
	}

	// The vertices blended towards others at the seams
	std::vector<uint32_t> primitiveVertices;
	std::vector<uint32_t> primitiveBlends;
	for (const auto& primitive : l3d.GetPrimitiveSpan(meshIndex))
	{
		primitiveVertices.push_back(primitive.numVertices);
		const bool read = primitive.vertexBlendsOffset != std::numeric_limits<uint32_t>::max();
		primitiveBlends.push_back(read ? primitive.numVertexBlends : 0);
	}
	std::vector<vertex_blend::Blend> blends;
	for (const auto& blend : l3d.GetBlendSpan(meshIndex))
	{
		blends.push_back({.vertex = blend.indices[0], .towards = blend.indices[1], .weight = blend.weight});
	}
	const auto partners = vertex_blend::Partners(primitiveVertices, primitiveBlends, blends);
	for (uint32_t i = 0; i < nVertices && i < partners.size(); ++i)
	{
		const auto& partner = partners[i];
		if (partner.Blended() && partner.vertex <= std::numeric_limits<int16_t>::max())
		{
			vertexAt(i)->index[1] = static_cast<int16_t>(partner.vertex);
			vertexAt(i)->index[2] = vertex_blend::QuantiseWeight(partner.weight);
		}
	}
	return verticesMem;
}

void L3DSubMesh::CreateBlendSource(const bgfx::Memory* vertices, uint32_t count)
{
	_hasBlends = false;
	const auto stride = _hasLightmapCoordinates ? sizeof(LightmappedL3DVertex) : sizeof(EnhancedL3DVertex);
	const auto* caps = bgfx::getCaps();
	if (count == 0 || caps == nullptr || count > caps->limits.maxTextureSize ||
	    (caps->formats[bgfx::TextureFormat::RGBA32F] & BGFX_CAPS_FORMAT_TEXTURE_VERTEX) == 0)
	{
		_blendSource.reset();
		return;
	}
	std::vector<glm::vec4> texels(count);
	for (uint32_t i = 0; i < count; ++i)
	{
		const auto& vertex = *reinterpret_cast<const EnhancedL3DVertex*>(vertices->data + (stride * i));
		texels[i] = glm::vec4(vertex.pos, static_cast<float>(std::max<int16_t>(vertex.index.x, 0)));
		_hasBlends = _hasBlends || vertex.index.y >= 0;
	}
	if (!_blendSource || _blendSourceWidth != count)
	{
		_blendSource = std::make_unique<Texture2D>(_l3dMesh.GetDebugName() + " blend source");
		_blendSource->CreateWithinFrame(static_cast<uint16_t>(count), 1, 1, TextureFormat::RGBA32F, Wrapping::ClampEdge,
		                                Filter::Nearest, nullptr);
		_blendSourceWidth = count;
	}
	_blendSource->Update(texels.data(), static_cast<uint32_t>(texels.size() * sizeof(texels[0])));
}

void L3DSubMesh::UpdateVertices(const l3d::L3DFile& l3d) noexcept
{
	BoundVertices(l3d, _meshIndex);
	const auto* vertices = PackVertices(l3d, _meshIndex);
	if (_flags.hasBones && _blendSource)
	{
		CreateBlendSource(vertices, _blendSourceWidth);
	}
	_mesh->GetVertexBuffer().Update(vertices);
}

Mesh& L3DSubMesh::GetMesh() const
{
	return *_mesh;
}

} // namespace openblack
