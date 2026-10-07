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
#include <optional>
#include <span>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "SpellRules.h"

namespace openblack
{
struct GAlignmentInfo;
}

// What a miracle's effect does where it lands: every object it reaches takes it, not only the nearest. The objects are
// looked for in the map's cells over the square round the point the effect's radius spans, x by x and in each x by z,
// each cell's things in the order the cell keeps them, so a building is met once for each cell of the square it covers
// and takes the effect each time; one is reached when its edge
// is within the radius across the land and its height within the radius up and down. Each takes the effect by how it
// stands up to each kind, which heals or hurts it. Changing a thing's life changes the caster's alignment: hurting the
// living and nice animals turns them evil, hurting nasty animals and skeletons good, healing the other way, damped by
// how far the caster already leans. Pure functions, so they are tested on made-up objects.

namespace openblack::magic
{

/// The land's cells the objects are found by are this many units across
inline constexpr float k_EffectCellSize = 10.0f;

/// A square of the land's cells, both corners included
struct CellRange
{
	glm::ivec2 first {0};
	glm::ivec2 last {-1};

	[[nodiscard]] bool Contains(glm::ivec2 cell) const;
	[[nodiscard]] size_t Count() const;
};

/// The cell a point is in
[[nodiscard]] glm::ivec2 EffectCellOf(glm::vec3 point);
/// The map cells over the square a radius spans round a point, the point taken as a map position
[[nodiscard]] CellRange EffectCellsAround(glm::vec3 point, float radius);

/// One object an effect may reach, as the world describes it
struct EffectReceiver
{
	entt::entity entity {entt::null};
	/// The middle of its foot and how far its edge is from it, across the land
	glm::vec3 position {0.0f};
	float radius {0.0f};
	float height {0.0f};
	/// Its life, 0 to 1; none for what has no life to lose yet (buildings, trees and features until they can be hurt)
	std::optional<float> life;
	EffectDefence defence;
	AlignmentType alignmentType {AlignmentType::Unimportant};
	/// Whether crushing it makes the people nearby react
	bool crushable {false};
};

/// Whether an effect at a point reaches an object: its edge within the radius across the land, and its height within
/// the radius up and down
[[nodiscard]] bool Reaches(const EffectValues& values, glm::vec3 point, const EffectReceiver& receiver);

/// A crush stronger than this makes the people nearby react to what was crushed
inline constexpr float k_CrushReactionThreshold = 0.01f;

/// What an effect did to one object
struct EffectOutcome
{
	entt::entity entity {entt::null};
	std::optional<float> lifeBefore;
	std::optional<float> lifeAfter;
	/// Its life was given back, or taken, by so much
	float healed {0.0f};
	float damaged {0.0f};
	/// The harm the heat would do it, which counts for the alignment; the life itself is lost as it burns
	float burnt {0.0f};
	/// It had life and has none left
	bool destroyed {false};
	/// It was crushed hard enough for people to react
	bool crushed {false};
	/// How much the caster's alignment moves for it, before the turn's limit
	float alignmentChange {0.0f};
	/// What was done to it counts as an attack: it burnt, crushed, hit or threw it, and it was hurt
	bool aggression {false};
};

/// An effect's values for one event: its table's, times the strength the event paid for, times the caster's tribal power
/// once more (the strength already carries it), times the event's own strength
[[nodiscard]] EffectValues EventEffectValues(EffectValues values, float paidStrength, float tribalPower, float eventStrength);

/// The effect on one object it reaches: the healing, then the harm, by its defence. The caster's alignment moves for the
/// change of life, from a row of the alignment table per kind of effect, by the object's alignment kind.
[[nodiscard]] EffectOutcome ApplyEffectTo(const EffectValues& values, const EffectReceiver& receiver,
                                          std::span<const GAlignmentInfo> alignmentTable, float alignmentAddition,
                                          float casterAlignment);

/// Every object in reach takes the effect, in the order given
[[nodiscard]] std::vector<EffectOutcome> ApplyEffectToAll(const EffectValues& values, glm::vec3 point,
                                                          std::span<const EffectReceiver> receivers,
                                                          std::span<const GAlignmentInfo> alignmentTable,
                                                          float alignmentAddition, float casterAlignment);

/// One kind of effect's weight against one alignment kind of object, from the alignment table
[[nodiscard]] float AlignmentWeight(const GAlignmentInfo& row, AlignmentType type);

/// How much a change of alignment moves a caster who already leans: less the same way, more the other way
[[nodiscard]] float DampAlignmentChange(float change, float current);

/// The caster's alignment moves for what an effect did to a thing whose life changed by so much; its heat counts by the
/// harm it did
[[nodiscard]] float EffectAlignmentChange(const EffectValues& values, float burnDamage, AlignmentType type, float lifeChange,
                                          std::span<const GAlignmentInfo> alignmentTable, float alignmentAddition,
                                          float casterAlignment);

/// A turn of an alignment, a player's or a creature's: the change waiting, held between -1 and 1, moves it by that share
/// of its owner's change a turn, and whatever was waiting is then gone. Returns the alignment, held between -1 and 1.
[[nodiscard]] float StepPendingAlignment(float alignment, float& pending, float changePerTurn);

} // namespace openblack::magic
