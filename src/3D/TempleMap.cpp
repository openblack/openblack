/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TempleMap.h"

#include <cmath>

#include <LNDFile.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

using namespace openblack;

namespace
{
/// The cells between the map's vertices
constexpr int32_t k_CellsPerVertex = 8;
/// LH3D's height of a unit of altitude
constexpr float k_HeightUnit = 0.67f;
/// 0x99EE50: the alpha of the map at the altitudes under 4, the coast fading into the pool
constexpr std::array<uint8_t, 4> k_CoastAlpha = {0x00, 0x55, 0xAA, 0xFF};

uint32_t Pack(uint8_t grey, uint8_t alpha)
{
	return static_cast<uint32_t>(grey) | (static_cast<uint32_t>(grey) << 8) | (static_cast<uint32_t>(grey) << 16) |
	       (static_cast<uint32_t>(alpha) << 24);
}
} // namespace

bool TempleMap::Frame(const FindCell& findCell)
{
	// The bounds of the blocks there are, in blocks
	glm::ivec2 min {k_Blocks, k_Blocks};
	glm::ivec2 max {0, 0};
	bool any = false;
	for (int32_t x = 0; x < k_Blocks; ++x)
	{
		for (int32_t z = 0; z < k_Blocks; ++z)
		{
			if (findCell(glm::u16vec2(x * 16, z * 16)) != nullptr)
			{
				min = glm::min(min, glm::ivec2(x, z));
				max = glm::max(max, glm::ivec2(x, z));
				any = true;
			}
		}
	}
	if (!any)
	{
		_scale = 0.0f;
		return false;
	}
	_centre = min + max + 1;
	const auto extent = glm::vec2(max - min);
	_scale = k_Diagonal / std::sqrt(glm::dot(extent, extent));
	return true;
}

void TempleMap::Build(const FindCell& findCell, std::vector<OrientedTextVertex>& triangles)
{
	triangles.clear();
	if (_scale <= 0.0f)
	{
		return;
	}
	// Each vertex is grey by the brightness of its cell, and faded by its altitude at the coast. A vertex with no block
	// under it is clear, at sea level.
	std::array<OrientedTextVertex, static_cast<size_t>(k_Vertices * k_Vertices)> vertices {};
	const float heightScale = k_HeightUnit * _scale / k_WorldPerVertex;
	for (int32_t z = 0; z < k_Vertices; ++z)
	{
		for (int32_t x = 0; x < k_Vertices; ++x)
		{
			const auto index = static_cast<size_t>(x + (z * k_Vertices));
			auto& vertex = vertices.at(index);
			const glm::ivec2 cell(x * k_CellsPerVertex, z * k_CellsPerVertex);
			const auto* land = x / 2 < k_Blocks && z / 2 < k_Blocks ? findCell(glm::u16vec2(cell)) : nullptr;
			const uint8_t altitude = land != nullptr ? land->altitude : 0;
			_altitudes.at(index) = altitude;
			vertex.position = glm::vec3(static_cast<float>(x - _centre.x) * _scale, static_cast<float>(altitude) * heightScale,
			                            static_cast<float>(z - _centre.y) * _scale);
			vertex.uv = glm::vec2(x, z) / static_cast<float>(k_Vertices - 1);
			if (land != nullptr)
			{
				const auto grey = static_cast<uint8_t>((255 * land->luminosity) >> 8);
				vertex.colour = Pack(grey, altitude < k_CoastAlpha.size() ? k_CoastAlpha.at(altitude) : 0xFF);
			}
		}
	}

	// Two triangles a quad, where there is a block
	for (int32_t z = 0; z < k_Vertices - 1; ++z)
	{
		for (int32_t x = 0; x < k_Vertices - 1; ++x)
		{
			if (findCell(glm::u16vec2(glm::ivec2(x, z) * k_CellsPerVertex)) == nullptr)
			{
				continue;
			}
			const auto at = [&vertices](int32_t vx, int32_t vz) {
				return vertices.at(static_cast<size_t>(vx + (vz * k_Vertices)));
			};
			triangles.insert(triangles.end(), {at(x + 1, z + 1), at(x, z), at(x, z + 1)});
			triangles.insert(triangles.end(), {at(x, z), at(x + 1, z + 1), at(x + 1, z)});
		}
	}
}

glm::vec3 TempleMap::ToMap(glm::vec2 world) const
{
	// Between the altitudes of the four vertices about the point, those off the map at sea level
	const auto u = world / k_WorldPerVertex;
	const auto vertex = glm::ivec2(u);
	const auto altitudeAt = [this](int32_t x, int32_t z) -> float {
		if (x < 0 || z < 0 || x >= k_Vertices || z >= k_Vertices)
		{
			return 0.0f;
		}
		return static_cast<float>(_altitudes.at(static_cast<size_t>(x + (z * k_Vertices))));
	};
	const auto fraction = u - glm::vec2(vertex);
	const float near =
	    altitudeAt(vertex.x, vertex.y) + ((altitudeAt(vertex.x + 1, vertex.y) - altitudeAt(vertex.x, vertex.y)) * fraction.x);
	const float far = altitudeAt(vertex.x, vertex.y + 1) +
	                  ((altitudeAt(vertex.x + 1, vertex.y + 1) - altitudeAt(vertex.x, vertex.y + 1)) * fraction.x);
	const float altitude = near + ((far - near) * fraction.y);
	return {(u.x - static_cast<float>(_centre.x)) * _scale, altitude * k_HeightUnit * _scale / k_WorldPerVertex,
	        (u.y - static_cast<float>(_centre.y)) * _scale};
}

glm::u8vec3 TempleMap::MarkerColour(std::optional<PlayerNames> player)
{
	// The players' colours (0xBFF0B8), the neutral one black
	constexpr std::array<glm::u8vec3, 8> k_PlayerColours = {
	    glm::u8vec3 {0xFF, 0x46, 0x46}, glm::u8vec3 {0x47, 0xFF, 0x54}, glm::u8vec3 {0xE3, 0x47, 0xFF},
	    glm::u8vec3 {0x47, 0xF9, 0xFF}, glm::u8vec3 {0xFF, 0xFD, 0x47}, glm::u8vec3 {0x47, 0x77, 0xFF},
	    glm::u8vec3 {0xFF, 0xA2, 0x47}, glm::u8vec3 {0x00, 0x00, 0x00},
	};
	// TODO(raffclar): GetRemapedPlayer gives some lands' second and third players others' colours, by the land's number
	auto colour = player.has_value() && static_cast<size_t>(*player) < k_PlayerColours.size()
	                  ? k_PlayerColours.at(static_cast<size_t>(*player))
	                  : glm::u8vec3(0xFF);
	if (colour == glm::u8vec3(0))
	{
		colour = glm::u8vec3(0xFF);
	}
	return colour + ((glm::u8vec3(0xFF) - colour) / glm::u8vec3(4));
}

glm::vec3 TempleMap::MarkerPosition(glm::vec2 world) const
{
	// The markers' callers give CalcPoint the cell of the thing's MapCoords, times 10
	constexpr float k_CellSize = 10.0f;
	return ToMap(glm::floor(world / k_CellSize) * k_CellSize);
}

glm::vec2 TempleMap::ToWorld(glm::vec3 map) const
{
	if (_scale <= 0.0f)
	{
		return glm::vec2(0.0f);
	}
	return ((glm::vec2(_centre) * _scale + glm::vec2(map.x, map.z)) / _scale) * k_WorldPerVertex;
}
