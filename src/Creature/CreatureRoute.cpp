/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureRoute.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <limits>
#include <numbers>
#include <ranges>

#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::creature_route;

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;
constexpr int32_t k_LatticePerSide = static_cast<int32_t>(static_cast<float>(k_CellsPerSide) * k_CellSize / k_LatticeStep);
/// Searching outwards for somewhere to stand: in this many directions, this much further each time
constexpr int32_t k_SearchDirections = 16;
constexpr float k_SearchStep = 0.5f;
/// Checking the way between two points is clear, every this many units
constexpr float k_ClearStep = 1.0f;
constexpr float k_Tiny = 1e-4f;

bool Standable(Ground ground)
{
	return ground == Ground::Open || ground == Ground::Water;
}

float Cross(glm::vec2 a, glm::vec2 b)
{
	return (a.x * b.y) - (a.y * b.x);
}

glm::vec2 Rotate(glm::vec2 v, float angle)
{
	const auto c = std::cos(angle);
	const auto s = std::sin(angle);
	return {(v.x * c) - (v.y * s), (v.x * s) + (v.y * c)};
}

float HeadingAlong(glm::vec2 from, glm::vec2 to)
{
	const auto d = to - from;
	return std::atan2(-d.x, -d.y);
}
} // namespace

WalkableLand::WalkableLand()
    : _cells(static_cast<size_t>(k_CellsPerSide * k_CellsPerSide), Ground::Open)
{
}

WalkableLand WalkableLand::Build(const CornerHeight& height, const CellWater& water)
{
	WalkableLand land;
	const auto index = [](int32_t x, int32_t z) { return static_cast<size_t>((z * k_CellsPerSide) + x); };
	for (int32_t z = 0; z < k_CellsPerSide; ++z)
	{
		for (int32_t x = 0; x < k_CellsPerSide; ++x)
		{
			const std::array corners {height(x, z), height(x + 1, z), height(x, z + 1), height(x + 1, z + 1)};
			const auto [low, high] = std::ranges::minmax(corners);
			const bool deep = high <= 0.0f;
			land._cells[index(x, z)] = (high - low > k_SteepRise || deep) ? Ground::Blocked : Ground::CutOff;
		}
	}

	// The largest stretch of walkable cells joined side by side is the land; the rest is cut off from it
	std::vector<int32_t> labels(land._cells.size(), -1);
	std::vector<size_t> sizes;
	std::vector<size_t> stack;
	for (size_t start = 0; start < land._cells.size(); ++start)
	{
		if (land._cells[start] == Ground::Blocked || labels[start] >= 0)
		{
			continue;
		}
		const auto label = static_cast<int32_t>(sizes.size());
		size_t count = 0;
		stack.push_back(start);
		labels[start] = label;
		while (!stack.empty())
		{
			const auto cell = stack.back();
			stack.pop_back();
			++count;
			const auto x = static_cast<int32_t>(cell % k_CellsPerSide);
			const auto z = static_cast<int32_t>(cell / k_CellsPerSide);
			for (const auto [dx, dz] : std::array<std::array<int32_t, 2>, 4> {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}})
			{
				const auto nx = x + dx;
				const auto nz = z + dz;
				if (nx < 0 || nz < 0 || nx >= k_CellsPerSide || nz >= k_CellsPerSide)
				{
					continue;
				}
				const auto next = index(nx, nz);
				if (land._cells[next] != Ground::Blocked && labels[next] < 0)
				{
					labels[next] = label;
					stack.push_back(next);
				}
			}
		}
		sizes.push_back(count);
	}
	if (sizes.empty())
	{
		return land;
	}
	const auto largest = static_cast<int32_t>(std::distance(sizes.begin(), std::ranges::max_element(sizes)));
	for (size_t i = 0; i < land._cells.size(); ++i)
	{
		if (labels[i] != largest)
		{
			continue;
		}
		const auto wet = water(static_cast<int32_t>(i % k_CellsPerSide), static_cast<int32_t>(i / k_CellsPerSide));
		land._cells[i] = !wet.has_value() || *wet ? Ground::Water : Ground::Open;
	}
	return land;
}

Ground WalkableLand::At(int32_t x, int32_t z) const
{
	if (x < 0 || z < 0 || x >= k_CellsPerSide || z >= k_CellsPerSide)
	{
		return Ground::Blocked;
	}
	return _cells[static_cast<size_t>((z * k_CellsPerSide) + x)];
}

