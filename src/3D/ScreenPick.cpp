/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ScreenPick.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

namespace openblack::screen_pick
{
namespace
{
// The view's sides a corner is beyond, as bits: in front of the near plane, past the right, left, top and bottom edges
constexpr uint8_t k_BeyondBottom = 0x2;
constexpr uint8_t k_BeyondTop = 0x4;
constexpr uint8_t k_BeyondLeft = 0x8;
constexpr uint8_t k_BeyondRight = 0x10;
constexpr uint8_t k_BeforeNear = 0x20;
/// The sides in the order the clipper cuts by them
constexpr std::array<uint8_t, 5> k_Sides = {k_BeforeNear, k_BeyondRight, k_BeyondLeft, k_BeyondTop, k_BeyondBottom};

/// A triangle cut by the view's five sides has at most three corners and one more for each side
constexpr size_t k_MostCutCorners = 3 + k_Sides.size();
/// A polygon being cut, its corners in place
struct CutPolygon
{
	std::array<ClipCorner, k_MostCutCorners + 1> corners {};
	size_t count {0};
};

[[nodiscard]] uint8_t SidesBeyond(const View& view, const ClipCorner& corner)
{
	uint8_t sides = corner.w < view.near ? k_BeforeNear : 0;
	if (corner.x > corner.w)
	{
		sides |= k_BeyondRight;
	}
	else if (corner.x < -corner.w)
	{
		sides |= k_BeyondLeft;
	}
	if (corner.y > corner.w)
	{
		sides |= k_BeyondTop;
	}
	else if (corner.y < -corner.w)
	{
		sides |= k_BeyondBottom;
	}
	return sides;
}

/// How far inside a side of the view a corner is, below 0 beyond it
[[nodiscard]] float Inside(const View& view, const ClipCorner& corner, uint8_t side)
{
	switch (side)
	{
	case k_BeforeNear:
		return corner.w - view.near;
	case k_BeyondRight:
		return corner.w - corner.x;
	case k_BeyondLeft:
		return corner.w + corner.x;
	case k_BeyondTop:
		return corner.w - corner.y;
	default:
		return corner.w + corner.y;
	}
}

/// The corner on the screen, kept on the screen's pixels
[[nodiscard]] ScreenCorner Project(const View& view, const ClipCorner& corner)
{
	const glm::vec2 half = view.resolution * 0.5f;
	const float overDepth = 1.0f / corner.w;
	const float x = std::clamp((overDepth * corner.x + 1.0f) * half.x, 0.0f, view.resolution.x - 1.0f);
	const float y = std::clamp(half.y - corner.y * overDepth * half.y, 0.0f, view.resolution.y - 1.0f);
	return {.x = x, .y = y, .depth = view.near * overDepth, .uv = corner.uv};
}

/// Whether a triangle faces the camera on the screen
[[nodiscard]] bool FacesCamera(const ScreenCorner& a, const ScreenCorner& b, const ScreenCorner& c)
{
	return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y) > 0.0f;
}

/// Whether the cursor is in a triangle on the screen, on its edges included, whichever way round its corners go
[[nodiscard]] bool Covers(const ScreenCorner& a, const ScreenCorner& b, const ScreenCorner& c, glm::vec2 cursor)
{
	const glm::vec2 d0(a.x - cursor.x, a.y - cursor.y);
	const glm::vec2 d1(b.x - cursor.x, b.y - cursor.y);
	const glm::vec2 d2(c.x - cursor.x, c.y - cursor.y);
	const float first = d1.y * d0.x - d1.x * d0.y;
	const float second = d2.y * d1.x - d2.x * d1.y;
	const float third = d0.y * d2.x - d0.x * d2.y;
	if (first <= 0.0f)
	{
		return second <= 0.0f && third <= 0.0f;
	}
	return second >= 0.0f && third >= 0.0f;
}

/// The depth (near over the view depth) and texture coordinates of a triangle at the cursor: across the scan line
/// through the cursor, between its long edge and whichever short edge the line crosses
struct AtCursor
{
	float depth;
	glm::vec2 uv;
};
[[nodiscard]] AtCursor Interpolate(const ScreenCorner& first, const ScreenCorner& second, const ScreenCorner& third,
                                   glm::vec2 cursor)
{
	// The corners from the top of the screen down
	const ScreenCorner* low = &second;
	const ScreenCorner* middle = &first;
	const ScreenCorner* high = &third;
	if (second.y <= first.y)
	{
		if (third.y <= first.y)
		{
			low = &third;
			high = &first;
			middle = &second;
			if (second.y < third.y)
			{
				low = &second;
				middle = &third;
			}
		}
	}
	else
	{
		low = &first;
		middle = &second;
		high = &third;
		if (third.y <= second.y)
		{
			low = &third;
			high = &second;
			middle = &first;
			if (first.y < third.y)
			{
				low = &first;
				middle = &third;
			}
		}
	}

	const float alongLong = (low->y - cursor.y) / ((low->y - cursor.y) - (high->y - cursor.y));
	const float beforeLong = 1.0f - alongLong;
	const float depthLong = alongLong * high->depth + beforeLong * low->depth;
	const float xLong = alongLong * high->x + beforeLong * low->x;
	const auto uvLong = (alongLong * high->uv * high->depth + low->uv * low->depth * beforeLong) / depthLong;

	const ScreenCorner* start = middle;
	const ScreenCorner* end = high;
	if (cursor.y < middle->y)
	{
		start = low;
		end = middle;
	}
	const float alongShort = (start->y - cursor.y) / ((start->y - cursor.y) - (end->y - cursor.y));
	const float beforeShort = 1.0f - alongShort;
	const float depthShort = alongShort * end->depth + beforeShort * start->depth;
	const float xShort = beforeShort * start->x + alongShort * end->x;
	const auto uvShort = (end->uv * end->depth * alongShort + start->uv * start->depth * beforeShort) / depthShort;

	const float across = (xShort - cursor.x) / ((xShort - cursor.x) - (xLong - cursor.x));
	const float rest = 1.0f - across;
	const float depth = depthLong * across + depthShort * rest;
	const auto uv = (uvLong * depthLong * across + uvShort * depthShort * rest) / depth;
	return {.depth = depth, .uv = uv};
}

} // namespace

