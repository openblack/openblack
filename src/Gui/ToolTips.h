/*******************************************************************************
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
#include <string>

namespace openblack::gui
{

/// The arrows a tooltip shows about its mouse, for the ways the mouse can be dragged (KEYALIGN)
namespace ToolTipArrows
{
constexpr uint32_t k_None = 0;
constexpr uint32_t k_Up = 0x100;
constexpr uint32_t k_Down = 0x200;
constexpr uint32_t k_Left = 0x400;
constexpr uint32_t k_Right = 0x800;
constexpr uint32_t k_UpDown = k_Up | k_Down;
constexpr uint32_t k_All = k_UpDown | k_Left | k_Right;
} // namespace ToolTipArrows

/// The action a tooltip's mouse shows the button of, or none for a tooltip of just words
enum class ToolTipAction : uint8_t
{
	None,
	/// SELECT, the left button
	Select,
	/// APPLY, the right button
	Apply,
};

/// How many tooltips the player has asked to see (the profile's TOOLTIP_LEVEL)
enum class ToolTipLevel : uint8_t
{
	None,
	/// Only the ones that matter most: numbers and the like
	Minimum,
	/// Each fades in slower each time it is shown, and fades out once it has been read
	Intelligent,
	All,
};

/// A tooltip's place in the game's 170 (HELP_TEXT_TOOLTIP_01 onwards, from text 0xE73), and its priority and how long it
/// is shown for, from the info script (GHelpSystemTooltipsInfo)
struct ToolTipInfo
{
	float priority;
	float displayTime;
	float displayTimeAfterFocus;
};

/// The words, mouse and arrows the hand shows for what it is over.
///
/// What the hand is over submits a tooltip every turn it is over it (ToolTips::SubmitToolTips), and once a turn the help
/// system lets the tooltip live on or ends it (HelpSystem::ProcessToolTips). The one shown fades in and out every
/// frame (KMIcon::Draw).
class ToolTips
{
public:
	static constexpr uint32_t k_Count = 170;
	/// Tooltips at least this high in priority are "important": numbers and stats, always shown
	static constexpr float k_ImportantPriority = 0.9f;
	/// The turns a second the counters of how long a tooltip is shown are in
	static constexpr float k_TurnsPerSecond = 25.0f;
	/// The most seconds a tooltip ever takes to fade in
	static constexpr uint32_t k_MaxTimesShown = 80;

	explicit ToolTips(std::span<const ToolTipInfo, k_Count> info);

	void SetLevel(ToolTipLevel level) noexcept { _level = level; }
	[[nodiscard]] ToolTipLevel GetLevel() const noexcept { return _level; }

	/// The tooltip of an index for this turn. A forced one shows at once, even over one still being shown.
	void Submit(uint32_t index, ToolTipAction action, uint32_t arrows, bool force = false);
	/// Ends the turn: keeps the tooltip submitted, or one lingering after it was, and ends any other
	void ProcessTurn();
	/// Fades the shown tooltip in or out
	void Update(float seconds);

	struct Shown
	{
		uint32_t index;
		ToolTipAction action;
		uint32_t arrows;
		/// From 0 to 1
		float alpha;
	};
	[[nodiscard]] std::optional<Shown> GetShown() const;

	/// The name of a tooltip's text in the info scripts
	[[nodiscard]] static std::string TextName(uint32_t index);

private:
	/// KMIcon: the tooltip being drawn, and its fade from start to end over fadeTime seconds
	struct Icon
	{
		uint32_t index;
		ToolTipAction action;
		uint32_t arrows;
		float start;
		float end;
		float fadeTime;
		float current;
		float elapsed;
	};

	/// ToolTips::SmartIcon: whether the intelligent level keeps the icon, which fades out once shown for long enough
	bool KeepIntelligently();
	[[nodiscard]] float PriorityOf(uint32_t index) const { return _info.at(index).priority; }

	std::array<ToolTipInfo, k_Count> _info;
	ToolTipLevel _level {ToolTipLevel::Intelligent};
	std::optional<uint32_t> _current;
	ToolTipAction _action {ToolTipAction::None};
	uint32_t _arrows {ToolTipArrows::k_None};
	/// Turns left of the tooltip's display time, and of its lingering once no longer submitted
	int32_t _displayLeft {0};
	int32_t _lingerLeft {0};
	bool _submitted {false};
	bool _important {false};
	/// How many times each tooltip has been shown afresh, which is how many seconds it takes to fade in
	std::array<uint32_t, k_Count> _timesShown {};
	std::optional<Icon> _icon;
};

} // namespace openblack::gui
