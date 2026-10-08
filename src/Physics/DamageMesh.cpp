/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "DamageMesh.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack::physics;
using namespace openblack::physics::damage;

namespace
{
float DistanceSquared(glm::vec3 a, glm::vec3 b)
{
	const auto d = a - b;
	return glm::dot(d, d);
}

/// Two corners the same, within the tolerance on every axis
bool Same(glm::vec3 a, glm::vec3 b)
{
	const auto d = glm::abs(a - b);
	return d.x < k_SameCorner && d.y < k_SameCorner && d.z < k_SameCorner;
}

/// How many of the nine pairs of corners of two triangles are the same corner
int SharedCorners(const Triangle& a, const Triangle& b)
{
	int shared = 0;
	for (const auto& corner : a.corners)
	{
		for (const auto& other : b.corners)
		{
			shared += Same(corner.position, other.position) ? 1 : 0;
		}
	}
	return shared;
}

Corner Middle(const Corner& a, const Corner& b)
{
	return {.position = (a.position + b.position) * 0.5f, .uv = (a.uv + b.uv) * 0.5f};
}

void SortInto(Sorted& sorted, const Triangle& triangle, glm::vec3 point, glm::vec3 direction, float reach)
{
	auto near = CornersNear(triangle, point, direction, reach);
	// The smallest triangles aren't halved again: they go with most of their corners
	if (triangle.sizeClass == 0)
	{
		if (near == 1)
		{
			near = 0;
		}
		else if (near == 2)
		{
			near = 3;
		}
	}
	switch (near)
	{
	case 0:
		if (sorted.kept.size() < k_MostListed)
		{
			sorted.kept.push_back(triangle);
		}
		break;
	case 3:
		if (sorted.broken.size() < k_MostListed)
		{
			sorted.broken.push_back(triangle);
		}
		break;
	default:
		for (const auto& half : Halve(triangle))
		{
			SortInto(sorted, half, point, direction, reach);
		}
		break;
	}
}
} // namespace

size_t Mesh::TriangleCount() const
{
	size_t count = 0;
	for (const auto& primitive : primitives)
	{
		count += primitive.triangles.size();
	}
	return count;
}

int8_t damage::SizeClassOf(const Triangle& triangle)
{
	const auto& c = triangle.corners;
	const float shortest =
	    std::min({DistanceSquared(c[0].position, c[1].position), DistanceSquared(c[1].position, c[2].position),
	              DistanceSquared(c[2].position, c[0].position)});
	for (size_t i = 0; i < k_SizeClassEdges.size(); ++i)
	{
		if (shortest > k_SizeClassEdges.at(i))
		{
			return static_cast<int8_t>(k_SizeClassEdges.size() - i);
		}
	}
	return 0;
}

Triangle damage::MakeTriangle(const std::array<Corner, 3>& corners)
{
	Triangle triangle {.corners = corners};
	triangle.sizeClass = SizeClassOf(triangle);
	triangle.countedClass = triangle.sizeClass;
	return triangle;
}

void damage::LinkNeighbours(Primitive& primitive)
{
	auto& triangles = primitive.triangles;
	for (size_t i = 0; i < triangles.size(); ++i)
	{
		auto& triangle = triangles[i];
		for (size_t side = 0; side < 3; ++side)
		{
			const auto from = triangle.corners.at(side).position;
			const auto to = triangle.corners.at((side + 1) % 3).position;
			triangle.neighbours.at(side) = -1;
			for (size_t j = 0; j < triangles.size(); ++j)
			{
				if (j == i)
				{
					continue;
				}
				// Both ends of the side are corners of the other triangle
				int matches = 0;
				for (const auto& corner : triangles[j].corners)
				{
					matches += (Same(corner.position, from) ? 1 : 0) + (Same(corner.position, to) ? 1 : 0);
				}
				if (matches == 2)
				{
					triangle.neighbours.at(side) = static_cast<int32_t>(j);
					break;
				}
			}
		}
	}
}

