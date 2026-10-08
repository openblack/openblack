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

#include <optional>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Magic/MagicWorldInterface.h"
#include "Magic/SpellRules.h"

namespace openblack
{
struct GObjectInfo;
}

// The living things the miracles act on, in the game: villagers, animals and creatures. Their tables, their lives, who
// the heal may heal, and what a creature does with a miracle's effect beyond its life: in a fight a heal restores its
// stamina rather than its life, it is never hurt by its own miracle, a heal mends its cuts and scars, and a miracle from
// another creature changes how nice it finds that creature.

namespace openblack::ecs::systems::magic_living
{

/// The table row of a villager, animal or creature, none for anything else
[[nodiscard]] const GObjectInfo* InfoOf(entt::entity entity);
/// Its life, 0 to 1, none for what doesn't live
[[nodiscard]] std::optional<float> LifeOf(entt::entity entity);
void SetLife(entt::entity entity, float life);
/// A villager or animal with no life left; creatures faint rather than die of a miracle
[[nodiscard]] bool IsDead(entt::entity entity);
/// Whether the heal may heal it: a villager or animal while alive, a creature always, the doves of a flock never
[[nodiscard]] bool CanBeHealedByHeal(entt::entity entity);
/// Whether an animal kind is a dove, which no heal ever heals
[[nodiscard]] bool IsDove(AnimalInfo type);

/// The people a heal of a radius and most targets at a point heals, as the game finds them: once, through the land's
/// cells in a spiral from the point's (see Magic/HealTargets.h)
[[nodiscard]] std::vector<entt::entity> HealTargets(glm::vec3 point, float radius, size_t maximum);

/// The defence an object of the living takes effects with: its row's, a creature's lessened by its size
[[nodiscard]] magic::EffectDefence DefenceOf(entt::entity entity);

/// An effect about to reach a creature: whether the creature takes it its own way, so that its life is left alone. A
/// creature is never harmed by its own miracle. In a fight a miracle that does no harm reaches it only from itself or its
/// own player's hand; one that reaches it makes it reel, a heal gives back the health it fights with and harm takes it
/// (a tenth of either while it blocks), and its life is untouched. Out of a fight harm from another player's miracle
/// frightens and angers it, and it then takes the effect as anything else does.
[[nodiscard]] bool TakesEffectItsOwnWay(entt::entity entity, const magic::EffectValues& values,
                                        const magic::EffectSource& source);
/// What more an effect does to a creature it reached, out of a fight: a heal mends its cuts and scars, and a miracle a
/// creature cast, itself too, changes how nice it finds that creature, no more often than once a minute
void AfterEffect(entt::entity entity, const magic::EffectValues& values, const magic::EffectSource& source);

/// The heal cures poison
void CurePoison(entt::entity entity);

} // namespace openblack::ecs::systems::magic_living
