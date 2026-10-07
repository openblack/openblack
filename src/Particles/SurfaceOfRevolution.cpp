/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SurfaceOfRevolution.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack::particles;
using namespace openblack::particles::surface;

namespace
{
/// The game turns the profile through this much, a float of two pi
constexpr float k_TwoPi = 6.2831855f;
/// The funnels start this far below the rim
constexpr float k_FunnelDepth = 3.0f;
/// The spout is this much wider, and its curve rises this much faster
constexpr float k_SpoutWidth = 1.5f;
constexpr float k_SpoutRise = 2.0f;
constexpr float k_ByteMax = 255.0f;
/// The land's cells are this wide, and their lines are found a tenth of a metre at a time
constexpr float k_CellWidth = 10.0f;
constexpr float k_PerCell = -0.1f;

/// A share of 0..1 as a byte, cut down as the game does
uint32_t Byte(float share)
{
	return static_cast<uint32_t>(static_cast<int32_t>(share * k_ByteMax)) & 0xFFu;
}

/// The share of the way out of point i of n, the last at 1
float Share(int i, int n)
{
	return static_cast<float>(i) * (1.0f / (static_cast<float>(n) - 1.0f));
}

uint32_t Channel(Argb colour, uint32_t shift)
{
	return (colour >> shift) & 0xFFu;
}

/// A colour between two by a byte factor, each channel moved by its difference times the factor over 256
Argb LerpColour(Argb from, Argb to, int32_t factor)
{
	Argb out = 0;
	for (const uint32_t shift : {0u, 8u, 16u, 24u})
	{
		const auto a = static_cast<int32_t>(Channel(from, shift));
		const auto b = static_cast<int32_t>(Channel(to, shift));
		// The difference times the factor, shifted down, rounds towards minus infinity as the game's shift does
		const int32_t moved = a + ((b - a) * factor >> 8);
		out |= (static_cast<uint32_t>(moved) & 0xFFu) << shift;
	}
	return out;
}
} // namespace

ProfilePoint surface::Evaluate(Profile profile, float t)
{
	switch (profile)
	{
	case Profile::Funnel:
		return {.radius = t, .height = (std::sqrt(t) - 1.0f) * k_FunnelDepth};
	case Profile::FunnelSpout:
		return {.radius = k_SpoutWidth * t, .height = (std::sqrt(k_SpoutRise * t) - 1.0f) * k_FunnelDepth};
	case Profile::FunnelParabola:
		return {.radius = t, .height = (t * t - 1.0f) * k_FunnelDepth};
	case Profile::Disk:
		break;
	}
	return {.radius = t, .height = 0.0f};
}

Profile surface::ProfileOf(int functionIndex)
{
	switch (functionIndex)
	{
	case 1:
		return Profile::Funnel;
	case 2:
		return Profile::FunnelSpout;
	case 3:
		return Profile::FunnelParabola;
	default:
		return Profile::Disk;
	}
}

RingShade surface::ShadeAt(float t, float fadeIn, float fadeOut)
{
	if (t < fadeIn)
	{
		return {.brightness = t < 0.0f ? 0.0f : t / fadeIn, .alpha = 1.0f};
	}
	const float start = 1.0f - fadeOut;
	if (t <= start)
	{
		return {.brightness = 1.0f, .alpha = 1.0f};
	}
	if (t > 1.0f)
	{
		return {.brightness = 1.0f, .alpha = 0.0f};
	}
	return {.brightness = 1.0f, .alpha = 1.0f - (t - start) / (1.0f - start)};
}

Argb surface::ScaleRgb(Argb colour, uint32_t factor)
{
	return (colour & 0xFF000000u) | (((Channel(colour, 16) * factor) >> 8u) << 16u) |
	       (((Channel(colour, 8) * factor) >> 8u) << 8u) | ((Channel(colour, 0) * factor) >> 8u);
}

Argb surface::Modulate(Argb a, Argb b)
{
	Argb out = 0;
	for (const uint32_t shift : {0u, 8u, 16u, 24u})
	{
		out |= ((Channel(a, shift) * Channel(b, shift)) >> 8u) << shift;
	}
	return out;
}

uint32_t surface::LightLevel(glm::vec3 normal, glm::vec3 towardsLight, uint32_t ambient)
{
	const auto facing = static_cast<int32_t>(std::nearbyint(glm::dot(towardsLight, normal) * k_ByteMax));
	if (facing < 0)
	{
		return ambient;
	}
	return ambient + static_cast<uint32_t>((static_cast<int32_t>(255u - ambient) * facing) >> 8);
}