int damage::CornersNear(const Triangle& triangle, glm::vec3 point, glm::vec3 direction, float reach)
{
	if (direction != glm::vec3(0.0f))
	{
		direction /= std::sqrt(glm::dot(direction, direction));
	}
	int near = 0;
	for (const auto& corner : triangle.corners)
	{
		// From the corner to the nearest point of the line
		const float along = glm::dot(corner.position - point, direction);
		const auto offset = (direction * along + point) - corner.position;
		if (std::sqrt(glm::dot(offset, offset)) < reach)
		{
			++near;
		}
	}
	return near;
}

std::array<Triangle, 2> damage::Halve(const Triangle& triangle)
{
	const auto& c = triangle.corners;
	// The longest side, the side from the last corner to the first winning a tie, then the first side, then the second
	size_t start = 2;
	float longest = DistanceSquared(c[2].position, c[0].position);
	for (const size_t side : {size_t {0}, size_t {1}})
	{
		const float length = DistanceSquared(c.at(side).position, c.at(side + 1).position);
		if (length > longest)
		{
			longest = length;
			start = side;
		}
	}
	const auto& a = c.at(start);
	const auto& b = c.at((start + 1) % 3);
	const auto& opposite = c.at((start + 2) % 3);
	const auto middle = Middle(a, b);
	std::array<Triangle, 2> halves {Triangle {.corners = {a, middle, opposite}}, Triangle {.corners = {middle, b, opposite}}};
	for (auto& half : halves)
	{
		half.sizeClass = static_cast<int8_t>(triangle.sizeClass - 1);
		half.countedClass = static_cast<int8_t>(triangle.countedClass - 1);
	}
	return halves;
}

Sorted damage::SortByBlow(const std::vector<Triangle>& triangles, glm::vec3 point, glm::vec3 direction, float reach)
{
	Sorted sorted;
	for (const auto& triangle : triangles)
	{
		SortInto(sorted, triangle, point, direction, reach);
	}
	return sorted;
}

std::vector<Primitive> damage::Strike(Mesh& mesh, glm::vec3 point, glm::vec3 direction, float reach)
{
	std::vector<Primitive> broken;
	for (auto& primitive : mesh.primitives)
	{
		auto sorted = SortByBlow(primitive.triangles, point, direction, reach);
		primitive.triangles = std::move(sorted.kept);
		LinkNeighbours(primitive);
		if (!sorted.broken.empty())
		{
			Primitive piece {.material = primitive.material, .triangles = std::move(sorted.broken)};
			LinkNeighbours(piece);
			broken.push_back(std::move(piece));
		}
	}
	// A primitive left with nothing is gone from the mesh
	std::erase_if(mesh.primitives, [](const Primitive& primitive) { return primitive.triangles.empty(); });
	return broken;
}

int32_t damage::LabelGroups(Mesh& mesh)
{
	struct Ref
	{
		size_t primitive;
		size_t triangle;
	};
	std::vector<Ref> all;
	for (size_t p = 0; p < mesh.primitives.size(); ++p)
	{
		for (size_t t = 0; t < mesh.primitives[p].triangles.size(); ++t)
		{
			mesh.primitives[p].triangles[t].group = -1;
			all.push_back({p, t});
		}
	}
	const auto at = [&mesh](const Ref& ref) -> Triangle& { return mesh.primitives[ref.primitive].triangles[ref.triangle]; };
	int32_t groups = 0;
	std::vector<Ref> pending;
	for (const auto& seed : all)
	{
		if (at(seed).group >= 0)
		{
			continue;
		}
		if (groups == k_MostGroups)
		{
			break;
		}
		at(seed).group = groups;
		pending.assign(1, seed);
		while (!pending.empty())
		{
			const auto current = pending.back();
			pending.pop_back();
			for (const auto& other : all)
			{
				if (at(other).group < 0 && SharedCorners(at(current), at(other)) > 1)
				{
					at(other).group = groups;
					pending.push_back(other);
				}
			}
		}
		++groups;
	}
	return groups;
}

