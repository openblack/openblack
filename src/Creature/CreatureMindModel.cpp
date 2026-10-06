/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureMindModel.h"

#include <algorithm>
#include <string_view>

#include "Creature/CreaturePlanner.h"

using namespace openblack;
using namespace openblack::creature_mind_model;
using creature_desires::Desire;

namespace
{
/// What a fresh file says it was saved as and who it belonged to
constexpr std::string_view k_SavedAs = "openblack.erc";
constexpr std::u16string_view k_Profile = u"openblack";
/// The counts of the sections of a current mind file
constexpr size_t k_Counters = 7;
constexpr size_t k_PlayerDesires = 40;
constexpr size_t k_TownDesires = 17;
constexpr size_t k_LearningCounts = 45;

creaturemind::MindFileData Skeleton(size_t skills, size_t miracles)
{
	creaturemind::MindFileData file;
	file.version = creaturemind::k_CurrentVersion;
	file.savedAs.assign(k_SavedAs.begin(), k_SavedAs.end());
	file.savedAs.push_back(0);
	for (const auto c : k_Profile)
	{
		file.profile.push_back(static_cast<uint8_t>(c & 0xff));
		file.profile.push_back(static_cast<uint8_t>(c >> 8));
	}
	file.profile.insert(file.profile.end(), {0, 0});
	file.counters.assign(k_Counters, 0);
	file.desireMemories.assign(k_DesireCount, {});
	file.actionsSeen.assign(skills, {.count = 0, .turn = 0});
	file.miraclesSeen.assign(miracles, {.count = 0, .turn = 0});
	file.playerDesires = std::vector<float>(k_PlayerDesires, 0.0f);
	file.townDesires = std::vector<float>(k_TownDesires, 0.0f);
	file.unknown1 = 0;
	file.alignment = 0.0f;
	file.physique.scratch = 0.0f;
	file.physique.unknown3 = 0;
	file.physique.size = 1.0f;
	file.database = std::vector<creaturemind::MindDatabaseEntry> {};
	file.unknown2 = 0;
	file.unknown3 = std::array<uint32_t, 4> {};
	file.tattooHeader = std::array<uint32_t, 8> {};
	file.tattoo = std::vector<uint8_t> {};
	file.learningCounts = std::vector<creaturemind::MindLearningCount>(k_LearningCounts, {.c = 0});
	file.drawnScale = 1.0f;
	file.unknown4 = 0.0f;
	file.unknown5 = std::array<int32_t, 2> {};
	return file;
}

/// The ids known, keeping the order a file listed them in and adding the newly known after
std::vector<creaturemind::MindKnownAction> KnownList(const std::vector<bool>& known,
                                                     const std::vector<creaturemind::MindKnownAction>& before)
{
	std::vector<creaturemind::MindKnownAction> list;
	for (const auto& action : before)
	{
		if (action.id < known.size() && known[action.id])
		{
			list.push_back({.id = action.id});
		}
	}
	for (uint32_t id = 0; id < known.size(); ++id)
	{
		if (known[id] && std::ranges::find(list, id, &creaturemind::MindKnownAction::id) == list.end())
		{
			list.push_back({.id = id});
		}
	}
	return list;
}

void Sightings(const std::vector<creature_watching::Sighting>& sightings, std::vector<creaturemind::MindSighting>& file)
{
	file.resize(std::max(file.size(), sightings.size()));
	for (size_t i = 0; i < sightings.size(); ++i)
	{
		file[i].count = sightings[i].count;
		file[i].turn = sightings[i].turn.value_or(file[i].turn.value_or(0));
	}
}
} // namespace

Learnt creature_mind_model::Fresh(const creature_desires::Desires& desires, size_t actionCount,
                                  creature_watching::Knowledge knowledge)
{
	Learnt learnt {
	    .opinions = std::vector<float>(actionCount, 0.0f),
	    .turnsSinceDone = std::vector<uint32_t>(actionCount, creature_planner::k_NoveltyTurns),
	    .knowledge = std::move(knowledge),
	};
	for (size_t d = 0; d < k_DesireCount; ++d)
	{
		for (const auto& source : desires.desires.at(d).sources)
		{
			learnt.initialThresholds.at(d).push_back(source.threshold);
		}
	}
	return learnt;
}

void creature_mind_model::Think(Learnt& learnt, std::string thought)
{
	learnt.thoughts.push_back(std::move(thought));
	if (learnt.thoughts.size() > k_MaxThoughts)
	{
		learnt.thoughts.erase(learnt.thoughts.begin());
	}
}

