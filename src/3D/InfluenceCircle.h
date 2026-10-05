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

#include <array>
#include <functional>
#include <optional>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

/// The border of a player's influence: a circle about each of their citadel and towns, drawn as a low curtain over the
/// land in the player's colour that scrolls along. Where one of a player's circles lies inside another's, it isn't
/// drawn.
namespace openblack::influence
{

/// The players' colours, 0xRRGGBB
inline constexpr std::array<uint32_t, 8> k_PlayerColours = {
    0xFF4646u, // red
    0x47FF54u, // green
    0xE347FFu, // magenta
    0x47F9FFu, // cyan
    0xFFFD47u, // yellow
    0x4664FFu, // blue
    0xFFA247u, // orange
    0xFFFFFFu, // white
};

/// The land's height at a point across the land
using Ground = std::function<float(glm::vec2)>;

/// A circle's curtain: a column of three vertices at the ground, 20 and 40 above it for each step round the circle and
/// one more closing it, with four triangles between each column and the next
struct Curtain
{
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uvs;
	std::vector<uint32_t> indices;
};
[[nodiscard]] Curtain MakeCurtain(const Ground& ground, const glm::vec3& centre, float radius);

/// One circle of a player's border
struct Circle
{
	PlayerNames player;
	glm::vec3 centre;
	float radius;
	Curtain curtain;
	/// The columns inside another circle of the player, not drawn
	std::vector<bool> hidden;
	/// Wholly inside another circle of the player: gone
	bool dead {false};

	[[nodiscard]] size_t Columns() const { return curtain.positions.size() / 3; }
};

/// A new circle about a centre, put in front of the others of the list: a circle wholly inside another of its player's
/// goes, and where two of them meet each hides its columns inside the other
void AddCircle(std::vector<Circle>& circles, PlayerNames player, const glm::vec3& centre, float radius, const Ground& ground);

/// How strongly the border shows, of 255, for the camera's height: none at all up to 100, coming in to 120 at 200
[[nodiscard]] std::optional<uint8_t> CurtainAlpha(float cameraHeight);
/// How far the curtain's texture has scrolled, from a clock of game milliseconds that wraps every ten seconds
[[nodiscard]] glm::vec2 ScrollOffset(int32_t milliseconds);
inline constexpr int32_t k_ScrollWrap = 10000;

/// The ripple a hand makes crossing a player's border: seven puffs of smoke in the plane of the border, upright along
/// it, growing out and fading over two seconds, in the player's colour
struct Ripple
{
	static constexpr size_t k_Puffs = 7;
	static constexpr float k_Life = 2000.0f;

	glm::vec3 point {0.0f};
	/// Milliseconds of game time left
	float life {k_Life};
	uint32_t rgb {0};
	/// Along the border: its tangent across the ground, and up
	glm::vec3 along {1.0f, 0.0f, 0.0f};
	std::array<float, k_Puffs> sizes {};
	std::array<float, k_Puffs> angles {};
	/// Each puff's opacity, of 255, as it last moved on
	std::array<uint8_t, k_Puffs> alphas {};
};

/// Draws a number between two, as the game's random numbers for the ripples
using Random = std::function<float(float, float)>;

/// Whether a point is inside a circle, across the ground
[[nodiscard]] bool Inside(const glm::vec3& centre, float radius, const glm::vec3& point);
/// Where a circle's edge lies between a point inside it and one outside, by halving the way between them
[[nodiscard]] glm::vec3 CrossingPoint(const glm::vec3& centre, float radius, glm::vec3 inside, glm::vec3 outside);
/// A ripple where the hand crossed a circle, on the land or the hand's height, whichever is higher
[[nodiscard]] Ripple MakeRipple(const Circle& circle, const glm::vec3& previous, const glm::vec3& hand, const Ground& ground,
                                const Random& random);
/// Moves a ripple on by some milliseconds of game time: its puffs grow, wrap round and fade, and it ages. Returns
/// whether anything is left of it.
[[nodiscard]] bool AdvanceRipple(Ripple& ripple, float milliseconds);
/// A ripple puff's four corners: a square of its size in the border's plane, turned by its angle
[[nodiscard]] std::array<glm::vec3, 4> PuffCorners(const Ripple& ripple, size_t puff);

/// How much a place a distance from an influence's centre is in it, for a radius: whole up to a part of the radius,
/// falling from a little less to nothing over the next part, then from a fifth to nothing out to the radius
[[nodiscard]] float OnRange(float distance, float radius, float fullPart, float fallingPart, float small);

} // namespace openblack::influence
