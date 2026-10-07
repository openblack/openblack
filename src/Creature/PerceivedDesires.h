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
#include <functional>
#include <optional>

#include <glm/vec2.hpp>

// What a creature thinks its player wants, from what it sees the player do: each of the player's miracles it can see
// shows the desires the miracle's table says it answers, a stroke shows compassion and a slap anger. What it thinks
// fades slowly, and the creature's scroll and the temple say what it thinks its god wants most. Pure, tested on its own.

namespace openblack::creature_perceived_desires
{

/// One for each of the creature's desires, and one for each of a town's
inline constexpr size_t k_PlayerDesires = 40;
inline constexpr size_t k_TownDesires = 17;
/// Each turn every one is multiplied by this
inline constexpr float k_TurnFade = 0.9995f;

struct PerceivedDesires
{
	std::array<float, k_PlayerDesires> player {};
	std::array<float, k_TownDesires> town {};
};

/// Seen to want a desire more, held between 0 and 1; a desire out of range is nothing
void Increase(PerceivedDesires& desires, size_t desire, float amount);
void IncreaseTown(PerceivedDesires& desires, size_t desire, float amount);
/// A turn: every one fades a little
void Fade(PerceivedDesires& desires);
/// The desire it thinks its player wants most: the last of those it feels itself (activated) that it thinks the player
/// wants at all, each of which it then forgets; none if there are none
[[nodiscard]] std::optional<size_t> TakeDominant(PerceivedDesires& desires, const std::function<bool(size_t)>& activated);

/// Whether a creature sees a point: within two thirds of a half turn either way of where it looks, by the game's angles
/// (2048 to a turn), or in its own map cell, however far
inline constexpr uint16_t k_SeeHalfAngle = 0x2AA;
[[nodiscard]] bool CanSeePos(uint16_t lookAngle, uint16_t angleToPoint, bool sameCell);

} // namespace openblack::creature_perceived_desires