Ground WalkableLand::AtPoint(glm::vec2 point) const
{
	return At(static_cast<int32_t>(std::floor(point.x / k_CellSize)), static_cast<int32_t>(std::floor(point.y / k_CellSize)));
}

bool WalkableLand::IsValid(glm::vec2 point, float radius) const
{
	const auto cx = static_cast<int32_t>(std::floor(point.x / k_CellSize));
	const auto cz = static_cast<int32_t>(std::floor(point.y / k_CellSize));
	if (!Standable(At(cx, cz)))
	{
		return false;
	}
	const auto reach = static_cast<int32_t>(std::ceil(radius / k_CellSize));
	for (int32_t dz = -reach; dz <= reach; ++dz)
	{
		for (int32_t dx = -reach; dx <= reach; ++dx)
		{
			if (Standable(At(cx + dx, cz + dz)))
			{
				continue;
			}
			const glm::vec2 centre {(static_cast<float>(cx + dx) + 0.5f) * k_CellSize,
			                        (static_cast<float>(cz + dz) + 0.5f) * k_CellSize};
			if (glm::distance(point, centre) < radius)
			{
				return false;
			}
		}
	}
	return true;
}

std::optional<glm::vec2> WalkableLand::NearestValid(glm::vec2 point, float radius, float maxDistance) const
{
	if (IsValid(point, radius))
	{
		return point;
	}
	for (float distance = k_SearchStep; distance <= maxDistance; distance += k_SearchStep)
	{
		for (int32_t i = 0; i < k_SearchDirections; ++i)
		{
			const auto angle = 2.0f * k_Pi * static_cast<float>(i) / static_cast<float>(k_SearchDirections);
			const auto candidate = point + (distance * glm::vec2(std::cos(angle), std::sin(angle)));
			if (IsValid(candidate, radius))
			{
				return candidate;
			}
		}
	}
	return std::nullopt;
}

bool creature_route::MustAvoid(Obstacle kind, float height, float creatureHeight, bool burning)
{
	switch (kind)
	{
	case Obstacle::Living:
		return false;
	case Obstacle::StandingCreature:
		return true;
	case Obstacle::MovingCreature:
		return burning;
	case Obstacle::Tree:
		return burning || height >= k_TreeAvoidHeight * creatureHeight;
	case Obstacle::Building:
	case Obstacle::Object:
	default:
		return burning || height >= k_ObjectAvoidHeight * creatureHeight;
	}
}

float creature_route::DistanceToSegment(glm::vec2 point, glm::vec2 from, glm::vec2 to)
{
	const auto along = to - from;
	const auto lengthSquared = glm::dot(along, along);
	if (lengthSquared <= 0.0f)
	{
		return glm::distance(point, from);
	}
	const auto t = std::clamp(glm::dot(point - from, along) / lengthSquared, 0.0f, 1.0f);
	return glm::distance(point, from + (t * along));
}

Planner::Planner(Request request)
    : _request(std::move(request))
{
}

glm::vec2 Planner::PointOf(int64_t key) const
{
	const auto i = static_cast<int32_t>(key / k_LatticePerSide);
	const auto j = static_cast<int32_t>(key % k_LatticePerSide);
	return {(static_cast<float>(i) + 0.5f) * k_LatticeStep, (static_cast<float>(j) + 0.5f) * k_LatticeStep};
}

int64_t Planner::KeyOf(int32_t i, int32_t j) const
{
	return (static_cast<int64_t>(i) * k_LatticePerSide) + j;
}

bool Planner::Passable(const WalkableLand& land, glm::vec2 point) const
{
	if (!land.IsValid(point, k_Clearance))
	{
		return false;
	}
	return std::ranges::none_of(_request.obstacles,
	                            [point](const Circle& circle) { return glm::distance(point, circle.centre) < circle.radius; });
}

bool Planner::IsGoal(glm::vec2 point) const
{
	const auto distance = glm::distance(point, _request.destination);
	return distance <= _request.maxDistance + k_LatticeStep && distance >= _request.minDistance - k_LatticeStep;
}

float Planner::Estimate(glm::vec2 point) const
{
	return std::max(glm::distance(point, _request.destination) - _request.maxDistance, 0.0f);
}

bool Planner::Clear(const WalkableLand& land, glm::vec2 from, glm::vec2 to) const
{
	const auto length = glm::distance(from, to);
	const auto samples = static_cast<int32_t>(std::ceil(length / k_ClearStep));
	for (int32_t k = 1; k <= samples; ++k)
	{
		const auto point = from + ((to - from) * (static_cast<float>(k) / static_cast<float>(samples)));
		if (!land.IsValid(point, k_Clearance))
		{
			return false;
		}
	}
	return std::ranges::none_of(_request.obstacles, [from, to](const Circle& circle) {
		return DistanceToSegment(circle.centre, from, to) < circle.radius;
	});
}

