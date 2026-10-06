/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <optional>
#include <string_view>
#include <utility>

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>

#include "Creature/CreatureDecisionTree.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureIdleMind.h"
#include "Enums.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs
{
class Registry;
}
namespace openblack::ecs::components
{
struct CreatureMindState;
}

/// What the creature mind system's files share: how a species' desires start, and finding food, water and things to
/// hurl at about a creature
namespace openblack::ecs::systems::mind_detail
{
[[nodiscard]] std::array<creature_desires::DesireSetup, creature_desires::k_DesireCount> SetupFor(CreatureType species);
[[nodiscard]] std::optional<float> FoodValueOf(entt::entity entity);
[[nodiscard]] std::optional<creature_mind::Wants::WaterSpot> NearestWater(glm::vec2 from);
[[nodiscard]] std::optional<glm::vec2> NearestHurlTarget(ecs::Registry& registry, glm::vec2 from);
/// What a creature knows of a thing, as far as the world here tells it
[[nodiscard]] std::optional<creature_tree::Belief> BeliefOf(const ecs::Registry& registry, entt::entity entity,
                                                            entt::entity self);
/// How useful the creature has learnt a food is to eat, 0.1 when nothing is known
[[nodiscard]] float FoodUsefulness(const ecs::Registry& registry, const components::CreatureMindState& mind, entt::entity self,
                                   entt::entity food);
/// Having done an action, the desire it satisfies is less, by the game's action table, and its body pays for it
void Satisfied(entt::entity creature, creature_desires::Desires& desires, std::string_view action);
} // namespace openblack::ecs::systems::mind_detail