void creature_mind_model::Learn(Learnt& learnt, TreeKind kind, Desire desire, creature_tree::Episode episode,
                                std::span<const creature_tree::Attribute> allowed)
{
	const auto k = static_cast<size_t>(kind);
	const auto d = static_cast<size_t>(desire);
	auto& episodes = learnt.episodes.at(k).at(d);
	creature_tree::AddEpisode(episodes, std::move(episode));
	learnt.trees.at(k).at(d) = creature_tree::Build(episodes, allowed);
}

void creature_mind_model::RebuildTrees(Learnt& learnt,
                                       const std::array<std::vector<creature_tree::Attribute>, k_DesireCount>& allowed)
{
	for (size_t k = 0; k < k_TreeKinds; ++k)
	{
		for (size_t d = 0; d < k_DesireCount; ++d)
		{
			learnt.trees.at(k).at(d) = creature_tree::Build(learnt.episodes.at(k).at(d), allowed.at(d));
		}
	}
}

LoadedMind creature_mind_model::FromFile(const creaturemind::MindFileData& file, const creature_desires::Desires& fresh,
                                         const Learnt& freshLearnt)
{
	LoadedMind mind {.desires = fresh, .learnt = freshLearnt};
	for (size_t d = 0; d < k_DesireCount && d < file.desires.size(); ++d)
	{
		const auto& saved = file.desires[d];
		auto& state = mind.desires.desires.at(d);
		const auto start = state;
		state.activated = saved.activated != 0;
		state.value = saved.value;
		state.max = saved.max;
		state.increaseSeconds = saved.increaseSeconds;
		state.suppressedTurns = 0;
		state.sources.clear();
		for (const auto& source : saved.sources)
		{
			const auto same = std::ranges::find(start.sources, source.type, &creature_desires::Source::type);
			state.sources.push_back({
			    .type = source.type,
			    .value = source.value,
			    .threshold = source.threshold,
			    .multiplier = same != start.sources.end() ? same->multiplier : 1.0f,
			});
		}
	}
	for (const auto& tree : file.trees)
	{
		if (tree.type < 0 || tree.type >= static_cast<int32_t>(k_TreeKinds) || tree.desire < 0 ||
		    tree.desire >= static_cast<int32_t>(k_DesireCount))
		{
			continue;
		}
		auto& episodes = mind.learnt.episodes.at(static_cast<size_t>(tree.type)).at(static_cast<size_t>(tree.desire));
		episodes.clear();
		for (const auto& saved : tree.episodes)
		{
			creature_tree::AddEpisode(episodes,
			                          {
			                              .belief = creature_tree::FromSlots(saved.belief.type, saved.belief.attributes),
			                              .feedback = saved.feedback,
			                              .saved = {saved.kind, saved.action, saved.belief.x, saved.belief.z},
			                          });
		}
	}
	if (file.opinions.has_value())
	{
		for (size_t a = 0; a < mind.learnt.opinions.size() && a < file.opinions->size(); ++a)
		{
			mind.learnt.opinions[a] = std::clamp((*file.opinions)[a], -1.0f, 1.0f);
		}
	}
	auto& knowledge = mind.learnt.knowledge;
	for (const auto& action : file.known[0])
	{
		if (action.id < knowledge.skillsKnown.size())
		{
			knowledge.skillsKnown[action.id] = true;
		}
	}
	for (const auto& action : file.known[1])
	{
		if (action.id < knowledge.miraclesKnown.size())
		{
			knowledge.miraclesKnown[action.id] = true;
		}
	}
	for (size_t i = 0; i < knowledge.skillsSeen.size() && i < file.actionsSeen.size(); ++i)
	{
		knowledge.skillsSeen[i] = {.count = file.actionsSeen[i].count, .turn = file.actionsSeen[i].turn};
	}
	for (size_t i = 0; i < knowledge.miraclesSeen.size() && i < file.miraclesSeen.size(); ++i)
	{
		knowledge.miraclesSeen[i] = {.count = file.miraclesSeen[i].count, .turn = file.miraclesSeen[i].turn};
	}
	mind.learnt.name = file.name;
	mind.learnt.alignment = file.alignment.value_or(0.0f);
	mind.learnt.developmentTimer = file.developmentTimer;
	mind.learnt.file = std::make_shared<const creaturemind::MindFileData>(file);
	mind.developmentPhase = static_cast<uint32_t>(std::max(file.developmentPhase, 0));
	mind.attitudeToPlayer = std::clamp(file.attitudeToPlayer, -1.0f, 1.0f);
	mind.speciesRow = file.speciesRow;
	mind.strength = file.physique.strength;
	mind.size = file.physique.size;
	mind.tattoo = file.tattoo;
	return mind;
}

