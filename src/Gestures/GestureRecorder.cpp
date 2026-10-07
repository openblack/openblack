/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GestureRecorder.h"

#include <cmath>

#include <algorithm>

#include <glm/common.hpp>

using namespace openblack::gesture;

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;
constexpr float k_TwoPi = 2.0f * k_Pi;
/// A point closer than this to the screen's corner on both axes was never set
constexpr float k_Unset = 1e-4f;

/// Key types the matcher compares: the start, the corners and the end
constexpr uint8_t k_MatchedKeys =
    static_cast<uint8_t>(KeyType::Start) | static_cast<uint8_t>(KeyType::Corner) | static_cast<uint8_t>(KeyType::End);

bool IsSet(const RecordedPoint& point)
{
	return std::abs(point.screen.x) > k_Unset || std::abs(point.screen.y) > k_Unset;
}

/// Whether two points are in different places, more than k_SamePlacePixels apart on an axis
bool Apart(const RecordedPoint& a, const RecordedPoint& b)
{
	const auto step = b.screen - a.screen;
	return !(std::abs(step.x) < k_SamePlacePixels && std::abs(step.y) < k_SamePlacePixels);
}

/// The turn the path makes at b, coming from a and going to c
float TurnAt(const RecordedPoint& a, const RecordedPoint& b, const RecordedPoint& c)
{
	return TurnBetween(HeadingOf(b.screen - a.screen), HeadingOf(c.screen - b.screen));
}
} // namespace

float openblack::gesture::HeadingOf(glm::vec2 step)
{
	auto angle = std::atan2(step.y, step.x);
	if (angle < 0.0f)
	{
		angle += k_TwoPi;
	}
	return angle;
}

float openblack::gesture::TurnBetween(float from, float to)
{
	const auto turn = to - from;
	if (turn > k_Pi)
	{
		return turn - k_TwoPi;
	}
	if (turn < -k_Pi)
	{
		return turn + k_TwoPi;
	}
	return turn;
}

uint8_t openblack::gesture::DirectionOf(float heading)
{
	// A quarter turn on, so that up the screen is 0, then rounded to the nearest eighth
	auto angle = heading + (k_Pi / 2.0f);
	while (angle > k_TwoPi)
	{
		angle -= k_TwoPi;
	}
	const auto eighths = angle * 8.0f / k_TwoPi;
	auto rounded = static_cast<int>(eighths);
	if (eighths - static_cast<float>(rounded) > 0.5f)
	{
		++rounded;
	}
	return static_cast<uint8_t>(rounded % 8);
}

void GestureRecorder::Reset()
{
	_points = {};
	_count = 0;
	_next = 0;
	_still = 0;
}

RecordedPoint& GestureRecorder::Point(int index)
{
	if (static_cast<int>(_count) < index)
	{
		return _points.front();
	}
	return _points.at((_next + k_Capacity - _count + static_cast<size_t>(index)) % k_Capacity);
}

const RecordedPoint& GestureRecorder::Point(int index) const
{
	if (static_cast<int>(_count) < index)
	{
		return _points.front();
	}
	return _points.at((_next + k_Capacity - _count + static_cast<size_t>(index)) % k_Capacity);
}

const RecordedPoint& GestureRecorder::At(size_t index) const
{
	return Point(static_cast<int>(index));
}

bool GestureRecorder::IsKey(const RecordedPoint& point)
{
	return (static_cast<uint8_t>(point.key) & k_MatchedKeys) != 0;
}

void GestureRecorder::AddPoint(glm::vec2 screen)
{
	// Over nothing, the land under the last point stands in; with no path yet there is nothing to go on
	if (_count != 0)
	{
		AddPoint(screen, At(_count - 1).world);
	}
}

void GestureRecorder::AddPoint(glm::vec2 screen, glm::vec3 world)
{
	while (true)
	{
		// The new point takes the oldest's place once the buffer is full, keeping what the oldest had found
		auto& point = _points.at(_next);
		point.screen = screen;
		point.world = world;
		_count = std::min(_count + 1, k_Capacity);
		if (_count < 2)
		{
			break;
		}
		if (Point(static_cast<int>(_count) - 2).screen != screen)
		{
			_still = 0;
			break;
		}
		// Held still for long enough, the path is forgotten and starts again from here
		if (++_still < k_StillSamples)
		{
			break;
		}
		Reset();
	}
	_next = (_next + 1) % k_Capacity;
	FindKeyPoints(static_cast<int>(_count) - 1);
}

int GestureRecorder::PreviousKey(int index) const
{
	for (auto i = index - 1; i > 0; --i)
	{
		if (Point(i).key != KeyType::None)
		{
			return i;
		}
	}
	return 0;
}

int GestureRecorder::PreviousCorner(int index) const
{
	for (auto i = index - 1; i > 0; --i)
	{
		if (Point(i).key == KeyType::Corner)
		{
			return i;
		}
	}
	return 0;
}

int GestureRecorder::PreviousDistinct(int index) const
{
	const auto& newest = Point(index);
	for (auto i = index - 1; i > 0; --i)
	{
		const auto& point = Point(i);
		if (point.key != KeyType::None || (IsSet(point) && Apart(point, newest)))
		{
			return i;
		}
	}
	return 0;
}