glm::vec2 Planner::ArrivalFrom(glm::vec2 from) const
{
	const auto offset = from - _request.destination;
	const auto distance = glm::length(offset);
	if (distance <= k_Tiny)
	{
		return _request.destination;
	}
	return _request.destination + (offset / distance * std::clamp(distance, _request.minDistance, _request.maxDistance));
}

void Planner::Finish(const WalkableLand& land, int64_t goal)
{
	std::vector<glm::vec2> points;
	for (auto key = goal; key >= 0; key = _nodes.at(key).parent)
	{
		points.push_back(PointOf(key));
	}
	std::ranges::reverse(points);
	points.insert(points.begin(), _request.start);
	points.push_back(ArrivalFrom(points.back()));
	const auto straight = Straighten(points, [this, &land](glm::vec2 from, glm::vec2 to) { return Clear(land, from, to); });
	_route = RoundCorners(straight, _request.cornerRadius, k_Pi / 18.0f);
	_status = Status::Found;
	_nodes.clear();
	_open = {};
}

Planner::Status Planner::Step(const WalkableLand& land, size_t budget)
{
	if (_status != Status::Planning)
	{
		return _status;
	}
	if (!_started)
	{
		_started = true;
		// Leave out what the creature stands in and what it walks up to
		std::erase_if(_request.obstacles, [this](const Circle& circle) {
			return glm::distance(_request.start, circle.centre) < circle.radius ||
			       glm::distance(_request.destination, circle.centre) < circle.radius;
		});
		const auto arrival = ArrivalFrom(_request.start);
		if (glm::distance(_request.start, _request.destination) <= _request.maxDistance || Clear(land, _request.start, arrival))
		{
			_route = {_request.start, arrival};
			_status = Status::Found;
			return _status;
		}
		const auto i = static_cast<int32_t>(std::floor(_request.start.x / k_LatticeStep));
		const auto j = static_cast<int32_t>(std::floor(_request.start.y / k_LatticeStep));
		const auto key = KeyOf(i, j);
		_nodes.emplace(key, Node {.cost = 0.0f, .parent = -1, .closed = false});
		_open.push({.estimate = Estimate(PointOf(key)), .key = key});
	}

	constexpr std::array<std::array<int32_t, 2>, 8> k_Neighbours {
	    {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};
	for (size_t searched = 0; searched < budget && !_open.empty(); ++searched)
	{
		const auto top = _open.top();
		_open.pop();
		auto& node = _nodes.at(top.key);
		if (node.closed)
		{
			continue;
		}
		node.closed = true;
		++_searched;
		const auto point = PointOf(top.key);
		if (IsGoal(point))
		{
			Finish(land, top.key);
			return _status;
		}
		const auto i = static_cast<int32_t>(top.key / k_LatticePerSide);
		const auto j = static_cast<int32_t>(top.key % k_LatticePerSide);
		for (const auto [di, dj] : k_Neighbours)
		{
			const auto ni = i + di;
			const auto nj = j + dj;
			if (ni < 0 || nj < 0 || ni >= k_LatticePerSide || nj >= k_LatticePerSide)
			{
				continue;
			}
			const auto nextKey = KeyOf(ni, nj);
			const auto nextPoint = PointOf(nextKey);
			auto [next, inserted] =
			    _nodes.try_emplace(nextKey, Node {.cost = std::numeric_limits<float>::max(), .parent = -1, .closed = false});
			if (next->second.closed)
			{
				continue;
			}
			if (inserted && !Passable(land, nextPoint))
			{
				next->second.closed = true;
				continue;
			}
			const auto cost = node.cost + (k_LatticeStep * ((di != 0 && dj != 0) ? std::numbers::sqrt2_v<float> : 1.0f));
			if (cost < next->second.cost)
			{
				next->second.cost = cost;
				next->second.parent = top.key;
				_open.push({.estimate = cost + Estimate(nextPoint), .key = nextKey});
			}
		}
		if (_searched >= _request.searchLimit)
		{
			break;
		}
	}
	if (_open.empty() || _searched >= _request.searchLimit)
	{
		_status = Status::Failed;
		_nodes.clear();
		_open = {};
	}
	return _status;
}

std::vector<glm::vec2> creature_route::Straighten(const std::vector<glm::vec2>& points,
                                                  const std::function<bool(glm::vec2, glm::vec2)>& clear)
{
	if (points.size() <= 2)
	{
		return points;
	}
	std::vector<glm::vec2> result {points.front()};
	size_t anchor = 0;
	while (anchor + 1 < points.size())
	{
		auto reach = anchor + 1;
		while (reach + 1 < points.size() && clear(points[anchor], points[reach + 1]))
		{
			++reach;
		}
		result.push_back(points[reach]);
		anchor = reach;
	}
	return result;
}

std::vector<glm::vec2> creature_route::RoundCorners(const std::vector<glm::vec2>& points, float radius, float maxStepAngle)
{
	if (points.size() <= 2 || radius <= 0.0f || maxStepAngle <= 0.0f)
	{
		return points;
	}
	std::vector<glm::vec2> result {points.front()};
	for (size_t i = 1; i + 1 < points.size(); ++i)
	{
		const auto before = points[i] - points[i - 1];
		const auto after = points[i + 1] - points[i];
		const auto beforeLength = glm::length(before);
		const auto afterLength = glm::length(after);
		if (beforeLength <= k_Tiny || afterLength <= k_Tiny)
		{
			continue;
		}
		const auto in = before / beforeLength;
		const auto out = after / afterLength;
		const auto turn = std::acos(std::clamp(glm::dot(in, out), -1.0f, 1.0f));
		if (turn <= k_Tiny || turn >= k_Pi - k_Tiny)
		{
			result.push_back(points[i]);
			continue;
		}
		// Each arc may use up to half of the segments either side of its corner
		const auto halfTan = std::tan(turn / 2.0f);
		const auto tangent = std::min(radius * halfTan, 0.5f * std::min(beforeLength, afterLength));
		const auto arcRadius = tangent / halfTan;
		const auto enter = points[i] - (in * tangent);
		const float direction = Cross(in, out) > 0.0f ? 1.0f : -1.0f;
		const glm::vec2 inward = direction * glm::vec2(-in.y, in.x);
		const auto centre = enter + (inward * arcRadius);
		const auto steps = std::max(static_cast<int32_t>(std::ceil(turn / maxStepAngle)), 1);
		for (int32_t k = 0; k <= steps; ++k)
		{
			const auto angle = direction * turn * static_cast<float>(k) / static_cast<float>(steps);
			result.push_back(centre + Rotate(enter - centre, angle));
		}
	}
	result.push_back(points.back());
	return result;
}

float Route::SegmentLength() const
{
	return Finished() ? 0.0f : glm::distance(points[segment], points[segment + 1]);
}

float Route::Remaining() const
{
	return std::max(SegmentLength() - travelled, 0.0f);
}

float Route::RemainingTotal() const
{
	auto total = Remaining();
	for (auto i = segment + 1; i + 1 < points.size(); ++i)
	{
		total += glm::distance(points[i], points[i + 1]);
	}
	return total;
}

glm::vec2 Route::Position() const
{
	if (points.empty())
	{
		return {};
	}
	if (Finished())
	{
		return points.back();
	}
	const auto length = SegmentLength();
	return length > 0.0f ? points[segment] + ((points[segment + 1] - points[segment]) * (travelled / length)) : points[segment];
}

float Route::Heading() const
{
	if (points.size() < 2)
	{
		return 0.0f;
	}
	const auto from = std::min(segment, points.size() - 2);
	return HeadingAlong(points[from], points[from + 1]);
}

float Route::TurnAtEnd() const
{
	if (OnLastSegment() || Finished())
	{
		return 0.0f;
	}
	const auto here = Heading();
	const auto next = HeadingAlong(points[segment + 1], points[segment + 2]);
	auto turn = std::fmod(next - here + k_Pi, 2.0f * k_Pi);
	if (turn < 0.0f)
	{
		turn += 2.0f * k_Pi;
	}
	return turn - k_Pi;
}

Advanced creature_route::Advance(Route& route, float distance)
{
	bool changed = false;
	while (!route.Finished())
	{
		const auto length = route.SegmentLength();
		if (route.travelled + distance < length)
		{
			route.travelled += distance;
			break;
		}
		distance -= length - route.travelled;
		++route.segment;
		route.travelled = 0.0f;
		changed = true;
	}
	return {.position = route.Position(), .heading = route.Heading(), .segmentChanged = changed, .finished = route.Finished()};
}
