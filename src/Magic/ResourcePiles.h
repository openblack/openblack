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

#include <glm/vec2.hpp>

#include "Enums.h"

/// The piles of food and wood: how far each rises out of the ground for what it holds, how it eases there, how its grain
/// flows, where poured food and wood go, and the thud each makes. Pure rules, which the miracles' world carries out.
namespace openblack::magic::piles
{

/// A pile holding anything at all shows at least this share of what it holds of its fill
inline constexpr float k_LeastShown = 0.05f;

/// How much of its height a pile shows above the ground for what it holds of what it is drawn full at. Food rises fast
/// at first and slows; wood rises evenly. Neither rises beyond full, however much more it holds.
[[nodiscard]] float ProportionRaised(ResourceType type, uint32_t amount, uint32_t fullAmount);

/// How far under the ground a pile sits for the share of its height it shows
[[nodiscard]] float SunkOffset(float proportion, float height);

/// A pile easing to how far under the ground it sits, from where and how fast it was, over a second of game time: its
/// offset follows a polynomial in time that ends at the target, still and without acceleration
struct Rise
{
	float start {0.0f};
	float speed {0.0f};
	/// The polynomial's terms after the speed: acceleration, jerk and snap
	float acceleration {0.0f};
	float jerk {0.0f};
	float snap {0.0f};
	float target {0.0f};
	float time {0.0f};
	float duration {0.0f};
	/// Where it is and how fast it moves now
	float offset {0.0f};
	float currentSpeed {0.0f};
};

/// A pile made fully under the ground, still
[[nodiscard]] Rise SunkRise(float height);
/// Eases from where the pile is now, at the speed it moves, to a new offset
void RiseTo(Rise& rise, float target);
/// Some seconds of game time on
void StepRise(Rise& rise, float seconds);
/// Whether any of it shows above the ground
[[nodiscard]] bool Shown(const Rise& rise, float height);

/// How far the texture of a pile of grain has flowed down it, by how far the pile is sunk: a quarter of the texture
/// when fully under the ground, none when fully up
inline constexpr float k_GrainFlowSpan = 0.25f;
[[nodiscard]] float GrainFlow(float offset, float height);
/// The piles whose grain flows: a storage pit's food and the food a miracle makes
[[nodiscard]] bool GrainFlows(PotInfo type);

/// The thud of a pile made or added to, a sample of the in-game bank: a big one for 200 or more, otherwise a small
/// one, picked by the tick count
inline constexpr uint32_t k_BigThud = 200;
[[nodiscard]] uint32_t PileSoundSample(ResourceType type, uint32_t amount, uint32_t tick);
/// The piles in the hand make no thud when added to
[[nodiscard]] bool Thuds(PotInfo type);

/// Food and wood poured at a point go first to what takes them in the map cell under it, then in the cells about it,
/// spiralling out: these are the nine cells, from the point's own, as whole cells across and along
inline constexpr float k_CellSize = 10.0f;
inline constexpr std::array<glm::ivec2, 9> k_SearchCells = {{
    {0, 0},
    {-1, 0},
    {-1, -1},
    {0, -1},
    {1, -1},
    {1, 0},
    {1, 1},
    {0, 1},
    {-1, 1},
}};
/// The map cell a point lies in
[[nodiscard]] glm::ivec2 CellOf(glm::vec2 xz);
/// How far out a store or pile takes what is poured: its radius on the ground times this, more for a pot or pile
inline constexpr float k_PotReachMultiplier = 2.0f;
inline constexpr float k_StoreReachMultiplier = 1.2f;
/// A model's radius on the ground: the larger of its half width and half depth, scaled. A pile of food's shrinks with
/// how much of it is raised.
[[nodiscard]] float RadiusOnGround(glm::vec2 halfExtent, float scale);

/// A pile that leads on to another of the same store holds no more than it is drawn full at; the last of a store's and
/// the miracles' piles hold all they are given
[[nodiscard]] bool IsCapped(PotInfo nextPotForResource);
/// What a pile takes of what it is given
[[nodiscard]] uint32_t AmountTaken(uint32_t holds, uint32_t given, uint32_t maximum, bool capped);

/// The models a miracle's piles are drawn at: food three tenths of its size, wood seven
inline constexpr float k_MagicFoodScale = 0.3f;
inline constexpr float k_MagicWoodScale = 0.7f;

} // namespace openblack::magic::piles