Mesh surface::Build(const Shape& shape, Argb specularColour)
{
	Mesh mesh;
	const int numU = std::max(shape.numU, 2);
	const int numV = std::max(shape.numV, 2);
	mesh.vertices.reserve(static_cast<size_t>(numU) * static_cast<size_t>(numV));
	for (int ring = 0; ring < numV; ++ring)
	{
		const float t = Share(ring, numV);
		const auto point = Evaluate(shape.profile, t);
		Argb colour = 0xFFFFFFFFu;
		Argb specular = 0;
		if (shape.fadeAlphas)
		{
			const auto shade = ShadeAt(t, shape.fadeIn, shape.fadeOut);
			const auto grey = Byte(shade.brightness);
			colour = (Byte(shade.alpha) << 24u) | (grey << 16u) | (grey << 8u) | grey;
			if (shape.specular)
			{
				specular = ScaleRgb(specularColour, Byte(1.0f - shade.brightness));
			}
		}
		for (int step = 0; step < numU; ++step)
		{
			const float s = Share(step, numU);
			const float angle = s * k_TwoPi;
			mesh.vertices.push_back({
			    .position = {point.radius * std::cos(angle), point.height, point.radius * std::sin(angle)},
			    .uv = glm::vec2(s, t) * shape.uvScale,
			    .colour = colour,
			    .specular = specular,
			});
		}
	}
	for (int ring = 0; ring + 1 < numV; ++ring)
	{
		const auto first = static_cast<uint32_t>(ring * numU);
		for (int step = 0; step + 1 < numU; ++step)
		{
			const auto inner = first + static_cast<uint32_t>(step);
			const auto outer = inner + static_cast<uint32_t>(numU);
			mesh.indices.insert(mesh.indices.end(), {outer, inner, outer + 1});
			mesh.indices.insert(mesh.indices.end(), {outer + 1, inner, inner + 1});
		}
	}
	return mesh;
}

void surface::ComputeNormals(Mesh& mesh)
{
	mesh.normals.assign(mesh.vertices.size(), glm::vec3(0.0f));
	std::vector<int> faces(mesh.vertices.size(), 0);
	for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
	{
		const auto a = mesh.indices[i];
		const auto b = mesh.indices[i + 1];
		const auto c = mesh.indices[i + 2];
		const auto& v0 = mesh.vertices[a].position;
		const auto& v1 = mesh.vertices[b].position;
		const auto& v2 = mesh.vertices[c].position;
		const auto cross = glm::cross(v1 - v0, v2 - v1);
		const float length = glm::length(cross);
		if (length <= 0.0f)
		{
			continue;
		}
		const auto normal = cross / length;
		for (const auto index : {a, b, c})
		{
			mesh.normals[index] += normal;
			++faces[index];
		}
	}
	for (size_t i = 0; i < mesh.normals.size(); ++i)
	{
		if (faces[i] > 1 && glm::length(mesh.normals[i]) > 0.0f)
		{
			mesh.normals[i] = glm::normalize(mesh.normals[i]);
		}
	}
}

void surface::Sway(const Mesh& still, const Shape& shape, float maxTwist, float maxSlide, float sway, Mesh& out)
{
	out.vertices = still.vertices;
	out.indices = still.indices;
	const int numU = std::max(shape.numU, 2);
	const int numV = std::max(shape.numV, 2);
	for (int ring = 0; ring < numV; ++ring)
	{
		const float t = Share(ring, numV);
		const float twist = t * maxTwist * sway;
		const float c = std::cos(twist);
		const float s = std::sin(twist);
		const float inward = 1.0f - t;
		for (int step = 0; step < numU; ++step)
		{
			const auto index = static_cast<size_t>(ring * numU + step);
			if (index >= out.vertices.size())
			{
				return;
			}
			auto& vertex = out.vertices[index];
			const auto& from = still.vertices[index];
			if (maxTwist != 0.0f)
			{
				vertex.position.x = from.position.x * c - from.position.z * s;
				vertex.position.z = from.position.x * s + from.position.z * c;
			}
			if (maxSlide != 0.0f)
			{
				vertex.uv.x = inward * inward * maxSlide * sway + from.uv.x;
			}
		}
	}
}

