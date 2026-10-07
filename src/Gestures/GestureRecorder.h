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

#include <array>
#include <numbers>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// The hand's path as the game records it for its gestures. The cursor is sampled about 34 times a second whether or
/// not a button is held, into a buffer of the last 80 points. As each point comes in, the path's corners are found: the
/// point between the last corner and the newest where the path turns most, if it turns enough, merged into the corner
/// before it when the two are too close. The gesture matcher then compares the turns at those corners with its
/// templates'. Points are on the screen, x to the right and y down, in pixels of a screen 768 pixels high, whatever the
/// window's size, so a gesture is drawn the same at any resolution.
namespace openblack::gesture
{

/// The points kept
constexpr size_t k_Capacity = 80;
/// The cursor is sampled once more than this much time has passed since the last sample
constexpr float k_SampleSeconds = 0.029f;
/// The height of the screen the path is measured on; the game measured it in the pixels of whatever screen it ran on
constexpr float k_ReferenceHeight = 768.0f;
/// The cursor held still for this many samples in a row (about two seconds) forgets the path
constexpr uint32_t k_StillSamples = 70;
/// Two points this close on both axes are the same place
constexpr float k_SamePlacePixels = 4.0f;
/// Corners this far apart on either axis are kept apart. Closer ones are merged, unless the path's last stretch is
/// small, when anything further than k_SamePlacePixels apart is kept.
constexpr float k_CornersApartPixels = 12.0f;
constexpr float k_SmallStretchPixels = 50.0f;
/// The last stretch reaches at least this many points back
constexpr int k_StretchPoints = 8;
/// The least turn that makes a corner: three thirty-seconds of a half turn, about 17 degrees
constexpr float k_CornerTurn = std::numbers::pi_v<float> * 3.0f / 32.0f;

/// What a recorded point is to the path
enum class KeyType : uint8_t
{
	None = 0,
	/// The oldest point kept
	Start = 1,
	/// A corner
	Corner = 2,
	/// The point the last corner was found from, where the next search starts
	SearchFrom = 4,
	/// The newest point
	End = 8,
};

/// One recorded point
struct RecordedPoint
{
	glm::vec2 screen {0.0f};
	/// The land under it, for where a gesture is on the land
	glm::vec3 world {0.0f};
	/// At a corner: the signed turn from the way the path came in to the way it leaves, in radians
	float turn {0.0f};
	/// At a corner: the way the path leaves it, in eighths of a turn, 0 up the screen, 2 right, 4 down, 6 left
	uint8_t direction {0};
	/// At a corner: the angle the path leaves it at, from the screen's x axis towards its y axis, 0 to two pi
	float heading {0.0f};
	KeyType key {KeyType::None};
};

/// A point the matcher compares: the start, each corner and the end
struct KeyPoint
{
	glm::vec2 screen {0.0f};
	float turn {0.0f};
	uint8_t direction {0};
};

/// A box on the screen round some points
struct ScreenBox
{
	glm::vec2 min {0.0f};
	glm::vec2 max {0.0f};

	/// Its size counting the pixels at both ends, as the game measures it
	[[nodiscard]] glm::vec2 Size() const { return max - min + glm::vec2(1.0f); }
	[[nodiscard]] glm::vec2 Centre() const { return (min + max) * 0.5f; }
};

/// The angle of a step on the screen, from the x axis towards y, 0 to two pi
[[nodiscard]] float HeadingOf(glm::vec2 step);
/// The signed turn from one heading to another, -pi to pi
[[nodiscard]] float TurnBetween(float from, float to);
/// A heading in eighths of a turn: 0 up the screen, 2 right, 4 down, 6 left
[[nodiscard]] uint8_t DirectionOf(float heading);

class GestureRecorder
{
public:
	/// Forgets the path
	void Reset();
	/// Adds a sampled point
	void AddPoint(glm::vec2 screen, glm::vec3 world);
	/// Adds a sampled point over nothing, as over the sky: the land under the last point is kept
	void AddPoint(glm::vec2 screen);

	[[nodiscard]] size_t Count() const { return _count; }
	[[nodiscard]] bool Empty() const { return _count == 0; }
	/// A point by its place, the oldest first
	[[nodiscard]] const RecordedPoint& At(size_t index) const;
	/// The start, each corner and the end, in order
	[[nodiscard]] std::vector<KeyPoint> KeyPoints() const;
	/// The places of the points that are the key points numbered first and last, as KeyPoints numbers them
	struct PointRange
	{
		size_t first {0};
		size_t last {0};
	};
	[[nodiscard]] PointRange PointsOfKeys(size_t firstKey, size_t lastKey) const;
	/// The box round the points from one place up to, but not including, another, skipping points never set
	[[nodiscard]] ScreenBox BoxOf(size_t first, size_t end) const;

private:
	[[nodiscard]] RecordedPoint& Point(int index);
	[[nodiscard]] const RecordedPoint& Point(int index) const;
	[[nodiscard]] static bool IsKey(const RecordedPoint& point);

	void FindKeyPoints(int newest);
	[[nodiscard]] int PreviousKey(int index) const;
	[[nodiscard]] int PreviousCorner(int index) const;
	[[nodiscard]] int PreviousDistinct(int index) const;
	[[nodiscard]] int FindCorner(int from, int newest) const;
	[[nodiscard]] bool MergeCorner(int corner, int newest);
	[[nodiscard]] bool FarEnoughApart(int from, int to, const RecordedPoint& a, const RecordedPoint& b) const;
	void UpdateCornerAngles(int index);

	std::array<RecordedPoint, k_Capacity> _points {};
	/// How many points are kept, and where the next one goes
	size_t _count {0};
	size_t _next {0};
	uint32_t _still {0};
};

} // namespace openblack::gesture
