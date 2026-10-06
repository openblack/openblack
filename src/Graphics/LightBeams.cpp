/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LightBeams.h"

#include <cmath>

#include <array>
#include <utility>

#include <L3DFile.h>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>

#include "3D/Light.h"

using namespace openblack::graphics;

namespace
{
constexpr uint32_t k_ConeSides = 8;

glm::u8vec4 ToBytes(const glm::vec4& colour)
{
	return glm::u8vec4(glm::round(glm::clamp(colour, 0.0f, 1.0f) * 255.0f));
}
} // namespace

void openblack::graphics::AppendCone(const LightCone& cone, float phase, BeamMesh& mesh)
{
	const auto first = static_cast<uint16_t>(mesh.vertices.size());
	const glm::vec3 axisX = cone.transform[0];
	const glm::vec3 axisY = cone.transform[1];
	const glm::vec3 axisZ = cone.transform[2];
	const glm::vec3 origin = cone.transform[3];
	// The angle is the cone's whole width
	const float farRadius = std::tan(glm::radians(cone.angle) * 0.5f) * cone.length;
	const auto nearColour = ToBytes(cone.colour);
	const auto farColour = glm::u8vec4(0, 0, 0, 255);

	for (uint32_t side = 0; side < k_ConeSides; ++side)
	{
		const float angle = static_cast<float>(side) * glm::quarter_pi<float>();
		const float sine = std::sin(angle);
		const float cosine = std::cos(angle);
		// Each side's coordinates in the atmosphere texture wander about their own frame on their own beat
		const float beat = phase + angle;
		mesh.vertices.push_back({
		    .position = origin + axisX * (sine * farRadius) + axisY * (cosine * farRadius) - axisZ * cone.length,
		    .colour = farColour,
		    .uv = {std::sin(beat) * 0.1f + 0.625f, std::sin(23.0f - beat) * 0.1f + 0.375f},
		});
		mesh.vertices.push_back({
		    .position = origin + axisX * (sine * cone.nearRadius) + axisY * (cosine * cone.nearRadius),
		    .colour = nearColour,
		    .uv = {std::sin(52.0f - beat * 1.01f) * 0.1f + 0.625f, std::sin(beat * 0.99f + 63.0f) * 0.1f + 0.375f},
		});
	}
	for (uint32_t side = 0; side < k_ConeSides; ++side)
	{
		const auto far = static_cast<uint16_t>(first + side * 2);
		const auto nextFar = static_cast<uint16_t>(first + ((side + 1) % k_ConeSides) * 2);
		mesh.indices.insert(mesh.indices.end(), {far, static_cast<uint16_t>(far + 1), static_cast<uint16_t>(nextFar + 1),
		                                         static_cast<uint16_t>(nextFar + 1), nextFar, far});
	}
}

BeamMesh openblack::graphics::MakeVolumeLight(std::span<const l3d::L3DVertex> vertices, std::span<const uint16_t> indices,
                                              glm::vec3 source, float length)
{
	// The edges of the window's triangles, each once. The game counts them in a table of 0x4000 by a hash of their
	// ends, and so leaves out an edge whose hash another has taken.
	std::array<uint8_t, 0x4000> seen {};
	std::vector<std::pair<uint16_t, uint16_t>> edges;
	const auto addEdge = [&seen, &edges](uint16_t a, uint16_t b) {
		if (a == b)
		{
			return;
		}
		const auto low = std::min(a, b);
		const auto high = std::max(a, b);
		const uint32_t span = high - low;
		const uint32_t hash = (((span << 9) | (span >> 5)) ^ low) & 0x3fff;
		if (seen[hash]++ == 0)
		{
			edges.emplace_back(low, high);
		}
	};
	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		addEdge(indices[i], indices[i + 1]);
		addEdge(indices[i], indices[i + 2]);
		addEdge(indices[i + 2], indices[i + 1]);
	}

	// Lit by the window at its end, a grey of 0x40 at half alpha, and nothing at the far one
	const auto nearColour = glm::u8vec4(0x40, 0x40, 0x40, 0x7f);
	const auto farColour = glm::u8vec4(0);
	const float reach = length * k_VolumeLightLengthScale;
	BeamMesh mesh;
	mesh.vertices.reserve(edges.size() * 4);
	mesh.indices.reserve(edges.size() * 6);
	for (const auto& [a, b] : edges)
	{
		if (a >= vertices.size() || b >= vertices.size())
		{
			continue;
		}
		const auto first = static_cast<uint16_t>(mesh.vertices.size());
		for (const auto index : {a, b})
		{
			const auto& vertex = vertices[index];
			const glm::vec3 position {vertex.position.x, vertex.position.y, vertex.position.z};
			const glm::vec2 uv {vertex.texCoord.x, vertex.texCoord.y};
			const auto away = position - source;
			const auto far = position + away * (reach / glm::length(away));
			mesh.vertices.push_back({.position = position, .colour = nearColour, .uv = uv});
			mesh.vertices.push_back({.position = far, .colour = farColour, .uv = uv});
		}
		mesh.indices.insert(mesh.indices.end(), {first, static_cast<uint16_t>(first + 1), static_cast<uint16_t>(first + 3),
		                                         static_cast<uint16_t>(first + 3), static_cast<uint16_t>(first + 2), first});
	}
	return mesh;
}
