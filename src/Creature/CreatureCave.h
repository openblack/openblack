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
#include <string_view>
#include <vector>

#include <glm/gtc/type_precision.hpp>

#include "3D/TempleScrolls.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreaturePhysiology.h"
#include "Creature/CreatureTattoo.h"

/// The Creature Cave, the temple's room for the player's creature, which F5 goes to. Its four scrolls tell of the
/// creature: its attributes, the actions it has learnt, its mind and the miracles it can cast; and clicking the creature
/// opens the tattoo editor, which drags the sixteen symbols onto the eight places on its body or off them. Everything
/// here is pure: what the scrolls are told of the creature, the pages the cave screen shows them on, and the tattoos'
/// edits.
namespace openblack::creature_cave
{

/// What the cave knows of the player's creature, as plain data gathered from its components
struct Snapshot
{
	std::u16string name;
	creature_physiology::Needs needs;
	float strength {0.5f};
	float fatness {0.5f};
	/// -1 evil to 1 good
	float alignment {0.0f};
	/// Each activated desire, and how much it likes it: 0 extremely to 10 not at all
	struct Like
	{
		creature_desires::Desire desire;
		size_t level;
	};
	std::vector<Like> likes;
	/// What it thinks of its god, -1 to 1, and seconds since the god last paid it any attention
	float attitudeToPlayer {0.0f};
	float secondsAlone {0.0f};
	/// It thinks it knows its god's dominant desire
	bool knowsGodsDesire {false};
	/// Other creatures it knows of
	size_t creaturesKnown {0};
	/// The opinion of each action, -1 to 1, by its name
	struct Opinion
	{
		std::string action;
		float opinion;
	};
	std::vector<Opinion> opinions;
	/// The ordinary skills it can learn by watching, and the miracles, with how far it has learnt each
	struct Skill
	{
		std::string name;
		bool known;
	};
	std::vector<Skill> skills;
	struct Miracle
	{
		std::string name;
		/// 0 to 100
		int32_t percent;
	};
	std::vector<Miracle> miracles;
};

/// The god's attention fades over this many seconds of leaving the creature alone
constexpr float k_AttentionFadeSeconds = 300.0f;

/// The scrolls' facts about the creature: the percentages cut short as the game does, the age in its hours of game time
/// (which the scroll calls years), and the personality's desires with how much it feels each
[[nodiscard]] TempleScrolls::CreatureFacts FactsOf(const Snapshot& snapshot);

/// The personality text of a desire the mind scroll tells of, none for the others
[[nodiscard]] std::optional<std::string_view> PersonalityText(creature_desires::Desire desire);

/// The pages of the cave screen: the four scrolls, then the tattoos
enum class Page : uint8_t
{
	Attributes,
	ActionsLearnt,
	Mind,
	Miracles,
	Tattoos,

	_Count
};
constexpr size_t k_PageCount = static_cast<size_t>(Page::_Count);
/// The page's title in the game's texts
[[nodiscard]] std::string_view TitleName(Page page);
/// The scroll whose text a page shows, none for the tattoos
[[nodiscard]] std::optional<TempleScrolls::Content> ContentOf(Page page);
/// The pages either side, going round
[[nodiscard]] Page Next(Page page);
[[nodiscard]] Page Previous(Page page);

/// The lessons the creature has learnt: the actions it thinks well of, best first, and those it has learnt not to do,
/// worst first, at most so many of each; opinions this near nothing are no lesson
constexpr float k_LessonThreshold = 0.05f;
struct Lessons
{
	std::vector<Snapshot::Opinion> toDo;
	std::vector<Snapshot::Opinion> notToDo;
};
[[nodiscard]] Lessons LessonsOf(std::span<const Snapshot::Opinion> opinions, size_t most);

/// The names of the eight places for a tattoo in the tattoo editor, in the order of the creature's sites
constexpr std::array<std::string_view, creature_tattoo::k_SlotCount> k_SiteNames {
    "HELP_TEXT_DIALOG_TATOOBACK",     "HELP_TEXT_DIALOG_TATOOHEAD",      "HELP_TEXT_DIALOG_TATOOCHEST",
    "HELP_TEXT_DIALOG_TATOOBUM",      "HELP_TEXT_DIALOG_TATOOLEFTARM",   "HELP_TEXT_DIALOG_TATOORIGHTARM",
    "HELP_TEXT_DIALOG_TATOOLEFTHAND", "HELP_TEXT_DIALOG_TATOORIGHTHAND",
};

/// An edit of the tattoos: the slot that changes and what it holds afterwards
struct TattooEdit
{
	size_t slot;
	creature_tattoo::Slot tattoo;
};
/// Dragging a symbol onto a place: it goes in the slot the game's editor would put it in, none when every slot holds a
/// tattoo on another place
[[nodiscard]] std::optional<TattooEdit> Apply(const creature_tattoo::Slots& slots, uint8_t site, uint8_t design,
                                              glm::u8vec3 colour);
/// Dragging the symbols off a place: the slot emptied, none when nothing is there
[[nodiscard]] std::optional<TattooEdit> Remove(const creature_tattoo::Slots& slots, uint8_t site);

/// The cave screen as the player leaves it: whether it is shown, the page, and the tattoo being put on
struct Screen
{
	bool open {false};
	Page page {Page::Attributes};
	/// The page was chosen other than by its tab, which the screen then turns to
	bool pageRequested {false};
	uint8_t design {0};
	uint8_t site {0};
	glm::u8vec3 colour {200, 40, 40};
};

} // namespace openblack::creature_cave