float Depth(const View& view, glm::vec3 point)
{
	return (view.worldToClip * glm::vec4(point, 1.0f)).w;
}

bool CursorOverSphere(const View& view, glm::vec3 centre, float radius, glm::vec3 origin)
{
	const auto clip = view.worldToClip * glm::vec4(centre, 1.0f);
	if (view.near > clip.w + radius)
	{
		return false;
	}
	// A camera inside the sphere is over it wherever the cursor is
	const auto fromCamera = origin - view.camera;
	if (glm::dot(fromCamera, fromCamera) < radius * radius)
	{
		return true;
	}
	const float overDepth = 1.0f / clip.w;
	const glm::vec2 half = view.resolution * 0.5f;
	const float x = (clip.x * overDepth + 1.0f) * half.x;
	const float y = (1.0f - clip.y * overDepth) * half.y;
	const float onScreen = radius * overDepth * half.x * view.xScale;
	if (onScreen + x < 0.0f || x - onScreen > view.resolution.x || onScreen + y < 0.0f || y - onScreen > view.resolution.y)
	{
		return false;
	}
	const float dx = view.cursor.x - x;
	const float dy = view.cursor.y - y;
	return dx * dx + dy * dy < onScreen * onScreen;
}

ClipCorner ToClip(const View& view, glm::vec3 world, glm::vec2 uv)
{
	const auto clip = view.worldToClip * glm::vec4(world, 1.0f);
	return {.x = clip.x, .y = clip.y, .w = clip.w, .uv = uv};
}

