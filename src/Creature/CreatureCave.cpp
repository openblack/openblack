/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureCave.h"

#include <algorithm>
#include <ranges>

#include <fmt/format.h>

namespace openblack::creature_cave
{

namespace
{
/// The scroll's percentages, cut short as the game does
int32_t Percent(float value)
{
	return static_cast<int32_t>(value * 100.0f);
}

/// The actions-learnt scroll tells of the skills after the first (building), in their order
constexpr size_t k_FirstScrollSkill = 1;

/// The first of the game's table of miracles is none at all, which every creature knows
constexpr size_t k_FirstScrollMiracle = 1;

/// The game's texts for learning each miracle, by the miracle's place in its table: the first is "none"
constexpr size_t k_MagicTexts = 64;
std::string_view MagicActionText(size_t index)
{
	static const auto k_Names = [] {
		std::array<std::string, k_MagicTexts> names;
		for (size_t i = 0; i < names.size(); ++i)
		{
			names.at(i) = fmt::format("HELP_TEXT_CREATURE_LESSON_LEARN_MAGIC_ACTION_{:02}", i + 1);
		}
		return names;
	}();
	return k_Names.at(std::min(index, k_MagicTexts - 1));
}

constexpr std::array<std::pair<creature_desires::Desire, std::string_view>, 6> k_Personality {{
    {creature_desires::Desire::Hunger, "HELP_TEXT_ROOM_PERSONALITY_GREEDY"},
    {creature_desires::Desire::Tiredness, "HELP_TEXT_ROOM_PERSONALITY_LETHARGIC"},
    {creature_desires::Desire::Anger, "HELP_TEXT_ROOM_PERSONALITY_ANGRY"},
    {creature_desires::Desire::Compassion, "HELP_TEXT_ROOM_PERSONALITY_COMPASSIONATE"},
    {creature_desires::Desire::Fear, "HELP_TEXT_ROOM_PERSONALITY_TERRIFIED"},
    {creature_desires::Desire::Play, "HELP_TEXT_ROOM_PERSONALITY_PLAYFUL"},
}};
} // namespace

std::optional<std::string_view> PersonalityText(creature_desires::Desire desire)
{
	const auto found = std::ranges::find(k_Personality, desire, &std::pair<creature_desires::Desire, std::string_view>::first);
	return found != k_Personality.end() ? std::optional(found->second) : std::nullopt;
}

TempleScrolls::CreatureFacts FactsOf(const Snapshot& snapshot)
{
	const auto& needs = snapshot.needs;
	TempleScrolls::CreatureFacts facts;
	facts.name = snapshot.name;
	// The scroll gives the body's age, in hours of game time, as its years
	facts.age = static_cast<int32_t>(needs.age);
	facts.health = Percent(needs.life);
	facts.energy = Percent(needs.energy);
	facts.strength = Percent(snapshot.strength);
	facts.fatness = Percent(snapshot.fatness);
	facts.exhaustion = Percent(needs.exhaustion);
	facts.dehydration = Percent(needs.dehydration);
	facts.poo = Percent(needs.poo);
	facts.alignment = std::clamp(snapshot.alignment, -1.0f, 1.0f);
	facts.warmth = std::clamp(needs.warmth, -1.0f, 1.0f);
	for (size_t i = 0; i < facts.actionsKnown.size(); ++i)
	{
		const auto skill = i + k_FirstScrollSkill;
		facts.actionsKnown.at(i) = skill < snapshot.skills.size() && snapshot.skills.at(skill).known;
	}
	// The personality, in the scroll's order
	for (const auto& [desire, text] : k_Personality)
	{
		const auto like = std::ranges::find(snapshot.likes, desire, &Snapshot::Like::desire);
		if (like != snapshot.likes.end())
		{
			facts.desires.push_back({.text = text, .attitude = static_cast<int32_t>(like->level)});
		}
	}
	facts.opinionOfGod = std::clamp(snapshot.attitudeToPlayer, -1.0f, 1.0f);
	facts.attention = std::clamp(1.0f - (snapshot.secondsAlone / k_AttentionFadeSeconds), 0.0f, 1.0f);
	facts.known = {static_cast<int32_t>(snapshot.creaturesKnown), 0, 0, 0};
	// The miracles it knows or has started to learn, by the game's texts for them
	for (size_t i = k_FirstScrollMiracle; i < snapshot.miracles.size(); ++i)
	{
		const auto percent = std::clamp(snapshot.miracles.at(i).percent, 0, 100);
		if (percent > 0)
		{
			facts.miracles.push_back({.text = MagicActionText(i), .percent = percent});
		}
	}
	return facts;
}

std::string_view TitleName(Page page)
{
	constexpr std::array<std::string_view, k_PageCount> k_Titles {
	    "HELP_TEXT_ROOM_CREATURE_SCROLL_TITLE", "HELP_TEXT_ROOM_ACTIONS_LEARNT_SCROLL_TITLE",
	    "HELP_TEXT_ROOM_LIKES_SCROLL_TITLE",    "HELP_TEXT_ROOM_MAGIC_SCROLL_TITLE",
	    "HELP_TEXT_DIALOG_CREATURETATOO",
	};
	return k_Titles.at(static_cast<size_t>(page));
}

std::optional<TempleScrolls::Content> ContentOf(Page page)
{
	using Content = TempleScrolls::Content;
	switch (page)
	{
	case Page::Attributes:
		return Content::CreatureAttributes;
	case Page::ActionsLearnt:
		return Content::CreatureActions;
	case Page::Mind:
		return Content::CreatureMind;
	case Page::Miracles:
		return Content::CreatureMiracles;
	default:
		return std::nullopt;
	}
}

Page Next(Page page)
{
	return static_cast<Page>((static_cast<size_t>(page) + 1) % k_PageCount);
}

Page Previous(Page page)
{
	return static_cast<Page>((static_cast<size_t>(page) + k_PageCount - 1) % k_PageCount);
}

Lessons LessonsOf(std::span<const Snapshot::Opinion> opinions, size_t most)
{
	Lessons lessons;
	for (const auto& opinion : opinions)
	{
		if (opinion.opinion >= k_LessonThreshold)
		{
			lessons.toDo.push_back(opinion);
		}
		else if (opinion.opinion <= -k_LessonThreshold)
		{
			lessons.notToDo.push_back(opinion);
		}
	}
	std::ranges::stable_sort(lessons.toDo, std::ranges::greater {}, &Snapshot::Opinion::opinion);
	std::ranges::stable_sort(lessons.notToDo, std::ranges::less {}, &Snapshot::Opinion::opinion);
	lessons.toDo.resize(std::min(lessons.toDo.size(), most));
	lessons.notToDo.resize(std::min(lessons.notToDo.size(), most));
	return lessons;
}

std::optional<TattooEdit> Apply(const creature_tattoo::Slots& slots, uint8_t site, uint8_t design, glm::u8vec3 colour)
{
	if (site >= creature_tattoo::k_SlotCount || design >= creature_tattoo::k_DesignCount)
	{
		return std::nullopt;
	}
	const auto slot = creature_tattoo::SlotFor(slots, site, design);
	if (!slot.has_value())
	{
		return std::nullopt;
	}
	return TattooEdit {.slot = *slot, .tattoo = {.design = design, .site = site, .colour = colour}};
}

std::optional<TattooEdit> Remove(const creature_tattoo::Slots& slots, uint8_t site)
{
	const auto found = std::ranges::find_if(slots, [site](const creature_tattoo::Slot& slot) { return slot.site == site; });
	if (found == slots.end())
	{
		return std::nullopt;
	}
	return TattooEdit {.slot = static_cast<size_t>(std::distance(slots.begin(), found)), .tattoo = {}};
}

} // namespace openblack::creature_cave
