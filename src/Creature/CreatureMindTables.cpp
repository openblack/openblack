/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureMindTables.h"

#include <cstring>

#include <algorithm>

#include "InfoConstants.h"

using namespace openblack;
using namespace openblack::creature_mind_tables;

namespace
{
/// The tables mark unused slots of the action lists with 0 (no action) or with a number past the last action
constexpr uint32_t k_NoAction = 0;

constexpr std::array<float, 17> k_MiracleMultipliers {1.0f, 1.7f,  1.5f, 1.2f, 1.1f, 1.3f, 1.4f, 1.2f, 1.4f,
                                                      1.3f, 1.35f, 1.9f, 0.9f, 4.0f, 1.2f, 1.5f, 1.1f};

template <size_t N>
std::string Text(const std::array<char, N>& chars)
{
	return {chars.data(), strnlen(chars.data(), chars.size())};
}

/// A table row of 32-bit values read as its numbers
template <typename T, typename Row>
std::vector<T> Words(const Row& row)
{
	static_assert(sizeof(Row) % sizeof(T) == 0);
	std::vector<T> values(sizeof(Row) / sizeof(T));
	std::memcpy(values.data(), &row, sizeof(Row));
	return values;
}
} // namespace

Tables creature_mind_tables::Build(const InfoConstants& info)
{
	Tables tables;
	for (const auto& row : info.creatureAction)
	{
		tables.actions.push_back({
		    .name = Text(row.name),
		    .desire = row.desire,
		    .desireMultiplier = row.desireMultiplier,
		    .alwaysApplies = row.field0xc8 != 0,
		    .learningWindowSeconds = row.field0xd0,
		    .learnable = row.field0xf0 != 0,
		    .eatWhenStroked = row.field0xe0 != 0,
		});
	}
	const auto actionCount = static_cast<uint32_t>(tables.actions.size());
	for (size_t d = 0; d < k_DesireCount; ++d)
	{
		for (const auto action : Words<uint32_t>(info.creatureDesireAction1.at(d)))
		{
			if (action != k_NoAction && action < actionCount &&
			    std::ranges::find(tables.desireActions.at(d), action) == tables.desireActions.at(d).end())
			{
				tables.desireActions.at(d).push_back(action);
			}
		}
		const auto& initial = info.creatureInitialDesire.at(d);
		tables.rules.at(d) = {
		    .initialMax = initial.field0x3c,
		    .decayMin = initial.field0x40,
		    .decayMax = initial.field0x44,
		    .initialWeight = initial.field0x68,
		    .learnsWeight = initial.field0x6c != 0,
		    .lessonDivisor = initial.field0x54,
		    .learnable = initial.field0x28 != 0,
		    .distanceWeight = initial.field0x58,
		};
		tables.texts.at(d) = {
		    .desire = Text(initial.field0xb0),
		    .trying = Text(initial.field0x130),
		    .doing = Text(initial.field0x170),
		};
		const auto dependencies = Words<float>(info.creatureDesireDependency.at(d));
		for (size_t e = 0; e < k_DesireCount && e < dependencies.size(); ++e)
		{
			tables.dependencies.at(d).at(e) = dependencies[e];
		}
		for (const auto attribute : Words<uint32_t>(info.creatureDesireAttributeEntry.at(d)))
		{
			if (attribute < creature_tree::k_NoAttribute)
			{
				tables.attributes.at(d).push_back(static_cast<creature_tree::Attribute>(attribute));
			}
		}
	}
	for (const auto& entry : info.creatireActionKnownAboutEntry)
	{
		tables.skills.push_back({
		    .name = Text(entry.field0x0),
		    .watchSeconds = entry.field0x48,
		    .minPhase = entry.field0x50,
		});
	}
	for (const auto& entry : info.creatureMagicActionKnownAboutEntry)
	{
		tables.miracles.push_back({
		    .name = Text(entry.field0x0),
		    .timesToSee = entry.field0x44,
		    .minPhase = entry.field0x50,
		    .knownAtStart = entry.field0x54 != 0,
		    .action = entry.field0x58,
		});
	}
	for (const auto& entry : info.creatureMimic)
	{
		creature_watching::MimicRule rule {
		    .name = Text(entry.name),
		    .chance = entry.field0x80,
		    .needsLearningLeash = entry.field0x84 != 0,
		    .noticeAction = entry.field0x88,
		    .desire = entry.field0xa4,
		    .stageSteps = entry.field0xa8,
		    .copiesDesire = entry.field0xac != 0,
		};
		for (const auto action :
		     {entry.field0x8c, entry.field0x90, entry.field0x94, entry.field0x98, entry.field0x9c, entry.field0xa0})
		{
			if (action != k_NoAction && action < actionCount)
			{
				rule.copyActions.push_back(action);
			}
		}
		tables.mimics.push_back(std::move(rule));
	}
	return tables;
}

std::optional<uint32_t> creature_mind_tables::FindAction(const Tables& tables, std::string_view name)
{
	const auto found = std::ranges::find(tables.actions, name, &ActionInfo::name);
	if (found == tables.actions.end())
	{
		return std::nullopt;
	}
	return static_cast<uint32_t>(std::distance(tables.actions.begin(), found));
}

float creature_mind_tables::MiracleMultiplier(size_t speciesRow)
{
	return k_MiracleMultipliers.at(std::min(speciesRow, k_MiracleMultipliers.size() - 1));
}
