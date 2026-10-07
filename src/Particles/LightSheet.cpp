/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LightSheet.h"

#include <cmath>

#include <algorithm>
#include <utility>

using namespace openblack::particles;

namespace
{
constexpr uint32_t k_Opaque = 0xFF000000u;
/// The added colour is half the light, its alpha an eighth
constexpr uint32_t k_HalfMask = 0xFEFEFEu;
constexpr uint32_t k_SpecularAlpha = 0x40000000u;
constexpr uint32_t k_ByteMax = 255u;
constexpr uint32_t k_CornersPerPoint = 3;

/// Each channel of a colour times a light of 0 to 255, over 256
uint32_t Lit(uint32_t argb, uint32_t light)
{
	const uint32_t red = ((argb & 0xFF0000u) * light) & 0xFF0000FFu;
	const uint32_t green = ((argb & 0xFF00u) * light) & 0xFF0000u;
	const uint32_t blue = ((argb & 0xFFu) * light) & 0xFF00u;
	return (red | green | blue) >> 8u;
}
} // namespace

void LightSheet::Start(std::vector<glm::vec3> points, uint32_t rgb, float height, float shiftSeconds)
{
	_points = std::move(points);
	_strengths.assign(_points.size(), 0.0f);
	_heights.assign(_points.size(), 0.0f);
	_rgb = rgb & 0xFFFFFFu;
	_height = height;
	_shiftSeconds = shiftSeconds;
	_strength = 0.0f;
	_time = 0.0f;
	_sinceShift = 0.0f;
	_slide = 0.0f;
}

void LightSheet::Update(float seconds)
{
	_time += seconds;
	const double since = static_cast<double>(seconds) + _sinceShift;
	_sinceShift = static_cast<float>(since);
	_slide = static_cast<float>(_slide - (static_cast<double>(seconds) * k_SlideSpeed));
	if (_shiftSeconds < since && !_strengths.empty())
	{
		do
		{
			_sinceShift -= _shiftSeconds;
			std::shift_right(_strengths.begin(), _strengths.end(), 1);
			_strengths.front() = _strength;
		} while (_shiftSeconds < _sinceShift);
	}
	const auto count = static_cast<int>(_points.size());
	for (int i = 0; i < count; ++i)
	{
		const double wave = std::cos(((static_cast<double>(i) * k_WaveSpan) / (count - 1)) - (_time * k_WaveSpeed));
		_heights[static_cast<size_t>(i)] = static_cast<float>(((wave * k_WaveDepth) + k_WaveMiddle) * _height);
	}
}

void LightSheet::Build(std::vector<Vertex>& vertices, std::vector<uint32_t>& triangles)
{
	vertices.clear();
	triangles.clear();
	const auto count = _points.size();
	if (count < 2)
	{
		return;
	}
	glm::vec3 sum(0.0f);
	for (const auto& point : _points)
	{
		sum += point;
	}
	const double share = 1.0 / static_cast<double>(count);
	_middle = {static_cast<float>(share * sum.x), static_cast<float>(share * sum.y), static_cast<float>(share * sum.z)};
	_slide -= static_cast<float>(static_cast<int>(_slide));

	vertices.reserve(count * k_CornersPerPoint);
	float along = 0.0f;
	for (size_t i = 0; i < count; ++i)
	{
		const auto& point = _points[i];
		auto top = ((point - _middle) * k_Spread) + _middle;
		const double rise = static_cast<double>(_heights[i]) * _strengths[i];
		top.y = static_cast<float>(rise + top.y);
		const auto bright = ((top - point) * k_BrightLine) + point;
		const auto light = std::min(static_cast<uint32_t>(static_cast<int64_t>(rise * k_LightPerHeight)), k_ByteMax);
		const uint32_t lit = Lit(_rgb, light);
		vertices.push_back({.position = point, .uv = {along, 1.0f - _slide}, .argb = k_Opaque, .specularArgb = 0});
		vertices.push_back({.position = bright,
		                    .uv = {along, k_BrightRow - _slide},
		                    .argb = lit | k_Opaque,
		                    .specularArgb = ((lit & k_HalfMask) | k_SpecularAlpha) >> 1u});
		vertices.push_back({.position = top, .uv = {along, -_slide}, .argb = k_Opaque, .specularArgb = 0});
		const auto step = _points[(i + 1) % count] - point;
		const double distance = std::sqrt((static_cast<double>(step.x) * step.x) + (static_cast<double>(step.y) * step.y) +
		                                  (static_cast<double>(step.z) * step.z));
		along = static_cast<float>((distance * k_StarsPerUnit) + along);
	}

	triangles.reserve((count - 1) * 12);
	for (uint32_t s = 0; s + 1 < count; ++s)
	{
		const uint32_t b = s * k_CornersPerPoint;
		triangles.insert(triangles.end(), {b, b + 1, b + 3, b + 1, b + 4, b + 3, b + 1, b + 2, b + 4, b + 2, b + 5, b + 4});
	}
}