std::vector<bool> damage::Anchors(const Mesh& mesh, int32_t groups,
                                  const std::optional<std::function<float(glm::vec2)>>& landHeight)
{
	std::vector<bool> anchored(static_cast<size_t>(groups), false);
	if (!landHeight.has_value())
	{
		if (groups > 0)
		{
			anchored.front() = true;
		}
		return anchored;
	}
	for (const auto& primitive : mesh.primitives)
	{
		for (const auto& triangle : primitive.triangles)
		{
			if (triangle.group < 0 || anchored.at(static_cast<size_t>(triangle.group)))
			{
				continue;
			}
			for (const auto& corner : triangle.corners)
			{
				const auto& p = corner.position;
				if (p.y < (*landHeight)(glm::vec2(p.x, p.z)) + k_AnchorHeight)
				{
					anchored.at(static_cast<size_t>(triangle.group)) = true;
					break;
				}
			}
		}
	}
	return anchored;
}

std::vector<uint32_t> damage::GroupSizes(const Mesh& mesh, int32_t groups)
{
	std::vector<uint32_t> sizes(static_cast<size_t>(groups), 0);
	for (const auto& primitive : mesh.primitives)
	{
		for (const auto& triangle : primitive.triangles)
		{
			if (triangle.group >= 0)
			{
				++sizes.at(static_cast<size_t>(triangle.group));
			}
		}
	}
	return sizes;
}

std::vector<FallingPart> damage::TakeAwayLooseGroups(Mesh& mesh, const std::vector<bool>& anchors, int32_t groups)
{
	const auto sizes = GroupSizes(mesh, groups);
	const auto holds = [&anchors](const Triangle& triangle) {
		return triangle.group >= 0 && anchors.at(static_cast<size_t>(triangle.group));
	};
	std::vector<FallingPart> parts;
	for (int32_t group = 0; group < groups; ++group)
	{
		if (anchors.at(static_cast<size_t>(group)) || sizes.at(static_cast<size_t>(group)) <= 1)
		{
			continue;
		}
		for (const auto& primitive : mesh.primitives)
		{
			FallingPart part {.primitive = {.material = primitive.material}, .group = group};
			for (const auto& triangle : primitive.triangles)
			{
				if (triangle.group == group)
				{
					auto copy = triangle;
					// What falls away no longer counts as the building
					copy.countedClass = static_cast<int8_t>(copy.sizeClass + 1);
					part.primitive.triangles.push_back(copy);
				}
			}
			if (!part.primitive.triangles.empty())
			{
				parts.push_back(std::move(part));
			}
		}
	}
	for (auto& primitive : mesh.primitives)
	{
		std::erase_if(primitive.triangles, [&holds](const Triangle& triangle) { return !holds(triangle); });
		LinkNeighbours(primitive);
	}
	return parts;
}

float damage::RemainingFraction(const Mesh& mesh)
{
	if (mesh.trianglesAtCreation == 0)
	{
		return 0.0f;
	}
	size_t counted = 0;
	for (const auto& primitive : mesh.primitives)
	{
		counted += static_cast<size_t>(std::ranges::count_if(primitive.triangles, &Triangle::CountsAsBuilding));
	}
	// Halving a triangle makes two that both count, so a blow that only halves can leave more than there was
	return std::clamp(static_cast<float>(counted) / static_cast<float>(mesh.trianglesAtCreation), 0.0f, 1.0f);
}

glm::vec3 damage::CentreOf(const Primitive& primitive)
{
	glm::vec3 sum(0.0f);
	for (const auto& triangle : primitive.triangles)
	{
		for (const auto& corner : triangle.corners)
		{
			sum += corner.position;
		}
	}
	const auto count = primitive.triangles.size() * 3;
	return count == 0 ? sum : sum * (1.0f / static_cast<float>(count));
}

