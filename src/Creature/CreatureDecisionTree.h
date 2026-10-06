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
#include <span>
#include <string>
#include <vector>

/// What a creature learns about the things it acts on. For each desire it keeps two decision trees: which things to act
/// on to satisfy it, and which things to use doing so. Each tree learns from the latest examples of things it acted on
/// and the feedback it had for them, and is grown again from scratch after every new example. A tree tests the
/// attributes of a thing (whose it is, whether it is alive, what kind of thing it is, and so on) and ends in how good the
/// thing is, from -1 to 1. Grown the C4.5 way: at each step the attribute that best separates the examples' feedback is
/// tested, until the examples left agree or no attribute helps.
namespace openblack::creature_tree
{

/// What a creature can know about a thing, as the game's decision trees test it
enum class Attribute : uint8_t
{
	/// Whose it is: the creature's own player's, nobody's, another player's
	Allegiance,
	/// Natural or made
	Origin,
	/// Whether it is alive and moves
	Animate,
	PlayerNumber,
	/// Whether it is stronger than the creature
	HarderThanMe,
	/// The species of a creature
	CreatureType,
	/// The kind of thing (the belief type)
	Type,
	Life,
	Tribe,
	TownReligiousBelief,
	TownNeedsMost,
	TownSize,
	/// The strongest desire of another creature
	DominantDesire,
	Height,
	SpellKnowledge,
	Carrying,
	ForestSize,
	VillagerJob,
	Sex,
	MobileObjectType,
	AbodeType,
	AbodeBeingBuilt,
	OnFire,
	_Count
};
constexpr size_t k_AttributeCount = static_cast<size_t>(Attribute::_Count);
/// The game's tables use this for no attribute
constexpr uint32_t k_NoAttribute = k_AttributeCount;
[[nodiscard]] const char* Name(Attribute attribute);

/// The kinds of thing a creature has beliefs about, as the game numbers them
namespace belief_types
{
constexpr uint32_t k_Town = 0;
constexpr uint32_t k_Forest = 1;
constexpr uint32_t k_Citadel = 2;
constexpr uint32_t k_Abode = 3;
constexpr uint32_t k_CitadelPart = 4;
constexpr uint32_t k_Tree = 5;
constexpr uint32_t k_Villager = 6;
constexpr uint32_t k_Animal = 7;
constexpr uint32_t k_Creature = 8;
constexpr uint32_t k_Spell = 10;
constexpr uint32_t k_Field = 11;
constexpr uint32_t k_Feature = 15;
constexpr uint32_t k_Other = 21;
constexpr uint32_t k_Flock = 22;
} // namespace belief_types
/// What a kind of thing is called, for the debug readouts
[[nodiscard]] const char* BeliefName(uint32_t type);

/// The attributes a kind of thing has, in the order the game lists them in its mind files: seven every thing has, and
/// four more for towns, abodes, villagers and creatures, one more for forests
[[nodiscard]] std::span<const Attribute> SlotsFor(uint32_t beliefType);

/// What the creature knew of a thing: its kind and the values of the attributes it has
struct Belief
{
	uint32_t type {belief_types::k_Other};
	std::array<std::optional<uint32_t>, k_AttributeCount> values {};

	[[nodiscard]] std::optional<uint32_t> Value(Attribute attribute) const { return values.at(static_cast<size_t>(attribute)); }
	void Set(Attribute attribute, uint32_t value) { values.at(static_cast<size_t>(attribute)) = value; }
};
/// A belief from the values listed in its kind's order, as mind files keep them
[[nodiscard]] Belief FromSlots(uint32_t type, std::span<const uint32_t> slots);
/// A belief's values in its kind's order, missing ones as 0
[[nodiscard]] std::vector<uint32_t> ToSlots(const Belief& belief);

/// One example: a thing and the feedback for acting on it, from -1 to 1
struct Episode
{
	Belief belief;
	float feedback {0.0f};
	/// What a mind file keeps with an example and the trees don't use: the kind of lesson, the action, and where the
	/// thing was
	std::array<int32_t, 4> saved {};
};
/// A tree keeps its newest examples, at most this many
constexpr size_t k_MaxEpisodes = 16;
/// Adds an example, the oldest going when there are too many
void AddEpisode(std::vector<Episode>& episodes, Episode episode);

/// The feedback is sorted into 11 steps from -1 to 1, each the first whose middle is within a quarter of it
constexpr std::array<float, 11> k_Buckets {-1.0f, -0.8f, -0.6f, -0.4f, -0.2f, 0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f};
constexpr size_t k_NeutralBucket = 5;
[[nodiscard]] size_t BucketOf(float feedback);
/// How mixed the examples' feedback is: two parts how mixed its signs are and one how mixed its steps are, in bits
[[nodiscard]] float Entropy(std::span<const Episode> episodes);
/// How much testing an attribute separates the examples, against how much it splits them; 0 if any example lacks it
[[nodiscard]] float Gain(std::span<const Episode> episodes, Attribute attribute);

struct Node
{
	/// The attribute tested, or none at a leaf
	std::optional<Attribute> test;
	/// For each value of the attribute the examples had, the node it leads to
	std::vector<std::pair<uint32_t, size_t>> children;
	/// A leaf's step of the feedback scale
	size_t bucket {k_NeutralBucket};
	/// How many examples reached it
	size_t examples {0};
};
struct Tree
{
	/// The root first; empty for a tree with no examples
	std::vector<Node> nodes;
};
/// Grows a tree from examples, testing only the attributes allowed
[[nodiscard]] Tree Build(std::span<const Episode> episodes, std::span<const Attribute> allowed);
/// How good a thing is by the tree, from -1 to 1; 0 when the tree has nothing to say about it
[[nodiscard]] float Evaluate(const Tree& tree, const Belief& belief);
/// How useful a thing is for the planner, from how good it is: 0.1 for things nothing is known about, rising to 1 for
/// the best and falling to 0 for the worst
[[nodiscard]] float Usefulness(float utility);
/// The tree as lines of text, for the debug readouts
[[nodiscard]] std::vector<std::string> Describe(const Tree& tree);

} // namespace openblack::creature_tree
