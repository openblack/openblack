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

#include <optional>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "SpellRules.h"

namespace openblack::magic
{

/// What the miracles do to the world and ask of it. The game backs it with the land, the creatures, villagers, trees,
/// piles and the other systems; tests use a fake. Where the game has nothing yet to act on, such as fire, the game's
/// world does what it can and says so.
class MagicWorldInterface
{
public:
	MagicWorldInterface() = default;
	MagicWorldInterface(const MagicWorldInterface&) = delete;
	MagicWorldInterface& operator=(const MagicWorldInterface&) = delete;
	MagicWorldInterface(MagicWorldInterface&&) = delete;
	MagicWorldInterface& operator=(MagicWorldInterface&&) = delete;
	virtual ~MagicWorldInterface() = default;

	[[nodiscard]] virtual float LandHeight(glm::vec2 xz) const = 0;
	/// On the map at all
	[[nodiscard]] virtual bool InBounds(glm::vec3 point) const = 0;
	/// Land above the sea, where piles can be put down
	[[nodiscard]] virtual bool IsDryLand(glm::vec3 point) const = 0;
	/// Within a player's influence, where most of its miracles may be cast
	[[nodiscard]] virtual bool InInfluence(PlayerNames player, glm::vec3 point) const = 0;
	/// Where an object stands, none once it has gone
	[[nodiscard]] virtual std::optional<glm::vec3> PositionOf(entt::entity object) const = 0;

	/// An effect on one object, which heals and hurts it by how it stands up to each kind; whether it took it
	virtual bool ApplyEffect(entt::entity object, const EffectValues& values) = 0;
	/// An effect at a point: the nearest object that takes effects within the effect's radius takes it. That object, or
	/// none.
	virtual entt::entity ApplyEffectAt(glm::vec3 point, const EffectValues& values) = 0;
	/// Heat round a point, as a fireball gives off: the living and the things that burn within the radius suffer the heat
	/// above their combustion temperature
	virtual void Heat(glm::vec3 point, float radius, float temperature) = 0;
	/// A drop of rain lands: it cools what burns, waters the trees and fields within reach and, when asked, leaves a ring
	/// on the land growing by the growth
	virtual void Water(glm::vec3 drop, float reach, std::optional<float> ringGrowth) = 0;
	/// The living things that can be healed within a radius of a point, the nearest first, no more than a number
	[[nodiscard]] virtual std::vector<entt::entity> HealTargets(glm::vec3 point, float radius, size_t maximum) const = 0;
	/// Food or wood put down at a point, onto a pile of it there or a new pile; whether it was put down. Food that
	/// sparkles makes the people who eat it work faster.
	virtual bool AddResource(ResourceType type, glm::vec3 point, uint32_t amount, bool sparkles) = 0;
};

} // namespace openblack::magic