void damage::Offset(Primitive& primitive, glm::vec3 offset)
{
	for (auto& triangle : primitive.triangles)
	{
		for (auto& corner : triangle.corners)
		{
			corner.position += offset;
		}
	}
}

Blow damage::JudgeBlow(float momentum)
{
	if (momentum > k_BreakingMomentum)
	{
		return Blow::Breaks;
	}
	if (momentum > k_HardKnockMomentum)
	{
		return Blow::HardKnock;
	}
	return momentum > k_KnockMomentum ? Blow::Knock : Blow::Nothing;
}

bool damage::RebuildsOnBlow(float drawShare)
{
	return drawShare >= k_RebuildDrawShare && drawShare != 1.0f;
}

float damage::RepairStartLife(float life)
{
	return life * 1.1f - 0.1f;
}

float damage::BreakageShare(float life, float remaining)
{
	return std::max(life - remaining, 0.0f);
}

float damage::DrawShare(float life, float built, std::optional<float> repairStart)
{
	if (built < 1.0f)
	{
		return std::min(1.0f, built);
	}
	constexpr float k_RepairedWithoutRepair = 0.98f;
	float repaired = life * k_RepairedWithoutRepair;
	if (repairStart.has_value())
	{
		// Nothing regained since, or a repair that started whole, counts as none
		const float regained = life - *repairStart;
		const float toRegain = 1.0f - *repairStart;
		repaired = (toRegain == 0.0f || regained == 0.0f) ? 0.0f : regained / toRegain;
	}
	return std::min(repaired, built);
}

damage::PartialBuild damage::PartialBuildOf(float share, float footHeight, float halfHeight, float scale)
{
	const float pct = std::clamp(share, 0.0f, 1.0f);
	const float height = 2.0f * halfHeight * scale;
	PartialBuild build;
	const float cut = footHeight + height * pct;
	if (cut - footHeight >= k_LeastPartialCut)
	{
		build.modelCut = cut;
	}
	// The scaffold rises out of the land over the first fifth, stands whole, and is taken down from its top over the last
	if (pct < k_ScaffoldRisen)
	{
		build.scaffoldSink = height * (1.0f - pct / k_ScaffoldRisen);
	}
	else if (pct > k_ScaffoldCutFrom)
	{
		const float scaffoldCut = footHeight + (1.0f - (pct - k_ScaffoldCutFrom) / (1.0f - k_ScaffoldCutFrom)) * height;
		build.scaffoldShown = scaffoldCut - footHeight >= k_LeastPartialCut;
		build.scaffoldCut = scaffoldCut;
	}
	return build;
}

std::optional<glm::vec3> damage::RandomSurfacePoint(const Mesh& mesh, const std::function<float(float)>& random)
{
	const auto primitives = static_cast<int32_t>(mesh.primitives.size());
	const auto drawnPrimitive = static_cast<int32_t>(random(static_cast<float>(primitives)));
	const int32_t primitive = std::max(drawnPrimitive, primitives - 1);
	const auto* triangles =
	    primitive >= 0 && primitive < primitives ? &mesh.primitives.at(static_cast<size_t>(primitive)).triangles : nullptr;
	const auto count = triangles != nullptr ? static_cast<int32_t>(triangles->size()) : 0;
	const int32_t triangle = std::min(static_cast<int32_t>(random(static_cast<float>(count))), count - 1);
	float a = random(1.0f);
	float b = random(1.0f);
	// The game reads past the mesh when it has no primitive, or past the primitive when it has no triangle; nothing is
	// struck here instead
	if (triangle < 0)
	{
		return std::nullopt;
	}
	if (a + b > 1.0f)
	{
		a = 1.0f - a;
		b = 1.0f - b;
	}
	const auto& corners = triangles->at(static_cast<size_t>(triangle)).corners;
	return corners[0].position + ((corners[1].position - corners[0].position) * a) +
	       ((corners[2].position - corners[0].position) * b);
}
