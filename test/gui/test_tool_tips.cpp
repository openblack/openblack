/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>

#include <gtest/gtest.h>

#include "3D/TempleToolTips.h"
#include "Gui/ToolTips.h"

using namespace openblack;
using namespace openblack::gui;

namespace
{
constexpr uint32_t k_Move = 0xE7E - 0xE73;
constexpr uint32_t k_ZoomIn = 0xE75 - 0xE73;
constexpr uint32_t k_Scroll = 0xEE2 - 0xE73;
constexpr uint32_t k_ZoomOut = 0xE8B - 0xE73;
constexpr uint32_t k_WorldRoom = 0xE9B - 0xE73;
constexpr uint32_t k_Stat = 20;

/// Most tooltips' info, and one stat that matters more
std::array<ToolTipInfo, ToolTips::k_Count> Info()
{
	std::array<ToolTipInfo, ToolTips::k_Count> info {};
	info.fill({.priority = 0.5f, .displayTime = 1.0f, .displayTimeAfterFocus = 0.0f});
	info.at(k_Stat) = {.priority = 0.9f, .displayTime = 1.0f, .displayTimeAfterFocus = 1.0f};
	return info;
}

/// Turns of a tooltip submitted, a frame of a tenth of a second each
void Show(ToolTips& toolTips, uint32_t index, int turns, ToolTipAction action = ToolTipAction::Select)
{
	for (int i = 0; i < turns; ++i)
	{
		toolTips.Submit(index, action, ToolTipArrows::k_None);
		toolTips.ProcessTurn();
		toolTips.Update(0.1f);
	}
}
} // namespace

TEST(ToolTips, NamesTheirTexts)
{
	EXPECT_EQ(ToolTips::TextName(0), "HELP_TEXT_TOOLTIP_01");
	EXPECT_EQ(ToolTips::TextName(k_Move), "HELP_TEXT_TOOLTIP_12");
	EXPECT_EQ(ToolTips::TextName(0xEED - 0xE73), "HELP_TEXT_TOOLTIP_123");
}

TEST(ToolTips, FadeInOverOneMoreSecondEachTimeShown)
{
	ToolTips toolTips(Info());
	Show(toolTips, k_Move, 5);
	ASSERT_TRUE(toolTips.GetShown().has_value());
	EXPECT_EQ(toolTips.GetShown()->index, k_Move);
	EXPECT_NEAR(toolTips.GetShown()->alpha, 0.5f, 1e-4f);

	// Not submitted, it ends at once
	toolTips.ProcessTurn();
	EXPECT_FALSE(toolTips.GetShown().has_value());

	// The second time it takes two seconds
	Show(toolTips, k_Move, 5);
	EXPECT_NEAR(toolTips.GetShown()->alpha, 0.25f, 1e-4f);
}

TEST(ToolTips, FadeOutOnceReadAtTheIntelligentLevel)
{
	ToolTips toolTips(Info());
	// In after a second, then shown for the rest of its 25 turns, then out over a second
	Show(toolTips, k_Move, 25);
	EXPECT_NEAR(toolTips.GetShown()->alpha, 1.0f, 1e-4f);
	Show(toolTips, k_Move, 5);
	EXPECT_NEAR(toolTips.GetShown()->alpha, 0.5f, 0.11f);
	Show(toolTips, k_Move, 10);
	ASSERT_TRUE(toolTips.GetShown().has_value());
	EXPECT_EQ(toolTips.GetShown()->alpha, 0.0f);
}

TEST(ToolTips, ShowAtOnceAndStayAtTheAllLevel)
{
	ToolTips toolTips(Info());
	toolTips.SetLevel(ToolTipLevel::All);
	Show(toolTips, k_Move, 1);
	EXPECT_EQ(toolTips.GetShown()->alpha, 1.0f);
	Show(toolTips, k_Move, 60);
	EXPECT_EQ(toolTips.GetShown()->alpha, 1.0f);
}

TEST(ToolTips, ShowOnlyTheImportantAtTheMinimumLevel)
{
	ToolTips toolTips(Info());
	toolTips.SetLevel(ToolTipLevel::Minimum);
	Show(toolTips, k_Move, 3);
	EXPECT_FALSE(toolTips.GetShown().has_value());
	Show(toolTips, k_Stat, 3);
	ASSERT_TRUE(toolTips.GetShown().has_value());
	EXPECT_EQ(toolTips.GetShown()->alpha, 1.0f);
}

