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
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Creature/CreatureDecisionTree.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureLearning.h"
#include "Creature/CreatureWatching.h"

namespace openblack
{
namespace v120
{
struct InfoConstants;
}
using InfoConstants = v120::InfoConstants;
} // namespace openblack

/// The game's tables for a creature's mind, taken from info.dat into plain values: the actions and what they do to
/// desires, which actions satisfy each desire, how feedback changes each desire, how desires depend on one another,
/// what each desire's decision trees may test, and the skills, miracles and player's deeds a creature can learn from.
namespace openblack::creature_mind_tables
{
using creature_desires::k_DesireCount;

struct ActionInfo
{
	std::string name;
	/// The desire the action serves, and what the desire is multiplied by once it is done
	uint32_t desire {0};
	float desireMultiplier {1.0f};
	/// Whether that applies however the action came about
	bool alwaysApplies {false};
	/// How many seconds after it finishes the action can still be learnt from, and whether it can be at all
	float learningWindowSeconds {0.0f};
	bool learnable {false};
	/// Stroked while holding something during it, the creature learns to eat what it holds
	bool eatWhenStroked {false};
};

struct DesireTexts
{
	/// As in "desire to impress", "am trying to impress", "am impressing"
	std::string desire;
	std::string trying;
	std::string doing;
};

struct Tables
{
	std::vector<ActionInfo> actions;
	/// The actions that satisfy each desire
	std::array<std::vector<uint32_t>, k_DesireCount> desireActions {};
	creature_learning::AllDesireRules rules {};
	creature_learning::Dependencies dependencies {};
	std::array<std::vector<creature_tree::Attribute>, k_DesireCount> attributes {};
	std::array<DesireTexts, k_DesireCount> texts {};
	std::vector<creature_watching::SkillRule> skills;
	std::vector<creature_watching::MiracleRule> miracles;
	std::vector<creature_watching::MimicRule> mimics;
};

[[nodiscard]] Tables Build(const InfoConstants& info);
/// An action's row by its name
[[nodiscard]] std::optional<uint32_t> FindAction(const Tables& tables, std::string_view name);

/// How many more times than usual each species must see a miracle to learn it, by its row in the creature tables
[[nodiscard]] float MiracleMultiplier(size_t speciesRow);

} // namespace openblack::creature_mind_tables