void surface::Slice(Mesh& mesh, const Plane& plane)
{
	const bool lit = mesh.normals.size() == mesh.vertices.size();
	const auto distance = [&](uint32_t index) { return glm::dot(plane.normal, mesh.vertices[index].position) + plane.offset; };
	const size_t faces = mesh.indices.size() / 3;
	for (size_t face = 0; face < faces; ++face)
	{
		const std::array<uint32_t, 3> corners {mesh.indices[face * 3], mesh.indices[face * 3 + 1], mesh.indices[face * 3 + 2]};
		const std::array<float, 3> distances {distance(corners[0]), distance(corners[1]), distance(corners[2])};
		const std::array<int, 3> sides {distances[0] > 0.0f ? 1 : -1, distances[1] > 0.0f ? 1 : -1,
		                                distances[2] > 0.0f ? 1 : -1};
		if (sides[0] == sides[1] && sides[1] == sides[2])
		{
			continue;
		}
		// The corner alone on its side
		const int lone = sides[0] * sides[1] * sides[2];
		const auto first = static_cast<size_t>(std::distance(sides.begin(), std::ranges::find(sides, lone)));
		const auto f = corners.at(first);
		const auto next = corners.at((first + 1) % 3);
		const auto last = corners.at((first + 2) % 3);
		const auto cut = [&](uint32_t other) {
			const float df = distances.at(first);
			const float d = distance(other);
			const float t = -df / (d - df);
			const auto factor = static_cast<int32_t>(static_cast<uint32_t>(static_cast<int32_t>(t * k_ByteMax)) & 0xFFu);
			const auto from = mesh.vertices[f];
			const auto to = mesh.vertices[other];
			mesh.vertices.push_back({
			    .position = from.position + (to.position - from.position) * t,
			    .uv = from.uv + (to.uv - from.uv) * t,
			    .colour = LerpColour(from.colour, to.colour, factor),
			    .specular = LerpColour(from.specular, to.specular, factor),
			});
			if (lit)
			{
				const auto normalFrom = mesh.normals[f];
				const auto normalTo = mesh.normals[other];
				mesh.normals.push_back(normalFrom + (normalTo - normalFrom) * t);
			}
			return static_cast<uint32_t>(mesh.vertices.size() - 1);
		};
		const auto i0 = cut(next);
		const auto i1 = cut(last);
		mesh.indices[face * 3] = f;
		mesh.indices[face * 3 + 1] = i0;
		mesh.indices[face * 3 + 2] = i1;
		mesh.indices.insert(mesh.indices.end(), {next, last, i1});
		mesh.indices.insert(mesh.indices.end(), {next, i1, i0});
	}
}

void surface::DrapeOverLand(Mesh& worldMesh, glm::vec2 middle, const std::function<float(glm::vec2)>& landHeight)
{
	if (worldMesh.vertices.empty())
	{
		return;
	}
	glm::vec3 lowest(1e7f);
	glm::vec3 highest(-1e7f);
	for (const auto& vertex : worldMesh.vertices)
	{
		lowest = glm::min(lowest, vertex.position);
		highest = glm::max(highest, vertex.position);
	}
	const auto centre = (lowest + highest) * 0.5f;
	const auto extent = (highest - lowest) * 0.5f;
	const auto cellOf = [](float coordinate) { return static_cast<int>(coordinate * k_PerCell); };
	const int x0 = -1 - cellOf(centre.x - extent.x);
	const int x1 = 1 - cellOf(centre.x + extent.x);
	const int z0 = -1 - cellOf(centre.z - extent.z);
	const int z1 = 1 - cellOf(centre.z + extent.z);
	// Along the cells' edges each way, then both ways of their diagonals
	for (int i = x0; i <= x1; ++i)
	{
		Slice(worldMesh, {.normal = {1.0f, 0.0f, 0.0f}, .offset = -static_cast<float>(i) * k_CellWidth});
	}
	for (int j = z0; j <= z1; ++j)
	{
		Slice(worldMesh, {.normal = {0.0f, 0.0f, 1.0f}, .offset = -static_cast<float>(j) * k_CellWidth});
	}
	const float diagonal = 1.0f / std::sqrt(2.0f);
	const auto through = [&](const glm::vec3& normal, int i) {
		const glm::vec3 point {static_cast<float>(i) * k_CellWidth, 0.0f, static_cast<float>(z1) * k_CellWidth};
		return Plane {.normal = normal, .offset = -glm::dot(point, normal)};
	};
	for (int i = x0 - (z1 - z0); i <= x1; ++i)
	{
		Slice(worldMesh, through({diagonal, 0.0f, diagonal}, i));
	}
	for (int i = x0; i <= x1 + (z1 - z0); ++i)
	{
		Slice(worldMesh, through({diagonal, 0.0f, -diagonal}, i));
	}
	const float base = landHeight(middle);
	for (auto& vertex : worldMesh.vertices)
	{
		vertex.position.y += landHeight({vertex.position.x, vertex.position.z}) - base;
	}
}
