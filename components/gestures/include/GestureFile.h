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
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

/// The gesture templates file (Data/Gestures.jty): the shapes the hand's drawn gestures are matched against.
///
/// The file is a count of templates followed by that many fixed-size records. Each record holds room for 80 points, of
/// which the first few are the template's key points (its corners, its start and its end), followed by seven 32-bit
/// fields: the number of points, the gesture the template draws, how the gesture's position is taken, whether the
/// template's first direction must match, whether it may be drawn mirrored, whether its aspect ratio must match, and the
/// aspect ratio. The game reads those three first fields a byte apart, so only their low bytes count; the rest of each
/// is kept as it was so a file is written back exactly as it was read.
namespace openblack::gestures
{

enum class GestureFileResult : uint8_t
{
	Success = 0,
	ErrCantOpen,
	ErrFileTooSmall,
	ErrSizeMismatch,
	ErrTooManyPoints,
};

[[nodiscard]] std::string_view ResultToStr(GestureFileResult result);

/// A key point of a template, in a box from 0 to 1 with x to the right and z down the screen
struct TemplatePoint
{
	float x;
	/// Always 0: the points lie in the screen's plane
	float y;
	float z;
	/// The signed turn at the point, in radians, from the way the path came in to the way it goes out
	float turn;
	/// The way the path leaves the point, in eighths of a turn: 0 up the screen, 2 right, 4 down, 6 left
	uint32_t direction;
};
static_assert(sizeof(TemplatePoint) == 20);

/// How a recognised gesture's position on the land is taken
enum class PositionMode : uint8_t
{
	/// Where the gesture started
	Start = 0,
	/// The middle of the box round the gesture, with its size from the box (every template in the game's file)
	Centre = 2,
};

struct GestureTemplate
{
	static constexpr size_t k_MaxPoints = 80;

	std::array<TemplatePoint, k_MaxPoints> points {};
	/// The fields as stored; only the low byte of the first three is used
	uint32_t pointCountField {0};
	uint32_t gestureField {0};
	uint32_t positionModeField {0};
	uint32_t checkDirection {0};
	uint32_t allowMirror {0};
	uint32_t checkAspectRatio {0};
	/// Width over height of the template as drawn
	float aspectRatio {1.0f};

	[[nodiscard]] size_t PointCount() const;
	[[nodiscard]] std::span<const TemplatePoint> Points() const;
	/// The gesture it draws (the game's gesture numbers: 1 spiral, 4 circle, 5 scribble...)
	[[nodiscard]] uint8_t Gesture() const { return static_cast<uint8_t>(gestureField & 0xFFu); }
	[[nodiscard]] uint8_t PositionModeValue() const { return static_cast<uint8_t>(positionModeField & 0xFFu); }
	[[nodiscard]] bool ChecksDirection() const { return checkDirection != 0; }
	[[nodiscard]] bool AllowsMirror() const { return allowMirror != 0; }
	[[nodiscard]] bool ChecksAspectRatio() const { return checkAspectRatio != 0; }
};

class GestureFile
{
public:
	/// The size of one template's record in the file
	static constexpr size_t k_RecordSize = (GestureTemplate::k_MaxPoints * sizeof(TemplatePoint)) + (7 * sizeof(uint32_t));

	[[nodiscard]] GestureFileResult Open(const std::filesystem::path& path);
	[[nodiscard]] GestureFileResult Open(std::span<const uint8_t> bytes);
	/// The file's bytes, as the game would write them
	[[nodiscard]] std::vector<uint8_t> Write() const;
	[[nodiscard]] GestureFileResult Write(const std::filesystem::path& path) const;

	[[nodiscard]] const std::vector<GestureTemplate>& GetTemplates() const { return _templates; }
	void AddTemplate(const GestureTemplate& gestureTemplate) { _templates.push_back(gestureTemplate); }

private:
	std::vector<GestureTemplate> _templates;
};

} // namespace openblack::gestures
