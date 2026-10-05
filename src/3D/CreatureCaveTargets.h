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
#include <optional>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack
{

/// What the cursor can be over in the creature's room, which the room's camera finds each frame by where on the screen
/// a point of each is, and what clicking it zooms the camera to
namespace CreatureCaveTargets
{

enum class Target : uint8_t
{
	/// The creature, which opens the tattoo editor
	Creature,
	/// The belts it has won, on the attack dummies
	Belts,
	/// Its medals for the miracles it has learnt, on the magic plinths
	Medals,
	/// A fourth place, whose tooltip says it zooms in but which a click does nothing for
	Fourth,
	/// The way out of the temple at the cave's far end
	Exit,

	_Count
};
constexpr size_t k_Count = static_cast<size_t>(Target::_Count);

/// Where on the screen each target is found, by how far from its point across and down, in pixels. The creature
/// has no reach but its point: it is found by picking its mesh.
constexpr std::array<glm::ivec2, k_Count> k_Reach {{{0, 0}, {45, 60}, {65, 35}, {65, 35}, {40, 50}}};
/// The creature's point is found this far further down the screen
constexpr int32_t k_CreatureDrop = 10;

/// Where the camera looks at the exit from, and the place it looks at, which is also the exit's place on the screen
constexpr glm::vec3 k_ExitEye {207.0f, 2.0f, -11.0f};
constexpr glm::vec3 k_ExitPlace {229.0f, 2.0f, -40.0f};

/// The points of the creature's room's mesh the camera looks at a target from, and looks at, when zoomed to it: 44 and
/// 49 onwards. Each but the exit is found on the screen by the point looked at (points 49 to 52).
constexpr uint32_t k_FirstEyePoint = 44;
constexpr uint32_t k_FirstLookPoint = 49;

/// The height of the screen the game's reach is in pixels of: the interface is laid out for 800 by 600
constexpr float k_ReachScreenHeight = 600.0f;

/// The target the mouse is over, of those on the screen at their places (in pixels, down from the top), the last of
/// them it is within reach of. The game's reach is in pixels whatever the screen; here it is scaled to the screen's
/// height, as it would be on the 800 by 600 screen the interface is laid out for, so that the targets are as easy to
/// find on larger screens.
[[nodiscard]] std::optional<Target> TargetAt(const std::array<std::optional<glm::vec2>, k_Count>& places, glm::vec2 mouse,
                                             float screenHeight = k_ReachScreenHeight);

} // namespace CreatureCaveTargets

} // namespace openblack
