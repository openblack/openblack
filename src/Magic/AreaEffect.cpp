/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AreaEffect.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

#include "3D/MapCoords.h"
#include "InfoConstants.h"

using namespace openblack;
using namespace openblack::magic;

bool CellRange::Contains(glm::ivec2 cell) const
{
	return cell.x >= first.x && cell.y >= first.y && cell.x <= last.x && cell.y <= last.y;
}

size_t CellRange::Count() const
{
	if (last.x < first.x || last.y < first.y)
	{
		return 0;
	}
	return static_cast<size_t>(last.x - first.x + 1) * static_cast<size_t>(last.y - first.y + 1);
}

glm::ivec2 magic::EffectCellOf(glm::vec3 point)
{
	return {static_cast<int>(std::floor(point.x / k_EffectCellSize)), static_cast<int>(std::floor(point.z / k_EffectCellSize))};
}

CellRange magic::EffectCellsAround(glm::vec3 point, float radius)
{
	// The point as a map position, back in metres, the radius taken off or added, and the cell of that as a map
	// position again, its high word read signed
	const auto edge = [radius](float metres, float side) {
		const float at = static_cast<float>(map_coords::ToFixed(metres)) * 10.0f * (1.0f / 65536.0f);
		const float moved = side < 0.0f ? at + -radius : at + radius;
		return static_cast<int>(map_coords::SignedCellOf(map_coords::FtoL(moved * 65536.0f / 10.0f)));
	};
	return {.first = {edge(point.x, -1.0f), edge(point.z, -1.0f)}, .last = {edge(point.x, 1.0f), edge(point.z, 1.0f)}};
}

bool magic::Reaches(const EffectValues& values, glm::vec3 point, const EffectReceiver& receiver)
{
	const float across = glm::distance(glm::vec2(point.x, point.z), glm::vec2(receiver.position.x, receiver.position.z));
	return across <= receiver.radius + values.radius &&
	       std::abs(point.y - receiver.position.y) <= receiver.height + values.radius;
}

EffectValues magic::EventEffectValues(EffectValues values, float paidStrength, float tribalPower, float eventStrength)
{
	values.Scale(paidStrength * tribalPower * eventStrength);
	return values;
}

float magic::AlignmentWeight(const GAlignmentInfo& row, AlignmentType type)
{
	switch (type)
	{
	case AlignmentType::AnimalNice:
		return row.animalNice;
	case AlignmentType::AnimalNasty:
		return row.animalNasty;
	case AlignmentType::Creature:
		return row.creature;
	case AlignmentType::Priest:
		return row.priest;
	case AlignmentType::Skeleton:
		return row.skeleton;
	case AlignmentType::Villager:
		return row.villager;
	case AlignmentType::Building:
		return row.building;
	case AlignmentType::Plant:
		return row.plant;
	case AlignmentType::Field:
		return row.field;
	case AlignmentType::Feature:
		return row.feature;
	case AlignmentType::MobileObject:
		return row.mobileObject;
	case AlignmentType::Land:
		return row.land;
	case AlignmentType::Script:
		return row.script;
	case AlignmentType::Unimportant:
		return row.unimportant;
	}
	return 0.0f;
}

float magic::DampAlignmentChange(float change, float current)
{
	const float lean = std::abs(current) * 0.5f;
	const bool sameWay = (change >= 0.0f) == (current >= 0.0f);
	return sameWay ? change * (1.0f - lean) : change * (1.0f + lean);
}

float magic::EffectAlignmentChange(const EffectValues& values, float burnDamage, AlignmentType type, float lifeChange,
                                   std::span<const GAlignmentInfo> alignmentTable, float alignmentAddition,
                                   float casterAlignment)
{
	const float change = std::abs(lifeChange);
	if (change == 0.0f)
	{
		return 0.0f;
	}
	const float weight = change + alignmentAddition;
	float total = 0.0f;
	for (const auto kind : {EffectKind::Burn, EffectKind::Crush, EffectKind::Hit, EffectKind::Heal, EffectKind::FlyAway})
	{
		const auto row = static_cast<size_t>(kind);
		if (row >= alignmentTable.size())
		{
			continue;
		}
		// Heat counts by the harm it does rather than by how hot it is
		const float amount = kind == EffectKind::Burn ? burnDamage : values[kind];
		const float value = amount * AlignmentWeight(alignmentTable[row], type) * weight;
		total += DampAlignmentChange(value, casterAlignment);
	}
	return total;
}

EffectOutcome magic::ApplyEffectTo(const EffectValues& values, const EffectReceiver& receiver,
                                   std::span<const GAlignmentInfo> alignmentTable, float alignmentAddition,
                                   float casterAlignment)
{
	EffectOutcome outcome {.entity = receiver.entity, .lifeBefore = receiver.life, .lifeAfter = receiver.life};
	outcome.damaged = DamageFrom(values, receiver.defence);
	outcome.healed = HealFrom(values, receiver.defence);
	const float burn = values[EffectKind::Burn];
	outcome.burnt = burn > 0.0f ? HeatDamage(burn, receiver.defence) : 0.0f;
	outcome.crushed = values[EffectKind::Crush] > k_CrushReactionThreshold && receiver.crushable;
	if (receiver.life.has_value())
	{
		const float before = *receiver.life;
		float life = std::min(before + outcome.healed, 1.0f);
		life = std::max(life - outcome.damaged, 0.0f);
		outcome.lifeAfter = life;
		outcome.destroyed = life == 0.0f && before != 0.0f;
		// Heat takes no life by itself: it heats what it reaches, which then burns. The alignment moves only when the heal
		// or the harm changed the life, and then heat counts too, by the harm it would do.
		outcome.alignmentChange = EffectAlignmentChange(values, outcome.burnt, receiver.alignmentType, before - life,
		                                                alignmentTable, alignmentAddition, casterAlignment);
	}
	outcome.aggression = values.IsDestructive() && (outcome.damaged + outcome.burnt) > 0.0f;
	return outcome;
}

std::vector<EffectOutcome> magic::ApplyEffectToAll(const EffectValues& values, glm::vec3 point,
                                                   std::span<const EffectReceiver> receivers,
                                                   std::span<const GAlignmentInfo> alignmentTable, float alignmentAddition,
                                                   float casterAlignment)
{
	std::vector<EffectOutcome> outcomes;
	for (const auto& receiver : receivers)
	{
		if (Reaches(values, point, receiver))
		{
			outcomes.push_back(ApplyEffectTo(values, receiver, alignmentTable, alignmentAddition, casterAlignment));
		}
	}
	return outcomes;
}

float magic::StepPendingAlignment(float alignment, float& pending, float changePerTurn)
{
	const float step = changePerTurn * std::clamp(pending, -1.0f, 1.0f);
	pending = 0.0f;
	return std::clamp(alignment + step, -1.0f, 1.0f);
}