TEST(ToolTips, KeepOthersOffForTheirDisplayTime)
{
	ToolTips toolTips(Info());
	Show(toolTips, k_Move, 2);
	// Another can't take over while the first is being shown, which then ends, and it shows from the next turn
	Show(toolTips, k_ZoomIn, 1);
	EXPECT_FALSE(toolTips.GetShown().has_value());
	Show(toolTips, k_ZoomIn, 1);
	ASSERT_TRUE(toolTips.GetShown().has_value());
	EXPECT_EQ(toolTips.GetShown()->index, k_ZoomIn);
}

TEST(ToolTips, LingerAfterFocusWhenTheirInfoSaysSo)
{
	ToolTips toolTips(Info());
	Show(toolTips, k_Stat, 2);
	for (int i = 0; i < 25; ++i)
	{
		toolTips.ProcessTurn();
		ASSERT_TRUE(toolTips.GetShown().has_value()) << i;
	}
	toolTips.ProcessTurn();
	EXPECT_FALSE(toolTips.GetShown().has_value());
}

TEST(TempleToolTips, MainRoomMovesZoomsAndLeadsThroughDoors)
{
	TempleToolTip toolTip = k_FirstTempleToolTip;
	TempleToolTipInput input {.room = TempleRoom::Main, .inControl = true};
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, k_Move);
	EXPECT_EQ(toolTip.arrows, ToolTipArrows::k_All);

	input.overPool = true;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, k_ZoomIn);
	EXPECT_EQ(toolTip.arrows, ToolTipArrows::k_None);

	input.overPool = false;
	input.hoveredDoor = 1;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, 0xE99 - 0xE73);
	// The scroll's wall leaves Move
	input.hoveredDoor = 6;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, k_Move);
}

TEST(TempleToolTips, ScrollsZoomInScrollAndZoomOut)
{
	const std::array scrolls = {TempleScrolls::Control {.subMesh = 9, .focused = true}};
	TempleToolTip toolTip = k_FirstTempleToolTip;
	TempleToolTipInput input {.room = TempleRoom::Challenge, .inControl = true, .hoveredSubMesh = 9, .scrolls = scrolls};
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, 0xEE5 - 0xE73);

	input.zoom = 1.0f;
	input.lookingAtScroll = true;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, k_Scroll);
	EXPECT_EQ(toolTip.arrows, ToolTipArrows::k_UpDown);

	input.hoveredSubMesh = std::nullopt;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, k_ZoomOut);
}

TEST(TempleToolTips, CreatureRoomKeepsTheScrollsArrows)
{
	const std::array scrolls = {TempleScrolls::Control {.subMesh = 3, .focused = false},
	                            TempleScrolls::Control {.subMesh = 4, .focused = true}};
	TempleToolTip toolTip = k_FirstTempleToolTip;
	TempleToolTipInput input {.room = TempleRoom::CreatureCave, .inControl = true, .scrolls = scrolls};
	// An unfocused scroll hovered shows its title, which isn't a tooltip
	input.hoveredSubMesh = 3;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_FALSE(toolTip.index.has_value());

	input.hoveredSubMesh = 4;
	input.zoom = 1.0f;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, k_Scroll);

	// Moving off, the arrows stay up and down
	input.hoveredSubMesh = std::nullopt;
	input.zoom = 0.0f;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, k_Move);
	EXPECT_EQ(toolTip.arrows, ToolTipArrows::k_UpDown);

	input.overWayBack = true;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, k_WorldRoom);
}

TEST(TempleToolTips, NoneOnTheWayIn)
{
	TempleToolTip toolTip = k_FirstTempleToolTip;
	UpdateTempleToolTip(toolTip, {.room = TempleRoom::Main, .inControl = false});
	EXPECT_FALSE(toolTip.index.has_value());
	EXPECT_EQ(toolTip.action, ToolTipAction::None);
	// The options room leaves it so
	UpdateTempleToolTip(toolTip, {.room = TempleRoom::Options, .inControl = true});
	EXPECT_FALSE(toolTip.index.has_value());
}