AlphaMask MaskOf(std::span<const uint16_t> texels, uint32_t width, uint32_t height)
{
	AlphaMask mask {};
	if (texels.size() < static_cast<size_t>(width) * height)
	{
		mask.fill(true);
		return mask;
	}
	const float acrossStep = static_cast<float>(width) * (1.0f / 64.0f);
	const float downStep = static_cast<float>(height) * (1.0f / 64.0f);
	for (uint32_t row = 0; row < 64; ++row)
	{
		const auto y = static_cast<uint32_t>(static_cast<float>(row) * downStep);
		for (uint32_t column = 0; column < 64; ++column)
		{
			const auto x = static_cast<uint32_t>(static_cast<float>(column) * acrossStep);
			mask[row * 64 + column] = (texels[y * width + x] & 0xF000u) != 0;
		}
	}
	return mask;
}

bool Solid(const AlphaMask& mask, glm::vec2 uv)
{
	const auto column = std::clamp(static_cast<int32_t>(uv.x * 64.0f), 0, 63);
	const auto row = std::clamp(static_cast<int32_t>(uv.y * 64.0f), 0, 63);
	return mask[static_cast<size_t>(row) * 64 + static_cast<size_t>(column)];
}

std::optional<float> FirstHit(const View& view, const Primitive& primitive)
{
	PickScratch scratch;
	return FirstHit(view, primitive, scratch);
}

std::optional<float> FirstHit(const View& view, const Primitive& primitive, PickScratch& scratch)
{
	const auto count = primitive.corners.size();
	auto& beyond = scratch.beyond;
	auto& onScreen = scratch.onScreen;
	auto& drawn = scratch.drawn;
	beyond.assign(count, 0);
	onScreen.resize(count);
	drawn.clear();
	for (size_t i = 0; i < count; ++i)
	{
		beyond[i] = SidesBeyond(view, primitive.corners[i]);
		if (beyond[i] == 0)
		{
			onScreen[i] = Project(view, primitive.corners[i]);
		}
	}

	// The triangles drawn, cut to the view where they reach past it
	const auto keep = [&drawn, &primitive](const ScreenCorner& a, const ScreenCorner& b, const ScreenCorner& c) {
		if (primitive.twoSided || FacesCamera(a, b, c))
		{
			drawn.push_back({a, b, c});
		}
	};
	for (size_t t = 0; t + 2 < primitive.indices.size(); t += 3)
	{
		const auto i0 = primitive.indices[t];
		const auto i1 = primitive.indices[t + 1];
		const auto i2 = primitive.indices[t + 2];
		if (i0 >= count || i1 >= count || i2 >= count)
		{
			continue;
		}
		if ((beyond[i0] | beyond[i1] | beyond[i2]) == 0)
		{
			keep(onScreen[i0], onScreen[i1], onScreen[i2]);
			continue;
		}
		if ((beyond[i0] & beyond[i1] & beyond[i2]) != 0)
		{
			continue;
		}
		CutPolygon polygon {.corners = {primitive.corners[i0], primitive.corners[i1], primitive.corners[i2]}, .count = 3};
		for (const auto side : k_Sides)
		{
			CutPolygon cut;
			const auto push = [&cut](const ClipCorner& corner) {
				if (cut.count < cut.corners.size())
				{
					cut.corners[cut.count++] = corner;
				}
			};
			for (size_t k = 0; k < polygon.count; ++k)
			{
				const auto& from = polygon.corners[(k + polygon.count - 1) % polygon.count];
				const auto& to = polygon.corners[k];
				const float fromInside = Inside(view, from, side);
				const float toInside = Inside(view, to, side);
				const auto crossing = [&] {
					const float at = fromInside / (fromInside - toInside);
					const float before = 1.0f - at;
					return ClipCorner {.x = before * from.x + at * to.x,
					                   .y = before * from.y + at * to.y,
					                   .w = before * from.w + at * to.w,
					                   .uv = before * from.uv + at * to.uv};
				};
				if (toInside >= 0.0f)
				{
					if (fromInside < 0.0f)
					{
						push(crossing());
					}
					push(to);
				}
				else if (fromInside >= 0.0f)
				{
					push(crossing());
				}
			}
			polygon = cut;
			if (polygon.count < 3)
			{
				break;
			}
		}
		if (polygon.count < 3)
		{
			continue;
		}
		const auto first = Project(view, polygon.corners[0]);
		for (size_t k = 1; k + 1 < polygon.count; ++k)
		{
			keep(first, Project(view, polygon.corners[k]), Project(view, polygon.corners[k + 1]));
		}
	}

	for (const auto& [a, b, c] : drawn)
	{
		if (!Covers(a, b, c, view.cursor))
		{
			continue;
		}
		const auto at = Interpolate(a, b, c, view.cursor);
		if (primitive.mask != nullptr && !Solid(*primitive.mask, at.uv))
		{
			continue;
		}
		return view.near / at.depth;
	}
	return std::nullopt;
}

