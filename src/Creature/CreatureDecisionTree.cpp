/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureDecisionTree.h"

#include <cmath>

#include <algorithm>
#include <map>
#include <numeric>

#include <fmt/format.h>

using namespace openblack;
using namespace openblack::creature_tree;

namespace
{
/// Below this the examples agree well enough to stop
constexpr float k_SettledEntropy = 0.01f;
constexpr float k_BucketReach = 0.25f;

constexpr std::array<const char*, k_AttributeCount> k_Names {
    "allegiance", "origin",      "animate",         "player",      "harder than me", "creature type",
    "type",       "life",        "tribe",           "town belief", "town needs",     "town size",
    "desire",     "height",      "spell knowledge", "carrying",    "forest size",    "job",
    "sex",        "object type", "abode type",      "being built", "on fire",
};

using enum Attribute;
constexpr std::array k_Common {Allegiance, Origin, Animate, PlayerNumber, HarderThanMe, CreatureType, Type};
constexpr std::array k_TownSlots {Allegiance,          Origin,        Animate,  PlayerNumber, HarderThanMe, CreatureType, Type,
                                  TownReligiousBelief, TownNeedsMost, TownSize, Tribe};
constexpr std::array k_ForestSlots {Allegiance, Origin, Animate, PlayerNumber, HarderThanMe, CreatureType, Type, ForestSize};
constexpr std::array k_AbodeSlots {Allegiance, Origin,    Animate, PlayerNumber, HarderThanMe,   CreatureType,
                                   Type,       AbodeType, Life,    OnFire,       AbodeBeingBuilt};
constexpr std::array k_VillagerSlots {Allegiance, Origin, Animate,     PlayerNumber, HarderThanMe, CreatureType,
                                      Type,       Sex,    VillagerJob, Life,         OnFire};
constexpr std::array k_CreatureSlots {Allegiance, Origin,         Animate, PlayerNumber,   HarderThanMe, CreatureType,
                                      Type,       DominantDesire, Height,  SpellKnowledge, Carrying};

float Log2Share(size_t count, size_t total)
{
	const auto share = static_cast<float>(count) / static_cast<float>(total);
	return share > 0.0f ? -share * std::log2(share) : 0.0f;
}

/// Grows the node for a set of examples, with the attributes still to test, appending it and its children
size_t Grow(Tree& tree, std::vector<Episode> episodes, std::vector<Attribute> remaining)
{
	const auto index = tree.nodes.size();
	tree.nodes.emplace_back();
	tree.nodes[index].examples = episodes.size();
	if (episodes.empty())
	{
		return index;
	}
	const auto mean =
	    std::accumulate(episodes.begin(), episodes.end(), 0.0f, [](float sum, const Episode& e) { return sum + e.feedback; }) /
	    static_cast<float>(episodes.size());
	tree.nodes[index].bucket = BucketOf(mean);
	if (remaining.empty() || Entropy(episodes) < k_SettledEntropy)
	{
		return index;
	}
	std::optional<Attribute> best;
	float bestGain = 0.0f;
	for (const auto attribute : remaining)
	{
		const auto gain = Gain(episodes, attribute);
		if (gain > bestGain)
		{
			bestGain = gain;
			best = attribute;
		}
	}
	if (!best.has_value())
	{
		return index;
	}
	std::erase(remaining, *best);
	std::map<uint32_t, std::vector<Episode>> split;
	for (auto& episode : episodes)
	{
		split[*episode.belief.Value(*best)].push_back(std::move(episode));
	}
	tree.nodes[index].test = best;
	for (auto& [value, subset] : split)
	{
		const auto child = Grow(tree, std::move(subset), remaining);
		tree.nodes[index].children.emplace_back(value, child);
	}
	return index;
}

void DescribeNode(const Tree& tree, size_t index, size_t depth, std::vector<std::string>& lines, const std::string& prefix)
{
	const auto& node = tree.nodes.at(index);
	const std::string indent(depth * 2, ' ');
	if (!node.test.has_value())
	{
		lines.push_back(fmt::format("{}{}{:+.1f} ({} examples)", indent, prefix, k_Buckets.at(node.bucket), node.examples));
		return;
	}
	lines.push_back(fmt::format("{}{}{}?", indent, prefix, Name(*node.test)));
	for (const auto& [value, child] : node.children)
	{
		DescribeNode(tree, child, depth + 1, lines, fmt::format("{} -> ", value));
	}
}
} // namespace

const char* creature_tree::Name(Attribute attribute)
{
	const auto index = static_cast<size_t>(attribute);
	return index < k_Names.size() ? k_Names.at(index) : "none";
}

const char* creature_tree::BeliefName(uint32_t type)
{
	switch (type)
	{
	case belief_types::k_Town:
		return "town";
	case belief_types::k_Forest:
		return "forest";
	case belief_types::k_Citadel:
		return "citadel";
	case belief_types::k_Abode:
		return "abode";
	case belief_types::k_CitadelPart:
		return "citadel part";
	case belief_types::k_Tree:
		return "tree";
	case belief_types::k_Villager:
		return "villager";
	case belief_types::k_Animal:
		return "animal";
	case belief_types::k_Creature:
		return "creature";
	case belief_types::k_Spell:
		return "spell";
	case belief_types::k_Field:
		return "field";
	case belief_types::k_Feature:
		return "feature";
	case belief_types::k_Flock:
		return "flock";
	default:
		return "object";
	}
}

