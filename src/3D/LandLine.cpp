/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LandLine.h"

#include <cmath>

#include <algorithm>

#include "Camera/CameraPan.h"

namespace openblack::land_line
{
namespace
{
/// A triangle's plane closer to level with the line than this, in height units per cell, is not met by it, and the cell
/// is passed over
constexpr double k_LevelWithLine = 0.0001;
/// How far, in cells, a cell's triangles reach past its edges and their shared diagonal
constexpr float k_Reach = 1.1f;
constexpr float k_ReachBefore = -0.1f;
constexpr float k_ReachAcross = 0.9f;
/// The share of the line, from the start to the map's edge, the game takes as "never" when the line runs along an axis
constexpr float k_Never = 1.0000000200408773e20f;
/// No walk over a 512 by 512 map can cross more cells than this
constexpr int k_MostCells = 4 * 512;

/// The sides of the map along z a point is past: 4 before z's edge, 8 past its far edge
[[nodiscard]] int SidesPastZ(float z)
{
	if (z < k_EdgeMargin)
	{
		return 4;
	}
	return z > k_FarEdge ? 8 : 0;
}

/// The sides of the map an end of the line is past: 1 before x's edge, 2 past its far edge, 4 and 8 the same for z
[[nodiscard]] int SidesPast(float x, float z)
{
	int sides = 0;
	if (x < k_EdgeMargin)
	{
		sides = 1;
	}
	else if (x > k_FarEdge)
	{
		sides = 2;
	}
	return sides | SidesPastZ(z);
}

} // namespace

std::optional<glm::vec2> HitInCell(int32_t x, int32_t z, glm::vec3 from, glm::vec3 to, const CellLookup& cells)
{
	if (x < 0 || x > 511 || z < 0 || z > 511)
	{
		return std::nullopt;
	}
	const auto corners = cells(x, z);
	if (!corners.has_value())
	{
		return std::nullopt;
	}
	const int here = corners->here;
	const int acrossX = corners->acrossX;
	const int acrossZ = corners->acrossZ;
	const int acrossBoth = corners->acrossBoth;

	// A line wholly below the lowest corner, or wholly above the highest, misses the cell
	const auto highest = static_cast<float>(std::max({here, acrossX, acrossZ, acrossBoth}));
	const auto lowest = static_cast<float>(std::min({here, acrossX, acrossZ, acrossBoth}));
	if (from.y < lowest && to.y < lowest)
	{
		return std::nullopt;
	}
	if (highest < from.y && highest < to.y)
	{
		return std::nullopt;
	}

	const float alongX = to.x - from.x;
	const float alongZ = to.z - from.z;
	const float climb = to.y - from.y;
	const auto cellX = static_cast<float>(x);
	const auto cellZ = static_cast<float>(z);
	const float fromCellX = from.x - cellX;
	const float fromCellZ = from.z - cellZ;

	// Where on the cell the line meets a triangle's plane, and whether that is within the triangle's reach and not
	// behind the line's start
	const auto onCell = [&](float share, bool far) -> std::optional<glm::vec2> {
		const float u = (share * alongX + from.x) - cellX;
		const float v = (share * alongZ + from.z) - cellZ;
		if (u > k_Reach || v > k_Reach || u < k_ReachBefore || v < k_ReachBefore)
		{
			return std::nullopt;
		}
		if (far ? (k_ReachAcross - v > u) : (k_Reach - v < u))
		{
			return std::nullopt;
		}
		const float hitX = cellX + u;
		const float hitZ = cellZ + v;
		if ((hitX - from.x) * alongX + (hitZ - from.z) * alongZ < 0.0f)
		{
			return std::nullopt;
		}
		return glm::vec2(hitX, hitZ);
	};

	// The triangle on the corners at the cell, across x and across z. A line level with it misses the whole cell.
	const auto slopeX = static_cast<float>(acrossX - here);
	const auto slopeZ = static_cast<float>(acrossZ - here);
	const float near = (climb - slopeX * alongX) - slopeZ * alongZ;
	if (std::abs(near) < k_LevelWithLine)
	{
		return std::nullopt;
	}
	const float nearShare = ((fromCellZ * slopeZ + fromCellX * slopeX + static_cast<float>(here)) - from.y) / near;
	if (const auto hit = onCell(nearShare, false))
	{
		return hit;
	}

	// The triangle on the corners across x, across both and across z
	const auto farSlopeX = static_cast<float>(acrossBoth - acrossZ);
	const auto farSlopeZ = static_cast<float>(acrossBoth - acrossX);
	const float far = (climb - farSlopeX * alongX) - farSlopeZ * alongZ;
	if (std::abs(far) < k_LevelWithLine)
	{
		return std::nullopt;
	}
	const float farShare =
	    (((static_cast<float>(acrossX) - farSlopeX) + fromCellZ * farSlopeZ) + fromCellX * farSlopeX - from.y) / far;
	return onCell(farShare, true);
}

std::optional<glm::vec2> FirstHit(glm::vec3 from, glm::vec3 to, const CellLookup& cells)
{
	float x0 = from.x;
	float z0 = from.z;
	float y0 = from.y;
	float x1 = to.x;
	float z1 = to.z;
	float y1 = to.y;

	// The line carries on past its end by as much again as it takes from its start to the map's edge. The game means to
	// skip this when the line never reaches an edge, but its test never passes, so a line along y is carried on too.
	float share = k_Never;
	if (x0 > x1)
	{
		share = x0 / (x0 - x1);
	}
	else if (x0 < x1)
	{
		share = (k_MapCells - x0) / (x1 - x0);
	}
	float shareZ = k_Never;
	if (z0 > z1)
	{
		shareZ = z0 / (z0 - z1);
	}
	else if (z0 < z1)
	{
		shareZ = (k_MapCells - z0) / (z1 - z0);
	}
	if (share > shareZ)
	{
		share = shareZ;
	}
	x1 = (x1 - x0) * share + x1;
	z1 = (z1 - z0) * share + z1;
	y1 = (y1 - y0) * share + y1;

	// Kept within the map, the ends cut where they leave it: x's near edge, its far edge, then z's
	int sidesFrom = SidesPast(x0, z0);
	int sidesTo = SidesPast(x1, z1);
	if ((sidesFrom & sidesTo) != 0)
	{
		return std::nullopt;
	}
	if ((sidesFrom | sidesTo) != 0)
	{
		const auto cutAtX = [&](float edge, int side) {
			const float at = (edge - x0) / (x1 - x0);
			const float z = (z1 - z0) * at + z0;
			const float y = (y1 - y0) * at + y0;
			if ((sidesTo & side) != 0)
			{
				y1 = y;
				z1 = z;
				x1 = edge;
				sidesTo = SidesPastZ(z);
			}
			else
			{
				y0 = y;
				z0 = z;
				x0 = edge;
				sidesFrom = SidesPastZ(z);
			}
		};
		const auto cutAtZ = [&](float edge, int side) {
			const float at = (edge - z0) / (z1 - z0);
			const float x = (x1 - x0) * at + x0;
			const float y = (y1 - y0) * at + y0;
			if ((sidesTo & side) != 0)
			{
				y1 = y;
				z1 = edge;
				sidesTo = 0;
				x1 = x;
			}
			else
			{
				y0 = y;
				z0 = edge;
				sidesFrom = 0;
				x0 = x;
			}
		};
		if (((sidesFrom ^ sidesTo) & 1) != 0)
		{
			cutAtX(k_EdgeMargin, 1);
		}
		if ((sidesFrom & sidesTo) != 0)
		{
			return std::nullopt;
		}
		if (((sidesFrom ^ sidesTo) & 2) != 0)
		{
			cutAtX(k_FarEdge, 2);
		}
		if ((sidesFrom & sidesTo) != 0)
		{
			return std::nullopt;
		}
		if (((sidesFrom ^ sidesTo) & 4) != 0)
		{
			cutAtZ(k_EdgeMargin, 4);
		}
		if ((sidesFrom & sidesTo) != 0)
		{
			return std::nullopt;
		}
		if (((sidesFrom ^ sidesTo) & 8) != 0)
		{
			cutAtZ(k_FarEdge, 8);
		}
		if ((sidesFrom & sidesTo) != 0)
		{
			return std::nullopt;
		}
	}

	const glm::vec3 end(x1, y1, z1);
	auto cellX = static_cast<int32_t>(x0);
	auto cellZ = static_cast<int32_t>(z0);
	const auto endX = static_cast<int32_t>(x1);
	const auto endZ = static_cast<int32_t>(z1);
	glm::vec3 at(x0, y0, z0);

	// Along a column or a row each cell is tested with the whole line
	if (cellX == endX)
	{
		const int step = cellZ < endZ ? 1 : -1;
		for (; cellZ != endZ; cellZ += step)
		{
			if (const auto hit = HitInCell(cellX, cellZ, at, end, cells))
			{
				return hit;
			}
		}
		return HitInCell(cellX, cellZ, at, end, cells);
	}
	if (cellZ == endZ)
	{
		const int step = cellX < endX ? 1 : -1;
		for (; cellX != endX; cellX += step)
		{
			if (const auto hit = HitInCell(cellX, cellZ, at, end, cells))
			{
				return hit;
			}
		}
		return HitInCell(cellX, cellZ, at, end, cells);
	}

	// Otherwise cell by cell, each with the stretch of the line from where it comes into the cell to where it leaves
	const float alongX = x1 - x0;
	const float alongZ = z1 - z0;
	const float xPerZ = alongX / alongZ;
	const float zPerX = 1.0f / xPerZ;
	const float yPerZ = (y1 - y0) / alongZ;
	const float yPerX = (y1 - y0) / alongX;
	const bool towardsLowerZ = z0 > z1;
	const bool towardsLowerX = x0 > x1;
	for (int crossed = 0; crossed < k_MostCells; ++crossed)
	{
		const bool more = (towardsLowerX ? cellX > endX : cellX < endX) || (towardsLowerZ ? cellZ > endZ : cellZ < endZ);
		if (!more)
		{
			break;
		}
		const int testX = cellX;
		const int testZ = cellZ;
		glm::vec3 next;
		if (towardsLowerZ)
		{
			// Where the line reaches the cell's near z edge, unless it leaves across x first
			const float toEdge = at.z - static_cast<float>(cellZ);
			const float xAtEdge = at.x - toEdge * xPerZ;
			if (static_cast<int32_t>(xAtEdge) == cellX)
			{
				next = glm::vec3(xAtEdge, at.y - toEdge * yPerZ, static_cast<float>(cellZ));
				--cellZ;
			}
			else if (!towardsLowerX)
			{
				++cellX;
				const float toX = static_cast<float>(cellX) - at.x;
				const float z = toX * zPerX + at.z;
				cellZ = static_cast<int32_t>(z);
				next = glm::vec3(static_cast<float>(cellX), toX * yPerX + at.y, z);
			}
			else
			{
				const float toX = at.x - static_cast<float>(cellX);
				const float z = at.z - toX * zPerX;
				next = glm::vec3(static_cast<float>(cellX), at.y - toX * yPerX, z);
				cellZ = static_cast<int32_t>(z);
				--cellX;
			}
		}
		else
		{
			// Where the line reaches the cell's far z edge, unless it leaves across x first
			++cellZ;
			const float toEdge = static_cast<float>(cellZ) - at.z;
			const float xAtEdge = toEdge * xPerZ + at.x;
			if (static_cast<int32_t>(xAtEdge) == cellX)
			{
				next = glm::vec3(xAtEdge, toEdge * yPerZ + at.y, static_cast<float>(cellZ));
			}
			else if (!towardsLowerX)
			{
				++cellX;
				const float toX = static_cast<float>(cellX) - at.x;
				const float z = toX * zPerX + at.z;
				cellZ = static_cast<int32_t>(z);
				next = glm::vec3(static_cast<float>(cellX), toX * yPerX + at.y, z);
			}
			else
			{
				const float toX = at.x - static_cast<float>(cellX);
				const float z = at.z - toX * zPerX;
				next = glm::vec3(static_cast<float>(cellX), at.y - toX * yPerX, z);
				cellZ = static_cast<int32_t>(z);
				--cellX;
			}
		}
		if (const auto hit = HitInCell(testX, testZ, at, next, cells))
		{
			return hit;
		}
		at = next;
	}
	return HitInCell(cellX, cellZ, at, end, cells);
}

namespace
{
[[nodiscard]] glm::vec3 ToCells(glm::vec3 metres)
{
	constexpr double k_PerMetre = 0.1;
	return {static_cast<float>(metres.x * k_PerMetre), metres.y / k_HeightUnit, static_cast<float>(metres.z * k_PerMetre)};
}
} // namespace

std::optional<glm::vec2> LandAlong(glm::vec3 from, glm::vec3 to, const CellLookup& cells)
{
	if (const auto hit = FirstHit(ToCells(from), ToCells(to), cells))
	{
		return *hit * k_CellSize;
	}
	return std::nullopt;
}

std::optional<glm::vec2> LandOrSeaAlong(glm::vec3 from, glm::vec3 to, glm::vec3 camera, const CellLookup& cells)
{
	if (const auto land = LandAlong(from, to, cells))
	{
		return land;
	}
	if (const auto sea = camera_pan::SeaHit(from, to, camera))
	{
		return glm::vec2(sea->x, sea->z);
	}
	return std::nullopt;
}

std::optional<glm::vec3> UnderPixel(glm::vec3 camera, glm::vec3 nearPoint, bool withSea, const CellLookup& cells,
                                    const std::function<float(glm::vec2)>& heightAt)
{
	glm::vec3 point;
	if (const auto hit = withSea ? LandOrSeaAlong(camera, nearPoint, camera, cells) : LandAlong(camera, nearPoint, cells))
	{
		point = glm::vec3(hit->x, 0.0f, hit->y);
	}
	else if (withSea && camera.y > nearPoint.y)
	{
		// The sea's level wherever the line from the camera goes down
		const float share = -(camera.y / (nearPoint.y - camera.y));
		point = glm::vec3((nearPoint.x - camera.x) * share + camera.x, 0.0f, (nearPoint.z - camera.z) * share + camera.z);
	}
	else
	{
		return std::nullopt;
	}
	point.y = heightAt({point.x, point.z});
	return point;
}

glm::vec3 KeptInReach(glm::vec3 point)
{
	constexpr float k_Middle = k_CellSize * k_MapCells * 0.5f;
	constexpr float k_PickReach = k_CellSize * 1536.0f * 0.5f;
	auto fromMiddle = glm::vec3(point.x - k_Middle, point.y, point.z - k_Middle);
	const float length = std::sqrt(fromMiddle.z * fromMiddle.z + fromMiddle.y * fromMiddle.y + fromMiddle.x * fromMiddle.x);
	if (!(k_PickReach < length))
	{
		return point;
	}
	if (fromMiddle.x != 0.0f || fromMiddle.y != 0.0f || fromMiddle.z != 0.0f)
	{
		fromMiddle *= k_PickReach / length;
	}
	return {fromMiddle.x + k_Middle, fromMiddle.y, fromMiddle.z + k_Middle};
}

} // namespace openblack::land_line
