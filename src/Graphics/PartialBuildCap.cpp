/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PartialBuildCap.h"

#include <array>

namespace openblack::graphics::partial_build_cap
{

namespace
{
struct Segment
{
	CapVertex outerA;
	CapVertex outerB;
	CapVertex innerA;
	CapVertex innerB;
};

/// Where an edge from one corner to another crosses the height, as far along as the corners' heights put it
CapVertex Along(const CapVertex& from, const CapVertex& to, float height)
{
	const float t = (height - from.position.y) / (to.position.y - from.position.y);
	return {
	    .position = from.position + (to.position - from.position) * t,
	    .uv = from.uv + (to.uv - from.uv) * t,
	    .normal = from.normal + (to.normal - from.normal) * t,
	};
}

/// The same point on the inner wall: moved in across the ground along its normal, its height kept
CapVertex Inner(const CapVertex& outer, float inset)
{
	auto inner = outer;
	inner.position.x -= outer.normal.x * inset;
	inner.position.z -= outer.normal.z * inset;
	return inner;
}
} // namespace

bool HasWholeTriangleBelow(std::span<const glm::vec3> positions, std::span<const uint16_t> indices, float height)
{
	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		const auto below = [&](size_t corner) {
			const auto index = indices[i + corner];
			return index < positions.size() && positions[index].y < height;
		};
		if (below(0) && below(1) && below(2))
		{
			return true;
		}
	}
	return false;
}

std::vector<CapVertex> Build(std::span<const glm::vec3> positions, std::span<const glm::vec2> uvs,
                             std::span<const glm::vec3> normals, std::span<const uint16_t> indices, float height, float inset)
{
	std::vector<Segment> segments;
	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		std::array<CapVertex, 3> corners {};
		bool valid = true;
		for (size_t c = 0; c < corners.size(); ++c)
		{
			const auto index = indices[i + c];
			if (index >= positions.size() || index >= uvs.size() || index >= normals.size())
			{
				valid = false;
				break;
			}
			corners.at(c) = {.position = positions[index], .uv = uvs[index], .normal = normals[index]};
		}
		if (!valid)
		{
			continue;
		}
		// A corner at the height counts as above it
		const auto above = [height](const CapVertex& corner) { return corner.position.y >= height; };
		const int count =
		    static_cast<int>(above(corners[0])) + static_cast<int>(above(corners[1])) + static_cast<int>(above(corners[2]));
		if (count == 0 || count == 3)
		{
			continue;
		}
		// The two edges whose ends lie either side of the height, in the triangle's order
		std::array<CapVertex, 2> cut {};
		size_t found = 0;
		for (size_t e = 0; e < corners.size() && found < cut.size(); ++e)
		{
			const auto& from = corners.at(e);
			const auto& to = corners.at((e + 1) % corners.size());
			if (above(from) != above(to))
			{
				cut.at(found++) = Along(from, to, height);
			}
		}
		segments.push_back({cut[0], cut[1], Inner(cut[0], inset), Inner(cut[1], inset)});
	}
	if (segments.empty() || segments.size() > k_MostSegments)
	{
		return {};
	}
	std::vector<CapVertex> vertices;
	vertices.reserve(segments.size() * 6);
	for (const auto& segment : segments)
	{
		vertices.push_back(segment.outerA);
		vertices.push_back(segment.outerB);
		vertices.push_back(segment.innerA);
		vertices.push_back(segment.innerB);
		vertices.push_back(segment.outerB);
		vertices.push_back(segment.innerA);
	}
	return vertices;
}

} // namespace openblack::graphics::partial_build_cap
