/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureStatusPanel.h"

#include <cmath>

#include <algorithm>
#include <string>

using namespace openblack;
using namespace openblack::creature_panel;

namespace
{
/// The box sits a fortieth of the screen in from its left, and is a quarter of the screen wide
constexpr float k_BoxInset = 1.0f / 40.0f;
constexpr float k_BoxWidth = 1.0f / 4.0f;
/// The rows are a sixteenth of the screen's height apart
constexpr float k_RowsPerScreen = 16.0f;
} // namespace

float creature_panel::Clamp01(float value)
{
	if (!(value > 0.0f))
	{
		return 0.0f;
	}
	if (!(value < 1.0f))
	{
		return 1.0f;
	}
	return value;
}

float creature_panel::ClampReward(float reward)
{
	if (!(reward > -1.0f))
	{
		return -1.0f;
	}
	if (!(reward < 1.0f))
	{
		return 1.0f;
	}
	return reward;
}

Values creature_panel::FromNeeds(const creature_physiology::Needs& needs, std::optional<float> reward)
{
	return {
	    .damage = Clamp01(1.0f - needs.life),
	    .hunger = Clamp01(1.0f - needs.energy),
	    .tiredness = Clamp01(needs.exhaustion),
	    .reward = reward.has_value() ? std::optional(ClampReward(*reward)) : std::nullopt,
	};
}

int creature_panel::Percent(float value)
{
	return static_cast<int>(value * 100.0f);
}

std::u16string creature_panel::FormatNumber(std::u16string_view format, int number)
{
	std::u16string text;
	for (size_t i = 0; i < format.size(); ++i)
	{
		if (format[i] == u'%' && i + 1 < format.size())
		{
			if (format[i + 1] == u'd')
			{
				for (const auto digit : std::to_string(number))
				{
					text.push_back(static_cast<char16_t>(digit));
				}
				++i;
				continue;
			}
			if (format[i + 1] == u'%')
			{
				text.push_back(u'%');
				++i;
				continue;
			}
		}
		text.push_back(format[i]);
	}
	return text;
}

std::u16string creature_panel::FormatPercent(float value)
{
	return FormatNumber(u"%d%%", Percent(value));
}

RewardKind creature_panel::Classify(float reward)
{
	if (reward < -k_NoRewardBelow)
	{
		return RewardKind::Bad;
	}
	if (reward > k_NoRewardBelow)
	{
		return RewardKind::Good;
	}
	return RewardKind::None;
}

std::u16string creature_panel::FormatReward(float reward, const RewardTexts& texts)
{
	switch (Classify(reward))
	{
	case RewardKind::Bad:
		return FormatNumber(texts.bad, Percent(-reward));
	case RewardKind::Good:
		return FormatNumber(texts.good, Percent(reward));
	case RewardKind::None:
	default:
		return std::u16string(texts.none) + u" 0%";
	}
}

float creature_panel::Fill(Row row, const Values& values)
{
	switch (row)
	{
	case Row::Damage:
		return values.damage;
	case Row::Hunger:
		return values.hunger;
	case Row::Tiredness:
		return values.tiredness;
	case Row::Reward:
	default:
		return (values.reward.value_or(0.0f) * 0.5f) + 0.5f;
	}
}

glm::vec4 creature_panel::BarColour(Row row, float fill)
{
	if (row != Row::Reward)
	{
		return {1.0f, 1.0f, 0.0f, 1.0f};
	}
	return fill < 0.5f ? glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
}

std::u16string creature_panel::FormatValue(Row row, const Values& values, const RewardTexts& texts)
{
	return row == Row::Reward ? FormatReward(values.reward.value_or(0.0f), texts) : FormatPercent(Fill(row, values));
}

float creature_panel::LabelColumnWidth(std::span<const float> labelWidths, float size)
{
	const auto widest = labelWidths.empty() ? 0.0f : *std::ranges::max_element(labelWidths);
	return widest + (size * 0.5f);
}

Layout creature_panel::Compute(glm::u16vec2 resolution, bool withReward, float labelColumnWidth)
{
	const auto screen = glm::vec2(resolution);
	Layout layout;
	layout.size = screen.y / k_RowsPerScreen;
	layout.textSize = layout.size * 0.5f;
	const float x0 = screen.x * k_BoxInset;
	const float x1 = (screen.x * k_BoxWidth) + x0;
	// With the reward a quarter of the way down; following a creature near the top, three quarters as high
	const float y0 = withReward ? screen.y / 4.0f : screen.y * k_BoxInset;
	const float height = withReward ? screen.y / 3.0f : 0.75f * (screen.y / 3.0f);
	layout.box = {.min = {x0, y0}, .max = {x1, y0 + height}};

	layout.rowCount = withReward ? k_RowCount : k_RowCount - 1;
	const float gap = layout.size * 0.25f;
	for (size_t i = 0; i < layout.rowCount; ++i)
	{
		const auto row = static_cast<Row>(i);
		// A third of a row more before the reward
		float y = y0 + (2.0f * layout.size / 3.0f) + (static_cast<float>(i) * layout.size);
		if (row == Row::Reward)
		{
			y += layout.size / 3.0f;
		}
		const Rect bar {.min = {x0 + labelColumnWidth, y}, .max = {x0 + (2.0f * labelColumnWidth), y + layout.textSize}};
		layout.rows.at(i) = {
		    .row = row,
		    .bar = bar,
		    .labelRight = {bar.min.x - gap, y},
		    .valueLeft = {bar.max.x + gap, y},
		};
	}
	return layout;
}

BarFill creature_panel::FillOf(const Rect& bar, float fill, bool fromMiddle)
{
	// The fill runs inside the frame, from its left edge or its middle to how full the bar is, in whole pixels
	const float left = bar.min.x + k_BarInset;
	const float right = bar.max.x - k_BarInset;
	const float end = std::clamp(std::trunc(left + ((right - left) * Clamp01(fill))), left, right);
	BarFill result {.from = left, .to = end, .top = bar.min.y + k_BarInset, .bottom = bar.max.y - k_BarInset};
	if (fromMiddle)
	{
		const float middle = std::trunc(left + ((right - left) * 0.5f));
		result.from = middle;
		result.middle = middle;
	}
	return result;
}