int GestureRecorder::FindCorner(int from, int newest) const
{
	if (newest <= 1)
	{
		return 0;
	}
	const auto& end = Point(newest);
	const auto& start = Point(from);
	// The point between the two where the path turns most, if it turns enough, walking back while the points are
	// apart from the start
	float sharpest = 0.0f;
	int corner = 0;
	for (auto i = PreviousDistinct(newest); from < i; --i)
	{
		const auto& point = Point(i);
		if (!Apart(start, point))
		{
			break;
		}
		const auto turn = std::abs(TurnAt(start, point, end));
		if (turn >= k_CornerTurn && turn > sharpest)
		{
			sharpest = turn;
			corner = i;
		}
	}
	if (corner != 0)
	{
		return corner;
	}
	// Else the start itself, if the path turns enough there on its way to the newest point
	if (from != 0 && std::abs(TurnAt(Point(PreviousKey(from)), start, end)) >= k_CornerTurn)
	{
		return from;
	}
	return 0;
}

bool GestureRecorder::FarEnoughApart(int from, int to, const RecordedPoint& a, const RecordedPoint& b) const
{
	const auto step = glm::abs(b.screen - a.screen);
	const auto apart = std::max(step.x, step.y);
	if (apart >= k_CornersApartPixels)
	{
		return true;
	}
	if (apart > k_SamePlacePixels)
	{
		// A small path is drawn with small strokes: its corners may be closer
		const auto stretchFrom = std::min(from == 0 ? 0 : PreviousCorner(from), std::max(to - k_StretchPoints, 0));
		const auto size = BoxOf(static_cast<size_t>(stretchFrom), static_cast<size_t>(to)).Size();
		if (std::max(size.x, size.y) < k_SmallStretchPixels)
		{
			return true;
		}
	}
	return false;
}

bool GestureRecorder::MergeCorner(int corner, int newest)
{
	const auto previous = PreviousCorner(corner);
	auto& point = Point(corner);
	if (previous == 0)
	{
		// Too near the start, the corner is the start
		const auto& start = Point(0);
		if (!FarEnoughApart(0, corner, start, point))
		{
			point.screen = start.screen;
			point.world = start.world;
			return true;
		}
		return false;
	}
	auto& before = Point(previous);
	if (FarEnoughApart(previous, corner, before, point))
	{
		return false;
	}
	// Too near the corner before: the two become one, where the path turns more
	const auto& beforeThat = Point(PreviousCorner(previous));
	const auto& end = Point(newest);
	if (std::abs(TurnAt(beforeThat, before, end)) <= std::abs(TurnAt(beforeThat, point, end)))
	{
		before.screen = point.screen;
		before.world = point.world;
	}
	else
	{
		point.screen = before.screen;
		point.world = before.world;
	}
	return true;
}

void GestureRecorder::UpdateCornerAngles(int index)
{
	// The corner before the point now knows which way the path leaves it, and so how much it turns
	const auto corner = PreviousCorner(index);
	auto& point = Point(corner);
	point.heading = HeadingOf(Point(index).screen - point.screen);
	point.direction = DirectionOf(point.heading);
	if (corner != 0)
	{
		point.turn = TurnBetween(Point(PreviousCorner(corner)).heading, point.heading);
	}
}

void GestureRecorder::FindKeyPoints(int newest)
{
	if (newest != 0 && Point(newest - 1).key == KeyType::End)
	{
		Point(newest - 1).key = KeyType::None;
	}
	Point(0).key = KeyType::Start;
	Point(newest).key = KeyType::End;
	if (newest == 0)
	{
		return;
	}
	const auto corner = FindCorner(PreviousKey(newest), newest);
	if (corner != 0)
	{
		if (!MergeCorner(corner, newest))
		{
			Point(corner).key = KeyType::Corner;
			UpdateCornerAngles(corner);
		}
		Point(newest).key = KeyType::SearchFrom;
	}
	UpdateCornerAngles(newest);
}

std::vector<KeyPoint> GestureRecorder::KeyPoints() const
{
	std::vector<KeyPoint> keys;
	for (size_t i = 0; i < _count; ++i)
	{
		const auto& point = At(i);
		if (i + 1 >= _count || IsKey(point))
		{
			keys.push_back({.screen = point.screen, .turn = point.turn, .direction = point.direction});
		}
	}
	return keys;
}

GestureRecorder::PointRange GestureRecorder::PointsOfKeys(size_t firstKey, size_t lastKey) const
{
	PointRange range;
	size_t seen = 0;
	size_t i = 0;
	const auto isKey = [this](size_t index) { return index + 1 >= _count || IsKey(At(index)); };
	if (firstKey != 0)
	{
		for (; i < _count; ++i)
		{
			if (isKey(i) && firstKey < ++seen)
			{
				range.first = i++;
				break;
			}
		}
	}
	for (; i < _count; ++i)
	{
		if (isKey(i) && lastKey < ++seen)
		{
			range.last = i;
			break;
		}
	}
	return range;
}

ScreenBox GestureRecorder::BoxOf(size_t first, size_t end) const
{
	const auto firstIndex = _count < first ? size_t {0} : (_next + k_Capacity - _count + first) % k_Capacity;
	const auto& start = _points.at(firstIndex);
	ScreenBox box {.min = start.screen, .max = start.screen};
	auto span = static_cast<int>(end) - static_cast<int>(first);
	if (static_cast<int>(_count) <= span)
	{
		span = static_cast<int>(_count);
	}
	if (span < 0)
	{
		span += static_cast<int>(k_Capacity);
	}
	auto index = firstIndex;
	for (int i = 1; i < span; ++i)
	{
		index = (index + 1) % k_Capacity;
		const auto& point = _points.at(index);
		if (IsSet(point))
		{
			box.min = glm::min(box.min, point.screen);
			box.max = glm::max(box.max, point.screen);
		}
	}
	return box;
}
