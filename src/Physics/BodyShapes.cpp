/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "BodyShapes.h"

#include <cmath>

#include <algorithm>
#include <ranges>

#include <glm/geometric.hpp>

using namespace openblack::physics;
using namespace openblack::physics::shapes;

namespace
{
constexpr float k_TreeDragFactor = 0.3f;
constexpr float k_LivingDragFactor = 2.0f;
constexpr float k_FragmentDragFactor = 2.0f;
/// A piece whose area over its width is under this share of its radius is too thin to keep
constexpr float k_ThinPieceShare = 0.1f;
/// The cosine of a quarter turn as the game stores it, which is not quite zero
constexpr float k_QuarterTurnCos = -4.371139e-8f;

float FarthestPoint(std::span<const glm::vec3> points)
{
	float farthest = 0.0f;
	for (const auto& point : points)
	{
		farthest = std::max(farthest, glm::length(point));
	}
	return farthest;
}
} // namespace

std::vector<std::size_t> shapes::BodyParts(std::span<const ModelPart> parts)
{
	const bool anyPhysics = std::ranges::any_of(parts, &ModelPart::isPhysics);
	std::vector<std::size_t> used;
	for (std::size_t i = 0; i < parts.size(); ++i)
	{
		if (anyPhysics ? parts[i].isPhysics : parts[i].nearestDetail)
		{
			used.push_back(i);
		}
	}
	return used;
}

Shape shapes::FromModel(std::span<const ModelPart> parts, float scale)
{
	Shape shape;
	std::vector<glm::vec3> positions;
	for (const auto index : BodyParts(parts))
	{
		const auto& part = parts[index];
		const auto base = static_cast<uint32_t>(positions.size());
		positions.insert(positions.end(), part.positions.begin(), part.positions.end());
		for (std::size_t i = 0; i + 2 < part.indices.size(); i += 3)
		{
			shape.faces.push_back({base + part.indices[i], base + part.indices[i + 1], base + part.indices[i + 2]});
		}
	}
	if (positions.empty())
	{
		return shape;
	}
	glm::vec3 sum(0.0f);
	for (const auto& position : positions)
	{
		sum += position;
	}
	shape.centreOfMass = sum / static_cast<float>(positions.size());
	shape.points.reserve(positions.size());
	for (const auto& position : positions)
	{
		shape.points.push_back((position - shape.centreOfMass) * scale);
	}
	shape.radius = FarthestPoint(shape.points);
	return shape;
}

Shape shapes::Tree(float height, float radius, float scale, bool rooted)
{
	const float half = (rooted ? 0.6f : 0.5f) * height;
	Shape shape;
	shape.centreOfMass = glm::vec3(0.0f, (rooted ? 0.4f : 0.5f) * height / scale, 0.0f);
	for (const auto& [ringRadius, y] :
	     {std::pair {0.65f * radius, -0.6f * half}, std::pair {radius, 0.0f}, std::pair {0.65f * radius, 0.6f * half}})
	{
		shape.points.emplace_back(ringRadius, y, 0.0f);
		shape.points.emplace_back(0.0f, y, -ringRadius);
		shape.points.emplace_back(-ringRadius, y, 0.0f);
		shape.points.emplace_back(0.0f, y, ringRadius);
	}
	shape.points.emplace_back(0.1f * radius, -half, 0.0f);
	shape.points.emplace_back(0.0f, half, 0.0f);
	shape.points.emplace_back(0.1f * radius, -half, 0.0f);
	shape.points.emplace_back(0.1f * radius, -half, 0.0f);
	for (const uint32_t ring : {5u, 9u})
	{
		shape.faces.push_back({ring - 5, ring - 4, ring});
		shape.faces.push_back({ring - 5, ring, ring - 1});
		shape.faces.push_back({ring - 4, ring - 3, ring + 1});
		shape.faces.push_back({ring - 4, ring + 1, ring});
		shape.faces.push_back({ring - 3, ring - 2, ring + 2});
		shape.faces.push_back({ring - 3, ring + 2, ring + 1});
		shape.faces.push_back({ring - 2, ring - 5, ring - 1});
		shape.faces.push_back({ring - 2, ring - 1, ring + 2});
	}
	for (const std::array<uint32_t, 3> cap : {std::array<uint32_t, 3> {12, 1, 0},
	                                          {12, 2, 1},
	                                          {12, 3, 2},
	                                          {12, 0, 3},
	                                          {13, 8, 9},
	                                          {13, 9, 10},
	                                          {13, 10, 11},
	                                          {13, 11, 8}})
	{
		shape.faces.push_back(cap);
	}
	shape.radius = std::max(half, radius);
	shape.dragFactor = k_TreeDragFactor;
	return shape;
}

