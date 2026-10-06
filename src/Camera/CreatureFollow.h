/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <numbers>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Editor/EditorMath.h"

/// The maths of Creature Mode's camera, which locks onto a creature and follows it, free of the game so it can be tested
/// on its own. The camera keeps a heading round the creature, a pitch above it and a distance from it, looking at the
/// middle of its body. Each frame it sets off afresh for where that puts it, arriving a second later (two as it starts),
/// so it eases after the creature as it moves.
///
/// The keys turn it: the cursor keys with Shift turn it round the creature and up and down, with Ctrl round it and in
/// and out, and the wheel draws it in and out. The cursor keys alone hand the camera back. Ctrl and Shift together
/// swing it round to where the land falls away, so the view of the creature is clear of the hills.
namespace openblack::creature_follow
{

/// The heading round the creature (from +z, as an orbit's yaw), the pitch above it and the distance from its middle
using View = editor::Orbit;

/// The camera keeps this far from the creature, and the keys bring it no further out than the second
constexpr float k_MinDistance = 2.0f;
constexpr float k_MaxDistance = 1500.0f;
constexpr float k_MaxKeyDistance = 1000.0f;
/// It looks down on the creature at least this steeply, about 14 degrees
constexpr float k_MinPitch = 0.2416609824f;
/// Shift and the cursor keys tilt it between these, though it never goes lower than the least pitch above
constexpr float k_MinKeyPitch = -std::numbers::pi_v<float> / 4.0f;
constexpr float k_MaxKeyPitch = 1.3744468689f;
/// It starts this many of the creature's heights away, unless it is close to the creature already
constexpr float k_ViewingDistancePerHeight = 8.0f;
/// The camera counts as close when it is this near the creature across the land
constexpr float k_CloseAcross = 80.0f;
/// How long the camera takes to arrive where it sets off for: two seconds as it starts, down to one over two seconds
constexpr float k_StartEaseSeconds = 2.0f;
constexpr float k_EaseSeconds = 1.0f;
constexpr float k_EaseSettleSeconds = 2.0f;
/// The cursor keys move it as a mouse would move this many pixels a second
constexpr float k_KeyPixelsPerSecond = 400.0f;
/// A notch of the wheel, as the mouse counts it
constexpr float k_WheelNotch = 120.0f;

/// The middle of a creature of a height standing at a position, which the camera looks at
[[nodiscard]] glm::vec3 Focus(glm::vec3 position, float height);
/// How far from a creature of a height the camera starts
[[nodiscard]] float ViewingDistance(float height);

/// The heading and pitch of a point seen from a centre, as the camera keeps them; straight above it is looked down on
/// with no heading
struct HeadingPitch
{
	float heading {0.0f};
	float pitch {0.0f};
};
[[nodiscard]] HeadingPitch HeadingPitchOf(glm::vec3 point, glm::vec3 centre);

/// The view the camera starts with on a creature: as it is turned and tilted now, and the creature's viewing distance
/// away, or as far as it already is when it is close, but no nearer than twice the creature's height
[[nodiscard]] View Start(glm::vec3 cameraOrigin, glm::vec3 cameraFocus, glm::vec3 creature, float height);
/// The view kept within the follow's bounds: the distance between the least and the most, and no lower than the least
/// pitch
[[nodiscard]] View Clamped(View view);
/// How long the camera takes to arrive, some seconds after Creature Mode starts
[[nodiscard]] float EaseSeconds(float secondsInMode);
/// Where the camera stands for a view of a focus
[[nodiscard]] glm::vec3 Origin(glm::vec3 focus, const View& view);

/// The keys held this frame: across is the left and right cursor keys and along the up and down, each as pixels the
/// mouse would have moved (left and up negative); the wheel is in the mouse's counts, negative to draw the camera out
struct Keys
{
	float across {0.0f};
	float along {0.0f};
	float wheel {0.0f};
	bool shift {false};
	bool ctrl {false};

	[[nodiscard]] bool Moving() const { return across != 0.0f || along != 0.0f; }
};
/// The cursor keys' pixels for a frame of some seconds: -1, 0 or 1 a way
[[nodiscard]] Keys KeysFor(int across, int along, float seconds);

enum class KeyOutcome : uint8_t
{
	Stay,
	/// The cursor keys were pressed with neither Shift nor Ctrl: the player takes the camera back
	Leave,
};
/// Turns the view by the keys on a screen so many pixels wide. The wheel's zoom is applied twice, before and after the
/// cursor keys, as the game does.
[[nodiscard]] KeyOutcome Apply(View& view, const Keys& keys, float screenWidth);

/// The height of the land at a point on it, x and z
using GroundHeight = std::function<float(glm::vec2)>;
/// The distance the camera looks for a clear view at: nearer ones are taken most of the way out to 50 and further ones a
/// little way in towards 100
[[nodiscard]] float ClearingDistance(float distance);
/// The heading, of 32 round the creature, along which the land falls away furthest below the creature's middle out to a
/// distance, favouring headings near the one the camera has
[[nodiscard]] float ClearHeading(float heading, float distance, glm::vec3 focus, const GroundHeight& ground);
/// The pitch of the land's slope under the creature, from its normal; on level land, nearly straight up
[[nodiscard]] float SlopePitch(glm::vec3 normal);
/// The pitch the camera tilts to for a clear view: mostly a steady 22 degrees or so, a little of the pitch it had and a
/// little of the slope, between 22.5 and 60 degrees
[[nodiscard]] float ClearPitch(float pitch, float slopePitch);
/// The view swung round and tilted for a clear view of the creature, keeping its distance
[[nodiscard]] View ClearView(View view, glm::vec3 focus, glm::vec3 landNormal, const GroundHeight& ground);

} // namespace openblack::creature_follow
