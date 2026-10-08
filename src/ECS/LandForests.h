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

#include <optional>
#include <span>

/// The rules the land's forests are chosen and ranked by
namespace openblack::ecs::land_forests
{

/// A forest's worth as a home for a flock, by its distance in the map's whole units. Only the distance counts (how many
/// trees it has is weighed and then thrown away), and anything further than a few centimetres scores the same.
[[nodiscard]] float LairScore(int32_t distance);

/// Which of the forests, met in order, a flock makes its home by: the first is taken without a score, then each one that
/// scores strictly more than the best score so far (which starts at 0) replaces it. With every forest scoring the same,
/// the second forest wins.
[[nodiscard]] std::optional<size_t> LairForest(std::span<const float> scores);

/// The nearest, met in order, by whole distances: only a strictly nearer one replaces the one before
[[nodiscard]] std::optional<size_t> Nearest(std::span<const int32_t> distances);

/// The forests within a town's reach: their nearest point strictly nearer than the reach, and their wood not nothing
[[nodiscard]] bool NearTown(float distance, float reach, float wood);

} // namespace openblack::ecs::land_forests
