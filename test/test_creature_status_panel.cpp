/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <limits>
#include <string>

#include <gtest/gtest.h>

#include "Creature/CreatureStatusPanel.h"

using namespace openblack;
using namespace openblack::creature_panel;

namespace
{
constexpr float k_Epsilon = 1e-4f;
const RewardTexts k_Texts {.bad = u"Bad Boy! %d%%", .good = u"Good Boy! %d%%", .none = u"No Reward"};
} // namespace

TEST(CreatureStatusPanel, ValuesComeFromTheBody)
{
	creature_physiology::Needs needs {};
	needs.life = 1.0f;
	needs.energy = 0.965f;
	needs.exhaustion = 0.32f;
	const auto values = FromNeeds(needs, 0.2f);
	EXPECT_FLOAT_EQ(values.damage, 0.0f);
	EXPECT_NEAR(values.hunger, 0.035f, k_Epsilon);
	EXPECT_FLOAT_EQ(values.tiredness, 0.32f);
	ASSERT_TRUE(values.reward.has_value());
	EXPECT_FLOAT_EQ(*values.reward, 0.2f);
	EXPECT_EQ(FormatPercent(values.damage), u"0%");
	EXPECT_EQ(FormatPercent(values.hunger), u"3%");
	EXPECT_EQ(FormatPercent(values.tiredness), u"32%");
	// Cut short in single precision, as the game is: 0.97 energy leaves 0.0299999 hunger, which shows as 2%
	needs.energy = 0.97f;
	EXPECT_EQ(FormatPercent(FromNeeds(needs, std::nullopt).hunger), u"2%");

	// A big meal fills it past full, which is no hunger; no reward without the hand
	needs.energy = 1.4f;
	needs.life = 0.25f;
	const auto fed = FromNeeds(needs, std::nullopt);
	EXPECT_FLOAT_EQ(fed.hunger, 0.0f);
	EXPECT_FLOAT_EQ(fed.damage, 0.75f);
	EXPECT_FALSE(fed.reward.has_value());
}

TEST(CreatureStatusPanel, ValuesAreClampedAsTheGameDoes)
{
	EXPECT_FLOAT_EQ(Clamp01(-0.5f), 0.0f);
	EXPECT_FLOAT_EQ(Clamp01(1.5f), 1.0f);
	EXPECT_FLOAT_EQ(Clamp01(0.5f), 0.5f);
	EXPECT_FLOAT_EQ(Clamp01(std::numeric_limits<float>::quiet_NaN()), 0.0f);
	EXPECT_FLOAT_EQ(ClampReward(-3.0f), -1.0f);
	EXPECT_FLOAT_EQ(ClampReward(3.0f), 1.0f);
	EXPECT_FLOAT_EQ(ClampReward(-0.4f), -0.4f);
	EXPECT_FLOAT_EQ(*FromNeeds({}, -2.0f).reward, -1.0f);
}

TEST(CreatureStatusPanel, PercentagesAreCutShort)
{
	EXPECT_EQ(Percent(0.0399f), 3);
	EXPECT_EQ(Percent(0.999f), 99);
	EXPECT_EQ(Percent(1.0f), 100);
	EXPECT_EQ(Percent(0.0f), 0);
	EXPECT_EQ(FormatNumber(u"%d%%", 42), u"42%");
	EXPECT_EQ(FormatNumber(u"Good Boy! %d%%", 7), u"Good Boy! 7%");
	// Anything else is left as it is
	EXPECT_EQ(FormatNumber(u"100% %s", 1), u"100% %s");
}

TEST(CreatureStatusPanel, RewardWords)
{
	EXPECT_EQ(Classify(0.005f), RewardKind::None);
	EXPECT_EQ(Classify(-0.01f), RewardKind::None);
	EXPECT_EQ(Classify(0.011f), RewardKind::Good);
	EXPECT_EQ(Classify(-0.011f), RewardKind::Bad);
	EXPECT_EQ(FormatReward(0.005f, k_Texts), u"No Reward 0%");
	EXPECT_EQ(FormatReward(-0.37f, k_Texts), u"Bad Boy! 37%");
	EXPECT_EQ(FormatReward(0.3f, k_Texts), u"Good Boy! 30%");
	EXPECT_EQ(FormatReward(1.0f, k_Texts), u"Good Boy! 100%");

	const Values values {.damage = 0.25f, .hunger = 0.5f, .tiredness = 0.0f, .reward = -0.5f};
	EXPECT_EQ(FormatValue(Row::Damage, values, k_Texts), u"25%");
	EXPECT_EQ(FormatValue(Row::Reward, values, k_Texts), u"Bad Boy! 50%");
}

