/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SeaRows.h"

#include <cstdint>

#include <algorithm>
#include <array>
#include <vector>

#include <glm/vec4.hpp>

using namespace openblack::graphics;

namespace
{
/// Which planes of the view a point is outside of. The view is a 90 degree frustum in clip space, |x| and |y| within
/// the view depth, with a near plane and no far one.
enum ClipPlane : uint32_t
{
	k_Bottom = 0x02,
	k_Top = 0x04,
	k_Left = 0x08,
	k_Right = 0x10,
	k_Near = 0x20,
};

/// A point being clipped: its clip x and y and view depth, and the planes it is outside of
struct ClipVertex
{
	float x;
	float y;
	float z;
	uint32_t outside;
};

uint32_t Outside(float x, float y, float z, float nearDistance)
{
	uint32_t outside = z < nearDistance ? static_cast<uint32_t>(k_Near) : 0u;
	if (x > z)
	{
		outside |= k_Right;
	}
	else if (-z > x)
	{
		outside |= k_Left;
	}
	if (y > z)
	{
		outside |= k_Top;
	}
	else if (-z > y)
	{
		outside |= k_Bottom;
	}
	return outside;
}

/// Clips triangles against the view's planes, one at a time from the near plane down, as the game does: each cut adds
/// two points and leaves one smaller triangle or two, the first of which is clipped on by itself. The order of the
/// triangles and points it leaves decides where the rows start, so it is kept.
class Clipper
{
public:
	Clipper(std::vector<ClipVertex>& vertices, std::vector<uint16_t>& triangles, float nearDistance)
	    : _vertices(vertices)
	    , _triangles(triangles)
	    , _near(nearDistance)
	{
	}

	// NOLINTNEXTLINE(misc-no-recursion): the clipping of one triangle recurses into one of its two halves
	void Clip(uint16_t a, uint16_t b, uint16_t c, uint32_t plane)
	{
		for (;;)
		{
			if (plane == 0)
			{
				Emit(a, b, c);
				return;
			}
			const uint32_t outA = _vertices[a].outside & plane;
			const uint32_t outB = _vertices[b].outside & plane;
			const uint32_t outC = _vertices[c].outside & plane;
			if (outA != 0)
			{
				if (outB != 0)
				{
					if (outC != 0)
					{
						return;
					}
					const auto n = Cut(plane, c, a);
					Cut(plane, c, b);
					a = c;
					b = n;
					c = static_cast<uint16_t>(n + 1);
				}
				else if (outC != 0)
				{
					const auto n = Cut(plane, b, a);
					Cut(plane, b, c);
					a = n;
					c = static_cast<uint16_t>(n + 1);
				}
				else
				{
					const auto n = Cut(plane, b, a);
					Cut(plane, c, a);
					plane >>= 1;
					Clip(n, b, c, plane);
					a = n;
					b = c;
					c = static_cast<uint16_t>(n + 1);
					continue;
				}
			}
			else if (outB != 0)
			{
				if (outC != 0)
				{
					const auto n = Cut(plane, a, b);
					Cut(plane, a, c);
					b = n;
					c = static_cast<uint16_t>(n + 1);
				}
				else
				{
					const auto n = Cut(plane, a, b);
					Cut(plane, c, b);
					plane >>= 1;
					Clip(a, n, c, plane);
					a = n;
					b = static_cast<uint16_t>(n + 1);
					continue;
				}
			}
			else if (outC != 0)
			{
				const auto n = Cut(plane, a, c);
				Cut(plane, b, c);
				plane >>= 1;
				Clip(n, a, b, plane);
				a = n;
				c = static_cast<uint16_t>(n + 1);
				continue;
			}
			else if ((_vertices[a].outside | _vertices[b].outside | _vertices[c].outside) == 0)
			{
				Emit(a, b, c);
				return;
			}
			plane >>= 1;
		}
	}

private:
	void Emit(uint16_t a, uint16_t b, uint16_t c)
	{
		_triangles.push_back(a);
		_triangles.push_back(b);
		_triangles.push_back(c);
	}

	/// Adds the point where the edge from an inside point to an outside one meets the plane. Only the planes after the
	/// one cut are tested for the new point.
	uint16_t Cut(uint32_t plane, uint16_t inside, uint16_t outside)
	{
		const auto distance = [this, plane](const ClipVertex& v) {
			switch (plane)
			{
			case k_Near:
				return v.z - _near;
			case k_Right:
				return v.z - v.x;
			case k_Left:
				return v.z + v.x;
			case k_Top:
				return v.z - v.y;
			default:
				return v.z + v.y;
			}
		};
		const auto in = _vertices[inside];
		const auto out = _vertices[outside];
		const float dIn = distance(in);
		const float t = dIn / (dIn - distance(out));
		ClipVertex v {
		    .x = (in.x * (1.0f - t)) + (out.x * t),
		    .y = (in.y * (1.0f - t)) + (out.y * t),
		    .z = (in.z * (1.0f - t)) + (out.z * t),
		    .outside = 0,
		};
		v.outside = Outside(v.x, v.y, v.z, _near) & (plane - 1);
		_vertices.push_back(v);
		return static_cast<uint16_t>(_vertices.size() - 1);
	}