std::optional<Picked> PickAmong(const View& view, std::span<const Candidate> candidates,
                                const std::function<std::optional<float>(size_t)>& distanceOf)
{
	std::optional<Picked> picked;
	for (size_t i = 0; i < candidates.size(); ++i)
	{
		const auto& candidate = candidates[i];
		if (!CursorOverSphere(view, candidate.centre, candidate.radius, candidate.origin))
		{
			continue;
		}
		// Nothing further than the nearest object picked so far by more than its own radius is tested
		if (picked.has_value() && picked->distance + candidate.radius < Depth(view, candidate.centre))
		{
			continue;
		}
		const auto distance = distanceOf(i);
		if (distance.has_value() && (!picked.has_value() || *distance < picked->distance))
		{
			picked = Picked {.index = i, .distance = *distance};
		}
	}
	return picked;
}

bool LandHidesObject(float landDistance, float objectDistance, glm::vec2 landPoint, glm::vec2 objectOrigin,
                     glm::vec2 halfExtents)
{
	if (landDistance > objectDistance)
	{
		return false;
	}
	const auto apart = landPoint - objectOrigin;
	return !(apart.y * apart.y + apart.x * apart.x < halfExtents.y * halfExtents.y + halfExtents.x * halfExtents.x);
}

std::optional<MeshHit> NearestIntersection(std::span<const glm::vec3> corners, std::span<const uint16_t> indices,
                                           glm::vec3 origin, glm::vec3 direction, bool behindAllowed)
{
	constexpr float k_SideOn = 0.005f;
	std::optional<MeshHit> nearest;
	for (size_t t = 0; t + 2 < indices.size(); t += 3)
	{
		if (indices[t] >= corners.size() || indices[t + 1] >= corners.size() || indices[t + 2] >= corners.size())
		{
			continue;
		}
		const auto& v0 = corners[indices[t]];
		const auto& v1 = corners[indices[t + 1]];
		const auto& v2 = corners[indices[t + 2]];
		auto normal = glm::cross(v1 - v0, v2 - v0);
		if (normal != glm::vec3(0.0f))
		{
			normal *= 1.0f / std::sqrt(glm::dot(normal, normal));
		}
		const float facing = glm::dot(direction, normal);
		if (!(facing < -k_SideOn || facing > k_SideOn))
		{
			continue;
		}
		const float distance = (glm::dot(v0, normal) - glm::dot(origin, normal)) / facing;
		if (!behindAllowed && !(distance > 0.0f))
		{
			continue;
		}
		const auto point = distance * direction + origin;
		const int positive = static_cast<int>(glm::dot(glm::cross(v0 - v2, point - v2), normal) > 0.0f) +
		                     static_cast<int>(glm::dot(glm::cross(v2 - v1, point - v1), normal) > 0.0f) +
		                     static_cast<int>(glm::dot(glm::cross(v1 - v0, point - v0), normal) > 0.0f);
		if (positive != 0 && positive != 3)
		{
			continue;
		}
		if (nearest.has_value() && !(distance < nearest->distance))
		{
			continue;
		}
		nearest = MeshHit {.point = point, .normal = facing <= 0.0f ? -normal : normal, .distance = distance};
	}
	return nearest;
}

} // namespace openblack::screen_pick
