/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "L3DRayCast.h"

#include <L3DFile.h>
#include <glm/gtx/intersect.hpp>

std::optional<float> openblack::RayCast(const l3d::L3DFile& mesh, glm::vec3 origin, glm::vec3 direction)
{
	std::optional<float> nearest;
	const auto point = [](const l3d::L3DVertex& vertex) {
		return glm::vec3(vertex.position.x, vertex.position.y, vertex.position.z);
	};
	for (uint32_t subMesh = 0; subMesh < mesh.GetSubmeshHeaders().size(); ++subMesh)
	{
		const auto& vertices = mesh.GetVertexSpan(subMesh);
		const auto& indices = mesh.GetIndexSpan(subMesh);
		// Each primitive's triangles count from its own first vertex
		size_t firstIndex = 0;
		size_t firstVertex = 0;
		for (const auto& primitive : mesh.GetPrimitiveSpan(subMesh))
		{
			for (size_t i = 0; i + 2 < primitive.numTriangles * 3ul; i += 3)
			{
				const auto at = [&](size_t corner) {
					const auto index = firstVertex + indices[firstIndex + i + corner];
					return index < vertices.size() ? std::optional<glm::vec3>(point(vertices[index])) : std::nullopt;
				};
				const auto a = at(0);
				const auto b = at(1);
				const auto c = at(2);
				glm::vec2 barycentric;
				float distance = 0.0f;
				if (a && b && c && glm::intersectRayTriangle(origin, direction, *a, *b, *c, barycentric, distance) &&
				    distance > 0.0f && (!nearest.has_value() || distance < *nearest))
				{
					nearest = distance;
				}
			}
			firstIndex += primitive.numTriangles * 3ul;
			firstVertex += primitive.numVertices;
		}
	}
	return nearest;
}
