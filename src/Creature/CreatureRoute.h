/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <functional>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// Where a creature can walk, and the routes it takes there.
///
/// The land is a grid of 512 by 512 cells of 10 units, sorted once a land is loaded: cells steeper than about 45
/// degrees and deep sea are blocked, shallow water can be waded through, and land cut off from the main part of the
/// land is out of reach. Things in the way are circles to walk round. Routes are planned over a finer lattice across
/// as many game turns as they take, straightened where nothing is in the way, and their corners rounded so the
/// creature can walk round them without stopping.
namespace openblack::creature_route
{
constexpr int32_t k_CellsPerSide = 512;
constexpr float k_CellSize = 10.0f;
/// A cell whose corners differ in height by more than this is too steep to walk on
constexpr float k_SteepRise = 10.0f;
/// A creature stands clear of cells it can't walk on by this much, and a destination by a little more
constexpr float k_Clearance = 7.05f;
constexpr float k_DestinationClearance = 7.1f;
/// The spacing of the lattice routes are planned on
constexpr float k_LatticeStep = 2.5f;

enum class Ground : uint8_t
{
	/// Land a creature can walk on
	Open = 0,
	/// Too steep, or deep sea
	Blocked = 1,
	/// Land cut off from the main part of the land
	CutOff = 4,
	/// Shallow water, waded through on the bottom
	Water = 6,
};

/// The land's cells, sorted by where creatures can walk
class WalkableLand
{
public:
	/// The height of a cell corner, x and z from 0 to 512, in world units
	using CornerHeight = std::function<float(int32_t x, int32_t z)>;
	/// Whether a cell has water in it, or nothing when the land has no such cell
	using CellWater = std::function<std::optional<bool>(int32_t x, int32_t z)>;

	WalkableLand();
	/// Sorts every cell: blocked where too steep or deep sea (all four corners at the bottom), then the largest stretch
	/// of land joined up side by side is open and the rest cut off, and open cells with water in them or none at all
	/// are water
	[[nodiscard]] static WalkableLand Build(const CornerHeight& height, const CellWater& water);

	[[nodiscard]] Ground At(int32_t x, int32_t z) const;
	[[nodiscard]] Ground AtPoint(glm::vec2 point) const;
	/// Whether a creature may stand at a point: its cell is open or water, and no cell it may not stand on has its
	/// centre within the radius
	[[nodiscard]] bool IsValid(glm::vec2 point, float radius) const;
	/// The nearest point where a creature may stand, searching outwards in 16 directions half a unit at a time
	[[nodiscard]] std::optional<glm::vec2> NearestValid(glm::vec2 point, float radius, float maxDistance) const;

private:
	std::vector<Ground> _cells;
};

/// Something in the way, walked round at its radius, which includes the creature's own
struct Circle
{
	glm::vec2 centre;
	float radius;
};

/// What kind of thing might be in a creature's way
enum class Obstacle : uint8_t
{
	Tree,
	Building,
	/// Anything else fixed to the land, such as rocks and fences
	Object,
	/// Villagers and animals, which are never walked round
	Living,
	StandingCreature,
	MovingCreature,
};
/// A tree at least this many times the creature's height is walked round, a smaller one through
constexpr float k_TreeAvoidHeight = 1.5f;
/// Anything else at least this many times the creature's height is walked round
constexpr float k_ObjectAvoidHeight = 0.1f;
/// Whether a creature walks round something, or through or over it: big creatures walk through small trees and step
/// over low things, never round villagers and animals, and round other creatures only while they stand still.
/// Anything burning is walked round.
[[nodiscard]] bool MustAvoid(Obstacle kind, float height, float creatureHeight, bool burning);

/// The shortest distance from a point to a segment
[[nodiscard]] float DistanceToSegment(glm::vec2 point, glm::vec2 from, glm::vec2 to);

/// A creature's side step towards a catch is checked against one block of 8 by 8 cells, 80 units a side
constexpr int32_t k_StepBlockCells = 8;
constexpr float k_StepBlockSize = 80.0f;
/// The block a step ending at a point is checked against: its column from the point's x and, as the game has it, its row
/// from the point's height rather than its z, so that for ordinary heights it is a block of the land's first rows
[[nodiscard]] glm::ivec2 StepBlock(glm::vec3 end);
/// The circles a side step must not end in, gathered over a block as the game gathers them: for each cell of the block,
/// a circle of the destination clearance at the centre of each of the cell and its eight neighbours that may not be
/// stood on. The game reads each neighbour's sort from the cell one row further on, and only blocked cells count (water
/// is waded). A circle about a point within a tenth of a unit of where the creature stands is left out, and one reaching
/// further than it is shrunk to reach it; one reaching off the land's last 80 units is left out.
[[nodiscard]] std::vector<Circle> StepBlockCircles(const WalkableLand& land, glm::ivec2 block, glm::vec2 standing);
/// Whether a point lies strictly inside any of the circles
[[nodiscard]] bool InsideAny(glm::vec2 point, const std::vector<Circle>& circles);

/// Where to go and what is in the way
struct Request
{
	glm::vec2 start;
	glm::vec2 destination;
	/// Arriving is anywhere from min to max away from the destination
	float minDistance;
	float maxDistance;
	std::vector<Circle> obstacles;
	/// Corners are rounded at this radius
	float cornerRadius;
	/// The most lattice points searched before giving up
	size_t searchLimit {200000};
};

/// Plans a route over the lattice by A*, a budget of lattice points a turn, then straightens and rounds it. Circles
/// around the start or the destination are left out, so a creature can walk out of something it stands in and up to
/// something it walks to.
class Planner
{
public:
	enum class Status : uint8_t
	{
		Planning,
		Found,
		Failed,
	};