Shape shapes::LivingBody(Living kind, float height, float radius, float scale)
{
	const bool animal = kind == Living::Animal;
	const float y = 0.5f * height;
	// Across and along at the head and feet, then at the waist
	const float endX = (animal ? 0.25f : 0.8f) * radius;
	const float endZ = (animal ? 0.8f : 0.32f) * radius;
	const float waistX = (animal ? 0.3f : 0.9f) * radius;
	const float waistZ = (animal ? 0.9f : 0.32f) * radius;
	const std::array<glm::vec3, 12> box {{
	    {endX, y, endZ},
	    {-endX, y, -endZ},
	    {-endX, y, endZ},
	    {endX, y, -endZ},
	    {waistX, 0.0f, waistZ},
	    {-waistX, 0.0f, -waistZ},
	    {-waistX, 0.0f, waistZ},
	    {waistX, 0.0f, -waistZ},
	    {endX, -y, endZ},
	    {-endX, -y, -endZ},
	    {-endX, -y, endZ},
	    {endX, -y, -endZ},
	}};
	Shape shape;
	shape.centreOfMass = glm::vec3(0.0f, y / scale, 0.0f);
	for (const auto& point : box)
	{
		// A quarter turn about the up axis
		shape.points.emplace_back(point.x * k_QuarterTurnCos - point.z, point.y, point.x + point.z * k_QuarterTurnCos);
	}
	shape.faces = {{1, 2, 0},  {0, 3, 1},  {5, 1, 3},  {7, 5, 3},  {7, 3, 0},   {4, 7, 0},  {6, 4, 0},
	               {2, 6, 0},  {6, 2, 1},  {5, 6, 1},  {9, 5, 7},  {11, 9, 7},  {11, 7, 4}, {8, 11, 4},
	               {10, 8, 4}, {6, 10, 4}, {10, 6, 5}, {9, 10, 5}, {11, 10, 9}, {8, 10, 11}};
	shape.radius = FarthestPoint(shape.points);
	shape.dragFactor = k_LivingDragFactor;
	return shape;
}

FragmentShape shapes::Fragment(std::span<const std::array<glm::vec3, 3>> triangles)
{
	FragmentShape result;
	std::vector<glm::vec3> corners;
	std::vector<glm::vec3> normals;
	for (const auto& triangle : triangles)
	{
		auto normal = glm::cross(triangle[1] - triangle[0], triangle[2] - triangle[0]);
		const float twiceArea = glm::length(normal);
		if (twiceArea != 0.0f)
		{
			normal /= twiceArea;
		}
		result.area += twiceArea * 0.5f;
		for (const auto& corner : triangle)
		{
			// Each corner once, with the normal of the first triangle it is found in
			if (std::ranges::find(corners, corner) == corners.end())
			{
				corners.push_back(corner);
				normals.push_back(normal);
			}
		}
	}
	result.mass = std::max(result.area * k_FragmentMassPerArea, k_MinMass);
	auto& shape = result.shape;
	for (std::size_t i = 0; i < corners.size(); ++i)
	{
		shape.points.push_back(corners[i]);
		shape.points.push_back(corners[i] - normals[i] * k_FragmentThickness);
	}
	shape.radius = FarthestPoint(shape.points);
	shape.dragFactor = k_FragmentDragFactor;
	const float width = shape.radius + shape.radius;
	result.tooThin = result.area / width < width * k_ThinPieceShare;
	return result;
}

Shape shapes::Creature(glm::vec3 sphereCentre, float sphereRadius, std::span<const glm::vec3> partOrigins)
{
	Shape shape;
	for (const auto& origin : partOrigins)
	{
		shape.points.push_back(origin - sphereCentre);
	}
	shape.radius = sphereRadius;
	return shape;
}