creaturemind::MindFileData creature_mind_model::ToFile(const creature_desires::Desires& desires, const Learnt& learnt,
                                                       uint32_t developmentPhase, float attitudeToPlayer, uint32_t speciesRow)
{
	const auto& knowledge = learnt.knowledge;
	auto file = learnt.file != nullptr ? *learnt.file : Skeleton(knowledge.skillsSeen.size(), knowledge.miraclesSeen.size());
	// Written as the current version, filling what older versions lacked
	if (file.version != creaturemind::k_CurrentVersion)
	{
		const auto skeleton = Skeleton(knowledge.skillsSeen.size(), knowledge.miraclesSeen.size());
		file.version = creaturemind::k_CurrentVersion;
		file.counters.resize(k_Counters, 0);
		for (auto& sighting : file.actionsSeen)
		{
			sighting.turn = sighting.turn.value_or(0);
		}
		for (auto& sighting : file.miraclesSeen)
		{
			sighting.turn = sighting.turn.value_or(0);
		}
		file.playerDesires = file.playerDesires.value_or(*skeleton.playerDesires);
		file.townDesires = file.townDesires.value_or(*skeleton.townDesires);
		file.unknown1 = file.unknown1.value_or(0);
		file.physique.scratch = file.physique.scratch.value_or(0.0f);
		file.physique.unknown3 = file.physique.unknown3.value_or(0);
		file.physique.size = file.physique.size.value_or(1.0f);
		file.database = file.database.value_or(*skeleton.database);
		file.unknown2 = file.unknown2.value_or(0);
		file.unknown3 = file.unknown3.value_or(*skeleton.unknown3);
		file.tattooHeader = file.tattooHeader.value_or(*skeleton.tattooHeader);
		file.tattoo = file.tattoo.value_or(*skeleton.tattoo);
		file.learningCounts = file.learningCounts.value_or(*skeleton.learningCounts);
		for (auto& count : *file.learningCounts)
		{
			count.c = count.c.value_or(0);
		}
		file.drawnScale = file.drawnScale.value_or(1.0f);
		file.unknown4 = file.unknown4.value_or(0.0f);
		file.unknown5 = file.unknown5.value_or(*skeleton.unknown5);
	}
	file.speciesRow = speciesRow;
	file.name = learnt.name;
	file.desires.resize(k_DesireCount);
	file.desireMemories.resize(k_DesireCount);
	for (size_t d = 0; d < k_DesireCount; ++d)
	{
		const auto& state = desires.desires.at(d);
		auto& saved = file.desires[d];
		saved.activated = state.activated ? 1 : 0;
		saved.value = state.value;
		saved.max = state.max;
		saved.increaseSeconds = state.increaseSeconds;
		saved.before7.reset();
		saved.from6To9.reset();
		saved.before15.reset();
		saved.sources.clear();
		for (const auto& source : state.sources)
		{
			saved.sources.push_back({.value = source.value, .threshold = source.threshold, .type = source.type});
		}
	}
	file.from6To9.reset();
	file.treeDesireCount = static_cast<int32_t>(k_DesireCount);
	file.trees.clear();
	for (size_t d = 0; d < k_DesireCount; ++d)
	{
		for (size_t k = 0; k < k_TreeKinds; ++k)
		{
			auto& tree = file.trees.emplace_back();
			tree.type = static_cast<int32_t>(k);
			tree.desire = static_cast<int32_t>(d);
			tree.desireAgain = static_cast<int32_t>(d);
			for (const auto& episode : learnt.episodes.at(k).at(d))
			{
				tree.episodes.push_back({
				    .kind = episode.saved[0],
				    .desire = static_cast<int32_t>(d),
				    .action = episode.saved[1],
				    .belief = {.type = episode.belief.type,
				               .x = episode.saved[2],
				               .z = episode.saved[3],
				               .attributes = creature_tree::ToSlots(episode.belief)},
				    .feedback = episode.feedback,
				});
			}
		}
	}
	file.opinions = learnt.opinions;
	Sightings(knowledge.skillsSeen, file.actionsSeen);
	Sightings(knowledge.miraclesSeen, file.miraclesSeen);
	file.attitudeToPlayer = attitudeToPlayer;
	file.known[0] = KnownList(knowledge.skillsKnown, file.known[0]);
	file.known[1] = KnownList(knowledge.miraclesKnown, file.known[1]);
	file.before11.reset();
	file.alignment = learnt.alignment;
	file.developmentTimer = learnt.developmentTimer;
	file.developmentPhase = static_cast<int32_t>(developmentPhase);
	return file;
}