	explicit Planner(Request request);

	/// Searches up to a budget of lattice points
	Status Step(const WalkableLand& land, size_t budget);
	[[nodiscard]] Status GetStatus() const { return _status; }
	/// The route found, from the start to the arrival point
	[[nodiscard]] const std::vector<glm::vec2>& GetRoute() const { return _route; }
	[[nodiscard]] size_t GetSearched() const { return _searched; }

private:
	struct Node
	{
		float cost;
		int64_t parent;
		bool closed;
	};
	struct Open
	{
		float estimate;
		int64_t key;
		bool operator>(const Open& other) const { return estimate > other.estimate; }
	};

	[[nodiscard]] glm::vec2 PointOf(int64_t key) const;
	[[nodiscard]] int64_t KeyOf(int32_t i, int32_t j) const;
	[[nodiscard]] bool Passable(const WalkableLand& land, glm::vec2 point) const;
	[[nodiscard]] bool IsGoal(glm::vec2 point) const;
	[[nodiscard]] float Estimate(glm::vec2 point) const;
	[[nodiscard]] bool Clear(const WalkableLand& land, glm::vec2 from, glm::vec2 to) const;
	[[nodiscard]] glm::vec2 ArrivalFrom(glm::vec2 from) const;
	void Finish(const WalkableLand& land, int64_t goal);

	Request _request;
	Status _status {Status::Planning};
	bool _started {false};
	std::unordered_map<int64_t, Node> _nodes;
	std::priority_queue<Open, std::vector<Open>, std::greater<>> _open;
	size_t _searched {0};
	std::vector<glm::vec2> _route;
};

/// Rounds a polyline's corners into arcs of a radius, smaller where the segments are too short for it, each step of
/// an arc turning at most maxStepAngle
[[nodiscard]] std::vector<glm::vec2> RoundCorners(const std::vector<glm::vec2>& points, float radius, float maxStepAngle);
/// Keeps only the points needed to go between where the way between is clear
[[nodiscard]] std::vector<glm::vec2> Straighten(const std::vector<glm::vec2>& points,
                                                const std::function<bool(glm::vec2, glm::vec2)>& clear);

/// A route being followed: along each segment in turn
struct Route
{
	std::vector<glm::vec2> points;
	/// The segment from points[segment] to points[segment + 1], and how far along it the creature is
	size_t segment {0};
	float travelled {0.0f};

	[[nodiscard]] bool Finished() const { return segment + 1 >= points.size(); }
	[[nodiscard]] bool OnLastSegment() const { return segment + 2 >= points.size(); }
	[[nodiscard]] float SegmentLength() const;
	/// How far is left of this segment, and of the whole route
	[[nodiscard]] float Remaining() const;
	[[nodiscard]] float RemainingTotal() const;
	[[nodiscard]] glm::vec2 Position() const;
	/// The heading along this segment (see creature_locomotion::HeadingOf)
	[[nodiscard]] float Heading() const;
	/// How far the route turns at the end of this segment, 0 at the last
	[[nodiscard]] float TurnAtEnd() const;
};

struct Advanced
{
	glm::vec2 position;
	float heading;
	/// Whether it went on to a new segment
	bool segmentChanged;
	bool finished;
};
/// Moves a distance along the route
Advanced Advance(Route& route, float distance);
} // namespace openblack::creature_route