TEST(CreatureStatusPanel, BarsFillAndColour)
{
	const Values values {.damage = 0.25f, .hunger = 0.5f, .tiredness = 1.0f, .reward = -0.5f};
	EXPECT_FLOAT_EQ(Fill(Row::Damage, values), 0.25f);
	EXPECT_FLOAT_EQ(Fill(Row::Tiredness, values), 1.0f);
	// The reward's bar is half full with none, empty slapped as hard as can be and full stroked
	EXPECT_FLOAT_EQ(Fill(Row::Reward, values), 0.25f);
	EXPECT_FLOAT_EQ(Fill(Row::Reward, Values {}), 0.5f);
	EXPECT_EQ(BarColour(Row::Hunger, 0.1f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
	EXPECT_EQ(BarColour(Row::Reward, 0.49f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	EXPECT_EQ(BarColour(Row::Reward, 0.5f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
}

TEST(CreatureStatusPanel, BarFillRunsInsideTheFrame)
{
	const Rect bar {.min = {100.0f, 50.0f}, .max = {206.0f, 70.0f}};
	// Inside the three pixel frame: 100 pixels to fill
	const auto empty = FillOf(bar, 0.0f, false);
	EXPECT_TRUE(empty.Empty());
	EXPECT_FLOAT_EQ(empty.from, 103.0f);
	EXPECT_FLOAT_EQ(empty.top, 53.0f);
	EXPECT_FLOAT_EQ(empty.bottom, 67.0f);
	EXPECT_FALSE(empty.middle.has_value());
	EXPECT_FLOAT_EQ(FillOf(bar, 0.032f, false).to, 106.0f);
	EXPECT_FLOAT_EQ(FillOf(bar, 1.0f, false).to, 203.0f);
	EXPECT_FLOAT_EQ(FillOf(bar, 2.0f, false).to, 203.0f);

	// The reward fills from the middle either way, and none fills nothing
	const auto none = FillOf(bar, 0.5f, true);
	ASSERT_TRUE(none.middle.has_value());
	EXPECT_FLOAT_EQ(*none.middle, 153.0f);
	EXPECT_TRUE(none.Empty());
	const auto bad = FillOf(bar, 0.25f, true);
	EXPECT_FLOAT_EQ(bad.from, 153.0f);
	EXPECT_FLOAT_EQ(bad.to, 128.0f);
	EXPECT_FLOAT_EQ(FillOf(bar, 0.75f, true).to, 178.0f);
}

TEST(CreatureStatusPanel, LayoutWithTheReward)
{
	// 1024 by 768: rows 48 apart, text 24 high
	const std::array<float, 4> labels {70.0f, 66.0f, 92.0f, 68.0f};
	const auto column = LabelColumnWidth(labels, 48.0f);
	EXPECT_FLOAT_EQ(column, 116.0f);
	const auto layout = Compute({1024, 768}, true, column);
	EXPECT_FLOAT_EQ(layout.size, 48.0f);
	EXPECT_FLOAT_EQ(layout.textSize, 24.0f);
	EXPECT_FLOAT_EQ(layout.box.min.x, 25.6f);
	EXPECT_FLOAT_EQ(layout.box.max.x, 281.6f);
	EXPECT_FLOAT_EQ(layout.box.min.y, 192.0f);
	EXPECT_FLOAT_EQ(layout.box.max.y, 448.0f);
	ASSERT_EQ(layout.Rows().size(), 4u);

	const auto& damage = layout.Rows()[0];
	EXPECT_EQ(damage.row, Row::Damage);
	EXPECT_FLOAT_EQ(damage.bar.min.x, 141.6f);
	EXPECT_FLOAT_EQ(damage.bar.max.x, 257.6f);
	EXPECT_FLOAT_EQ(damage.bar.min.y, 224.0f);
	EXPECT_FLOAT_EQ(damage.bar.max.y, 248.0f);
	EXPECT_FLOAT_EQ(damage.labelRight.x, 129.6f);
	EXPECT_FLOAT_EQ(damage.valueLeft.x, 269.6f);
	EXPECT_FLOAT_EQ(damage.labelRight.y, 224.0f);

	// A row apart, and a third of a row more before the reward
	EXPECT_FLOAT_EQ(layout.Rows()[2].bar.min.y, 320.0f);
	EXPECT_EQ(layout.Rows()[3].row, Row::Reward);
	EXPECT_FLOAT_EQ(layout.Rows()[3].bar.min.y, 384.0f);
}

TEST(CreatureStatusPanel, LayoutFollowingACreature)
{
	// 640 by 480, smaller than the dialogs' space: the same shares of the screen, near the top and without the reward
	const auto layout = Compute({640, 480}, false, 80.0f);
	EXPECT_FLOAT_EQ(layout.size, 30.0f);
	EXPECT_FLOAT_EQ(layout.box.min.x, 16.0f);
	EXPECT_FLOAT_EQ(layout.box.max.x, 176.0f);
	EXPECT_FLOAT_EQ(layout.box.min.y, 12.0f);
	EXPECT_FLOAT_EQ(layout.box.max.y, 132.0f);
	ASSERT_EQ(layout.Rows().size(), 3u);
	EXPECT_FLOAT_EQ(layout.Rows()[0].bar.min.y, 32.0f);
	EXPECT_FLOAT_EQ(layout.Rows()[2].bar.max.y, 107.0f);
	EXPECT_FLOAT_EQ(layout.Rows()[2].bar.max.x, 176.0f);
}

TEST(CreatureStatusPanel, ShowsOnHover)
{
	EXPECT_EQ(k_Trigger, Trigger::Hover);
	EXPECT_TRUE(Triggered(false));
	EXPECT_TRUE(Triggered(true));
}