	std::vector<ClipVertex>& _vertices;
	std::vector<uint16_t>& _triangles;
	float _near;
};
} // namespace

std::optional<sea_rows::ScreenRange> sea_rows::ComputeScreenRange(const glm::mat4& viewProjection, glm::vec2 viewportSize,
                                                                  float nearDistance)
{
	const std::array<glm::vec2, 4> corners = {{
	    {k_SquareMinimum, k_SquareMinimum},
	    {k_SquareMaximum, k_SquareMinimum},
	    {k_SquareMaximum, k_SquareMaximum},
	    {k_SquareMinimum, k_SquareMaximum},
	}};
	std::vector<ClipVertex> vertices;
	vertices.reserve(32);
	for (const auto& corner : corners)
	{
		const auto p = viewProjection * glm::vec4(corner.x, 0.0f, corner.y, 1.0f);
		vertices.push_back({.x = p.x, .y = p.y, .z = p.w, .outside = Outside(p.x, p.y, p.w, nearDistance)});
	}
	// The square is two triangles, (0, 2, 1) and (0, 3, 2). The game only looks at the first three corners to decide
	// whether to clip at all: with those on the screen both triangles are taken whole, the last corner as it is.
	std::vector<uint16_t> triangles;
	triangles.reserve(64);
	const uint32_t firstThreeOutside = vertices[0].outside | vertices[1].outside | vertices[2].outside;
	if (firstThreeOutside == 0)
	{
		triangles = {0, 2, 1, 0, 3, 2};
	}
	else
	{
		if ((vertices[0].outside & vertices[1].outside & vertices[2].outside & vertices[3].outside) != 0)
		{
			// The whole square is beyond one plane
			return std::nullopt;
		}
		Clipper clipper(vertices, triangles, nearDistance);
		clipper.Clip(0, 2, 1, k_Near);
		if ((vertices[3].outside | vertices[0].outside | vertices[2].outside) != 0)
		{
			clipper.Clip(0, 3, 2, k_Near);
		}
		else
		{
			triangles.insert(triangles.end(), {0, 3, 2});
		}
	}
	if (triangles.empty())
	{
		return std::nullopt;
	}

	// The game goes through the clipped points in order: a point below the lowest so far becomes the bottom, *else* one
	// above the highest so far becomes the top. The first point can therefore only ever be the bottom.
	ScreenRange range {.top = viewportSize.y, .bottom = -1.0f, .inverseDepthTop = 0.0f, .inverseDepthBottom = 0.0f};
	const float lowestPixel = viewportSize.y - 1.0f;
	for (const auto index : triangles)
	{
		const auto& v = vertices[index];
		float y = (1.0f - (v.y / v.z)) * 0.5f * viewportSize.y;
		float inverseDepth = 1.0f / v.z;
		if (index >= corners.size())
		{
			// The points the clipping added are kept on the screen
			y = std::clamp(y, 0.0f, lowestPixel);
		}
		else if (v.outside != 0)
		{
			// A corner taken whole while off the screen was never projected: the game reads its clip y as a screen y,
			// and its depth over the near plane's as one over its depth
			y = v.y;
			inverseDepth = v.z / nearDistance;
		}
		if (y > range.bottom)
		{
			range.bottom = y;
			range.inverseDepthBottom = inverseDepth;
		}
		else if (y < range.top)
		{
			range.top = y;
			range.inverseDepthTop = inverseDepth;
		}
	}
	if (range.bottom < 0.0f || range.top > lowestPixel)
	{
		return std::nullopt;
	}
	range.bottom = std::min(range.bottom, lowestPixel);
	range.top = std::max(range.top, 0.0f);
	return range;
}

sea_rows::Rows sea_rows::MakeRows(const ScreenRange& range)
{
	Rows rows {};
	// Truncated towards zero
	rows.first = static_cast<int>(range.top);
	const int bottom = static_cast<int>(range.bottom);
	rows.count = (bottom - rows.first + 2) / 2;
	rows.inverseDepth = range.inverseDepthTop;
	rows.inverseStep =
	    rows.count > 1 ? (range.inverseDepthBottom - range.inverseDepthTop) / static_cast<float>(rows.count - 1) : 0.0f;
	rows.softTop = static_cast<float>(rows.first) > 0.5f;
	return rows;
}
