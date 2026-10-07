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

#include <functional>
#include <optional>
#include <span>

#include <glm/vec3.hpp>

#include "3D/MapCoords.h"

/// The god hand shows its player's alignment. Its base mesh is pulled towards its evil mesh (alignment below 0) or its
/// good mesh (0 and above) by how far the alignment is from 0: the vertices and normals in the vertex shader, and its
/// skin blended a 4-bit channel at a time, alpha too, in whole steps. With the game's meshes the good hand only
/// changes colour, to gold, and the evil hand turns red with ridged, spiked fingers.
///
/// The hand is not eased: each frame, once the player's alignment is k_RefreshThreshold or more away from the one it
/// is drawn at, it jumps straight to it. It starts neutral. Its skin is also blended again as it goes into or out of its
/// player's influence (see Advance).
namespace openblack::hand_morph
{
/// How far the alignment must move from the one the hand is drawn at before the hand is drawn anew
constexpr float k_RefreshThreshold = 0.03f;

/// The meshes the base is pulled towards
enum class Look : uint8_t
{
	Evil,
	Good,
};

/// The alignment the hand is to show, held between -1 (evil) and 1 (good)
[[nodiscard]] float Target(float playerAlignment);

/// The alignment the hand is drawn at once it has caught up with target, or none while it is near enough
[[nodiscard]] std::optional<float> Refresh(float drawn, float target);

/// The hand's morph between frames
struct State
{
	/// The alignment it last took from its player, held to its range
	float target {0.0f};
	/// The alignment its shape is drawn at
	float drawn {0.0f};
	/// Whether it was last over land in its player's influence. It starts as if it were.
	bool inInfluence {true};
};

/// What a frame changes: the alignment the skin is blended anew for, and whether the shape is drawn anew at drawn
struct Change
{
	std::optional<float> skin;
	bool shape {false};
};

/// A frame of the hand. Going into or out of its player's influence (none when that isn't known this frame) blends the
/// skin again for the alignment taken the frame before, so the skin can show an alignment up to k_RefreshThreshold
/// away from the shape's. Then the frame's alignment is taken, and once far enough from the shape's the skin and the
/// shape both catch up with it.
[[nodiscard]] Change Advance(State& state, float playerAlignment, std::optional<bool> inInfluence);

/// Whether the hand is in its player's influence is tested at the point the interface picks under the cursor. It is
/// taken while the frame is drawn and tested after it, so a frame's hand goes by the point picked the frame before.
///
/// The point is where the ray from the camera through the cursor meets the land, or else the sea's level where the ray
/// points down. It is kept within k_PickReach of k_PickMiddle, counting its height, the land's there.
constexpr glm::vec3 k_PickMiddle {2560.0f, 0.0f, 2560.0f};
constexpr float k_PickReach = 7680.0f;

/// The point picked for a ray (origin and direction) that meets the land at landHit, if it does, made a map position;
/// none when it misses the land and doesn't point down. heightAt gives the land's height at a map position.
[[nodiscard]] std::optional<map_coords::MapCoords> Pick(std::optional<glm::vec3> landHit, glm::vec3 origin, glm::vec3 direction,
                                                        const std::function<float(const map_coords::MapCoords&)>& heightAt);

/// The point tested after a frame: the one picked, or else the map's origin. While held, as while the hand grips the
/// land, a frame that picks nothing keeps the last point instead.
[[nodiscard]] map_coords::MapCoords NextPoint(const map_coords::MapCoords& last, std::optional<map_coords::MapCoords> picked,
                                              bool held);

/// The mesh the hand drawn at an alignment is pulled towards, and how far, 0 to 1
[[nodiscard]] Look LookOf(float drawn);
[[nodiscard]] float Weight(float drawn);

/// The hand's skin drawn at an alignment: the base skin blended towards the evil or good one (look), each texel of 4
/// bits a channel
void BlendSkin(std::span<const uint16_t> base, std::span<const uint16_t> look, float drawn, std::span<uint16_t> blended);
} // namespace openblack::hand_morph
