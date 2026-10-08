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

/// Who an effect comes from, whose alignment what it does moves
struct EffectSource
{
	PlayerNames player {PlayerNames::NEUTRAL};
	/// The creature that cast it, if a creature did: its own alignment moves rather than its player's
	entt::entity casterCreature {entt::null};
	/// What applied it when no miracle did, such as the thing a blow came from: none at all for a fall onto the land.
	/// Without it, the miracle's own rules hold. With it, the people round react to it rather than to what it struck,
	/// and a town counts an attack only when something applied the harm
	std::optional<entt::entity> appliedBy;
	/// The effect comes from no player at all, as a fall nobody caused: no player's alignment moves and nobody is
	/// remembered for the harm
	bool playerless {false};
};

/// A drop of the water miracle's rain
struct WaterDrop
{
	glm::vec3 position {0.0f};
	/// The objects whose edge it falls this near take the water
	float reach {0.0f};
	/// A ring left on the land, growing by this, when it is time for one
	std::optional<float> ringGrowth;
	/// The power-up's water grows any tree past its full size, and plants none
	bool extreme {false};
	/// The miracle it falls from, and who cast it
	entt::entity spell {entt::null};
	EffectSource source;
};

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
	/// Land rather than water by its cell: what a miracle that must be held over land needs
	[[nodiscard]] virtual bool IsLand(glm::vec3 point) const = 0;
	/// Within a player's influence, where most of its miracles may be cast
	[[nodiscard]] virtual bool InInfluence(PlayerNames player, glm::vec3 point) const = 0;
	/// Where an object stands, none once it has gone
	[[nodiscard]] virtual std::optional<glm::vec3> PositionOf(entt::entity object) const = 0;

	/// An effect on one object, which heals and hurts it by how it stands up to each kind, and moves the alignment of
	/// whoever it comes from by what it did; whether it took it
	virtual bool ApplyEffect(entt::entity object, const EffectValues& values, const EffectSource& source) = 0;
	/// An effect at a point: every object it reaches takes it, found in the land's cells round the point (see
	/// AreaEffect.h). The objects that took it.
	virtual std::vector<entt::entity> ApplyEffectAt(glm::vec3 point, const EffectValues& values,
	                                                const EffectSource& source) = 0;
	/// A drop of rain lands: it waters the trees and fields within reach, the people watch it put out what burns and,
	/// when asked, it leaves a ring on the land. Its cooling is its effect's burn, given round the point.
	virtual void Water(const WaterDrop& drop) = 0;
	/// The living things the heal heals within a radius of a point, no more than a number, as the game finds them: through
	/// the land's cells in a spiral out from the point's (see HealTargets.h)
	[[nodiscard]] virtual std::vector<entt::entity> HealTargets(glm::vec3 point, float radius, size_t maximum) const = 0;
	/// The heal cures a living thing of poison
	virtual void CurePoison(entt::entity /*object*/) {}
	/// A player's miracle of a kind reached an object, the last its effect reached: the player's creature may learn the
	/// deed that shows (MiracleDeeds.h)
	virtual void PlayerAffected(entt::entity /*object*/, MagicType /*type*/, PlayerNames /*player*/) {}
	/// Food or wood poured at a point by a player, into the stores and onto the piles of it about there, the rest onto a
	/// new pile unless in the water; whether any was put down. A new pile of food that speeds up the people who take
	/// from it sparkles.
	virtual bool AddResource(ResourceType type, glm::vec3 point, uint32_t amount, bool speedUp, PlayerNames player) = 0;
	/// Whether a miracle may destroy an object, as a tornado does what it picks up and a blast's wave what it shatters:
	/// anything but a creature, a field or a part of a citadel
	[[nodiscard]] virtual bool CanBeDestroyedBySpell(entt::entity /*object*/) const { return false; }
};

} // namespace openblack::magic