std::span<const Attribute> creature_tree::SlotsFor(uint32_t beliefType)
{
	switch (beliefType)
	{
	case belief_types::k_Town:
		return k_TownSlots;
	case belief_types::k_Forest:
		return k_ForestSlots;
	case belief_types::k_Abode:
		return k_AbodeSlots;
	case belief_types::k_Villager:
		return k_VillagerSlots;
	case belief_types::k_Creature:
		return k_CreatureSlots;
	default:
		return k_Common;
	}
}

Belief creature_tree::FromSlots(uint32_t type, std::span<const uint32_t> slots)
{
	Belief belief {.type = type};
	const auto names = SlotsFor(type);
	for (size_t i = 0; i < slots.size() && i < names.size(); ++i)
	{
		belief.Set(names[i], slots[i]);
	}
	return belief;
}

std::vector<uint32_t> creature_tree::ToSlots(const Belief& belief)
{
	std::vector<uint32_t> slots;
	for (const auto attribute : SlotsFor(belief.type))
	{
		slots.push_back(belief.Value(attribute).value_or(0));
	}
	return slots;
}

void creature_tree::AddEpisode(std::vector<Episode>& episodes, Episode episode)
{
	if (episodes.size() >= k_MaxEpisodes)
	{
		episodes.erase(episodes.begin(), episodes.begin() + static_cast<std::ptrdiff_t>(episodes.size() - k_MaxEpisodes + 1));
	}
	episodes.push_back(std::move(episode));
}

size_t creature_tree::BucketOf(float feedback)
{
	for (size_t i = 0; i < k_Buckets.size(); ++i)
	{
		if (std::abs(feedback - k_Buckets.at(i)) <= k_BucketReach)
		{
			return i;
		}
	}
	return feedback < 0.0f ? 0 : k_Buckets.size() - 1;
}

float creature_tree::Entropy(std::span<const Episode> episodes)
{
	if (episodes.empty())
	{
		return 0.0f;
	}
	std::array<size_t, 2> signs {};
	std::array<size_t, k_Buckets.size()> buckets {};
	for (const auto& episode : episodes)
	{
		++signs.at(episode.feedback >= 0.0f ? 1 : 0);
		++buckets.at(BucketOf(episode.feedback));
	}
	float sign = 0.0f;
	for (const auto count : signs)
	{
		sign += Log2Share(count, episodes.size());
	}
	float bucket = 0.0f;
	for (const auto count : buckets)
	{
		bucket += Log2Share(count, episodes.size());
	}
	return (2.0f * sign + bucket) / 3.0f;
}

float creature_tree::Gain(std::span<const Episode> episodes, Attribute attribute)
{
	if (episodes.empty())
	{
		return 0.0f;
	}
	std::map<uint32_t, std::vector<Episode>> split;
	for (const auto& episode : episodes)
	{
		const auto value = episode.belief.Value(attribute);
		if (!value.has_value())
		{
			return 0.0f;
		}
		split[*value].push_back(episode);
	}
	float remainder = 0.0f;
	float splitInfo = 0.0f;
	for (const auto& [value, subset] : split)
	{
		const auto share = static_cast<float>(subset.size()) / static_cast<float>(episodes.size());
		remainder += share * Entropy(subset);
		splitInfo += Log2Share(subset.size(), episodes.size());
	}
	return (Entropy(episodes) - remainder) / std::max(splitInfo, 1.0f);
}

Tree creature_tree::Build(std::span<const Episode> episodes, std::span<const Attribute> allowed)
{
	Tree tree;
	if (episodes.empty())
	{
		return tree;
	}
	Grow(tree, {episodes.begin(), episodes.end()}, {allowed.begin(), allowed.end()});
	return tree;
}

float creature_tree::Evaluate(const Tree& tree, const Belief& belief)
{
	if (tree.nodes.empty())
	{
		return 0.0f;
	}
	size_t index = 0;
	while (true)
	{
		const auto& node = tree.nodes.at(index);
		if (!node.test.has_value())
		{
			return k_Buckets.at(node.bucket);
		}
		const auto value = belief.Value(*node.test);
		if (!value.has_value())
		{
			return 0.0f;
		}
		const auto found = std::ranges::find(node.children, *value, &std::pair<uint32_t, size_t>::first);
		if (found == node.children.end())
		{
			return 0.0f;
		}
		index = found->second;
	}
}

float creature_tree::Usefulness(float utility)
{
	return utility >= 0.0f ? 0.1f + 0.9f * utility : 0.1f * (utility + 1.0f);
}

std::vector<std::string> creature_tree::Describe(const Tree& tree)
{
	std::vector<std::string> lines;
	if (!tree.nodes.empty())
	{
		DescribeNode(tree, 0, 0, lines, "");
	}
	return lines;
}
