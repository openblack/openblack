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

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Creature/CreaturePhysiology.h"

/// The creature's status panel: while the hand is over a creature, any player's, the left of the screen shows how
/// damaged, hungry and tired it is as bars, and below them the reward the hand has given it so far, from a slap to a
/// stroke. While the camera follows a creature the panel shows the first three near the top of the screen instead.
/// It is drawn afresh every frame it is wanted, without fading. Everything here is pure: the values, their words and
/// the panel's layout in screen pixels.
namespace openblack::creature_panel
{

/// What brings the panel up over a creature: the game shows it whenever the hand is over one, but it can be made to
/// need the right button held as well
enum class Trigger : uint8_t
{
	Hover,
	RightButtonHeld,
};
constexpr Trigger k_Trigger = Trigger::Hover;
/// Whether the panel shows for a creature under the hand, with the right button held or not
[[nodiscard]] constexpr bool Triggered(bool rightButtonHeld)
{
	return k_Trigger == Trigger::Hover || rightButtonHeld;
}

/// The panel's rows, top to bottom
enum class Row : uint8_t
{
	Damage,
	Hunger,
	Tiredness,
	Reward,
};
constexpr size_t k_RowCount = 4;

/// The names of the rows' labels and the reward's words in the game's info scripts
constexpr std::array<std::string_view, k_RowCount> k_LabelNames {
    "HELP_TEXT_DIALOG_ADDITION_48",  // Damage
    "HELP_TEXT_DIALOG_ADDITION_68",  // Hunger
    "HELP_TEXT_DIALOG_ADDITION_137", // Tiredness
    "HELP_TEXT_DIALOG_ADDITION_118", // Reward
};
constexpr std::string_view k_BadBoyName = "HELP_TEXT_DIALOG_ADDITION_23";
constexpr std::string_view k_GoodBoyName = "HELP_TEXT_DIALOG_ADDITION_65";
constexpr std::string_view k_NoRewardName = "HELP_TEXT_DIALOG_ADDITION_99";
/// The hand's "Interact" tooltip, shown over the player's own creature
constexpr uint32_t k_InteractToolTip = 18;

/// Rewards nearer nothing than this read as no reward
constexpr float k_NoRewardBelow = 0.01f;

/// What the panel shows, each from 0 to 1 and the reward from -1 (slapped) to 1 (stroked); no reward row without one
struct Values
{
	float damage {0.0f};
	float hunger {0.0f};
	float tiredness {0.0f};
	std::optional<float> reward;
};

/// From 0 to 1, as the game clamps: anything not above 0, nonsense included, is 0, anything not below 1 is 1
[[nodiscard]] float Clamp01(float value);
/// From -1 to 1 the same way
[[nodiscard]] float ClampReward(float reward);
/// The panel's values from a body: damage is the life lost, hunger the energy missing (a big meal fills it past 1, and
/// then it is 0) and tiredness the exhaustion; the reward when the hand gives one
[[nodiscard]] Values FromNeeds(const creature_physiology::Needs& needs, std::optional<float> reward);

/// A value as a whole percentage, cut short rather than rounded: 0.0399 is 3
[[nodiscard]] int Percent(float value);
/// "N%"
[[nodiscard]] std::u16string FormatPercent(float value);
/// A text with "%d" replaced by a number and "%%" by "%", as the scripts' texts give them
[[nodiscard]] std::u16string FormatNumber(std::u16string_view format, int number);

enum class RewardKind : uint8_t
{
	Bad,
	Good,
	None,
};
[[nodiscard]] RewardKind Classify(float reward);
/// The scripts' words for the reward: "Bad Boy! %d%%", "Good Boy! %d%%" and "No Reward"
struct RewardTexts
{
	std::u16string_view bad;
	std::u16string_view good;
	std::u16string_view none;
};
/// "Bad Boy! 37%", "Good Boy! 20%", or "No Reward 0%"
[[nodiscard]] std::u16string FormatReward(float reward, const RewardTexts& texts);

/// How full a row's bar is, 0 to 1; the reward's bar is half full with no reward
[[nodiscard]] float Fill(Row row, const Values& values);
/// The colour a row's bar fills with: yellow, but the reward's red while it is a punishment and green otherwise
[[nodiscard]] glm::vec4 BarColour(Row row, float fill);
/// The text of a row's value
[[nodiscard]] std::u16string FormatValue(Row row, const Values& values, const RewardTexts& texts);

/// A rectangle in screen pixels, from min to max
struct Rect
{
	glm::vec2 min {0.0f};
	glm::vec2 max {0.0f};
};

struct RowLayout
{
	Row row {Row::Damage};
	Rect bar;
	/// The top right of the label, and the top left of the value
	glm::vec2 labelRight {0.0f};
	glm::vec2 valueLeft {0.0f};
};

struct Layout
{
	/// The see-through black box behind the panel, which fades out to its right
	Rect box;
	/// The rows' pitch, and the text's height, half of it
	float size {0.0f};
	float textSize {0.0f};
	std::array<RowLayout, k_RowCount> rows {};
	size_t rowCount {0};

	[[nodiscard]] std::span<const RowLayout> Rows() const { return std::span(rows).first(rowCount); }
};

/// The width the labels' column takes: the widest label, at the text's size, and half a row more
[[nodiscard]] float LabelColumnWidth(std::span<const float> labelWidths, float size);
/// The panel's layout on a screen: with the reward a third of the screen high and a quarter down it, without it three
/// quarters as high and near the top. The bars are as wide as the labels' column.
[[nodiscard]] Layout Compute(glm::u16vec2 resolution, bool withReward, float labelColumnWidth);

/// The part of a bar that is filled: from one side to the other, the bright end where it ends. The reward's bar fills
/// from its middle, either way, and has a line down its middle.
struct BarFill
{
	/// The dim end and the bright end, across the screen; nothing to fill when they meet
	float from {0.0f};
	float to {0.0f};
	float top {0.0f};
	float bottom {0.0f};
	std::optional<float> middle;

	[[nodiscard]] bool Empty() const { return from == to; }
};
/// The bars' frame is this many pixels thick inside their rectangle
constexpr float k_BarInset = 3.0f;
[[nodiscard]] BarFill FillOf(const Rect& bar, float fill, bool fromMiddle);

} // namespace openblack::creature_panel
