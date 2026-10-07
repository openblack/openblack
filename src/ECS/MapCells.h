/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <optional>
#include <span>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>

#include "3D/MapCoords.h"

// What stands in each of the map's cells, in the order the game keeps it, which is the order every search of a cell
// meets things in. Each cell has two lists: the buildings, trees, features and other things that stay put, then
// everything that moves. A search of a cell goes through the first list, then the second.
//
// Where a thing goes in its lists depends on what it is:
// - buildings, features, trees and the like go to the front of the first list of every cell they cover, so the
//   newest is met first;
// - piles, bubbles and other things that count as staying put but can be carried go to the back of the first list,
//   behind everything already there;
// - the living and the other things that move go to the front of the second list, so whoever came into the cell last
//   is met first.
// Taking a thing out keeps the others in their order. A thing that moves to another cell is taken out and put in again
// the same way, so it goes to the front (or back) of its new cell.
//
// A building covers the cells its outline touches: the outline is a circle the size of the larger side of the model's
// box, or, for a box more than 1.4 times as long as it is wide, a row of circles the width of the box along its
// length, tested against a circle of 7.1 units at the middle of each cell round it. A thing that touches no cell's
// circle is put in the cell in the middle of the square looked at.
//
// Pure structures and functions, tested on their own.

namespace openblack::ecs::map_cells
{

/// Where a thing goes in a cell
enum class Placement : uint8_t
{
	/// The front of the list of things that stay put: buildings, features and trees
	FixedFront,
	/// The back of the list of things that stay put: piles, bubbles and other things that can be carried
	FixedBack,
	/// The front of the list of things that move
	MobileFront,
};

/// The two lists of every cell of the map
class CellLists
{
public:
	explicit CellLists(uint32_t cellsPerSide = map_coords::k_MapCells);

	/// A cell's index, x times the cells a side plus z; none off the map
	[[nodiscard]] std::optional<uint32_t> IndexOf(glm::ivec2 cell) const;

	void Insert(uint32_t cell, entt::entity entity, Placement placement);
	/// Takes a thing out of a cell, from whichever list it is in, keeping the others in their order
	void Remove(uint32_t cell, entt::entity entity);
	[[nodiscard]] std::span<const entt::entity> Fixed(uint32_t cell) const;
	[[nodiscard]] std::span<const entt::entity> Mobile(uint32_t cell) const;
	/// Everything in a cell as a search meets it: the things that stay put, then the things that move
	[[nodiscard]] std::vector<entt::entity> All(uint32_t cell) const;
	void Clear();

	[[nodiscard]] uint32_t CellsPerSide() const { return _cellsPerSide; }

private:
	struct Cell
	{
		std::vector<entt::entity> fixed;
		std::vector<entt::entity> mobile;
	};

	uint32_t _cellsPerSide;
	std::vector<Cell> _cells;
	/// The cells that have had anything in them, the only ones there is anything to clear
	std::vector<uint32_t> _used;
};

/// The outline of a building on the ground, from its model's box, placed and scaled
struct Outline
{
	/// The middle of the box on the ground
	glm::vec2 centre {0.0f};
	/// Half the box's width (along the model's x) and depth (along its z), scaled
	glm::vec2 halfSize {0.0f};
	/// Half the box's diagonal, scaled: the cells looked at are those within it, and a unit more, of the middle
	float halfDiagonal {0.0f};
	/// Which way the model's x points across the land, a unit long
	glm::vec2 axis {1.0f, 0.0f};
};

/// The radius of the circle at the middle of each cell that a building's outline is tested against
inline constexpr float k_CellTestRadius = 7.1f;
/// A box longer than this many times its width is a row of circles rather than one
inline constexpr float k_RoundBoxRatio = 1.4f;

/// A circle of the outline, on the ground
struct Circle
{
	glm::vec2 centre {0.0f};
	float radius {0.0f};
};

/// The circles a building's outline is made of: one, or a row along the box's length. The first is the bounding circle
/// that a cell must touch before the row is tried, and is all there is for a round box.
struct OutlineCircles
{
	Circle bounds;
	std::vector<Circle> row;
};
[[nodiscard]] OutlineCircles CirclesOf(const Outline& outline);

/// Whether a cell's circle touches an outline
[[nodiscard]] bool Touches(const OutlineCircles& circles, glm::ivec2 cell);

/// The cells a building covers, x by x and in each x by z, the order the building goes into them
[[nodiscard]] std::vector<glm::ivec2> CellsCovered(const Outline& outline, uint32_t cellsPerSide = map_coords::k_MapCells);

} // namespace openblack::ecs::map_cells
