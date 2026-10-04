/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "3D/TempleToggles.h"
#include "3D/TempleToolTips.h"
#include "Audio/Sound.h"

using namespace openblack;
using Display = TempleToggles::Display;

namespace
{
/// The main room's submeshes, as its mesh names them, among others
const std::vector<std::string> k_Names = {
    "LH_Wall",
    "LH_Checkbox_DisplayCitadel_checked",
    "LH_Checkbox_DisplayCitadel_unchecked",
    "LH_Checkbox_DisplayCreature_checked",
    "LH_Checkbox_DisplayCreature_unchecked",
    "LH_Checkbox_Displaymagicactivity_checked",
    "LH_Checkbox_Displaymagicactivity_unchecked",
    "LH_Checkbox_DisplayInfluence_checked",
    "LH_Checkbox_DisplayInfluence_unchecked",
    "LH_Checkbox_DisplayChallenges_checked",
    "LH_Checkbox_DisplayChallenges_unchecked",
    "LH_SCROLL_WORLD",
};
constexpr uint32_t k_CitadelChecked = 1;
constexpr uint32_t k_CitadelUnchecked = 2;
constexpr uint32_t k_Scroll = 11;

struct Toggles
{
	std::vector<entt::id_type> sounds;
	TempleToggles toggles {[this](entt::id_type sound) { sounds.push_back(sound); }};
	Toggles() { toggles.Find(k_Names); }
};
} // namespace

TEST(TempleToggles, ShowEverythingAtFirst)
{
	Toggles t;
	for (const auto display :
	     {Display::Temples, Display::Creatures, Display::MiracleActivity, Display::Influence, Display::Challenges})
	{
		EXPECT_TRUE(t.toggles.IsShown(display));
	}
	// The checked button of each pair is drawn
	EXPECT_EQ(t.toggles.GetHidden(), (std::vector<uint32_t> {2, 4, 6, 8, 10}));
	ASSERT_EQ(t.toggles.GetControls().size(), 5);
	EXPECT_EQ(t.toggles.GetControls()[0].subMesh, k_CitadelChecked);
	EXPECT_EQ(t.toggles.GetControls()[0].toolTip, 0xE91 - 0xE73);
	EXPECT_TRUE(t.toggles.IsControl(k_CitadelChecked));
	EXPECT_FALSE(t.toggles.IsControl(k_CitadelUnchecked));
}

TEST(TempleToggles, TurnOverAsTheyAreLetGoOf)
{
	Toggles t;
	// The press takes the mouse, and nothing happens until it is let go
	EXPECT_TRUE(t.toggles.Hold(true, k_CitadelChecked));
	EXPECT_TRUE(t.toggles.Hold(true, k_CitadelChecked));
	EXPECT_TRUE(t.toggles.IsShown(Display::Temples));
	EXPECT_TRUE(t.toggles.Hold(false, k_CitadelChecked));
	EXPECT_FALSE(t.toggles.IsShown(Display::Temples));
	EXPECT_FALSE(t.toggles.IsHeld());
	ASSERT_EQ(t.sounds.size(), 1);
	EXPECT_EQ(t.sounds[0], static_cast<entt::id_type>(audio::SoundId::G_CitadelButtonDown_01));
	EXPECT_TRUE(t.toggles.IsControl(k_CitadelUnchecked));

	// And back on with the other button of the pair
	t.toggles.Hold(true, k_CitadelUnchecked);
	t.toggles.Hold(false, k_CitadelUnchecked);
	EXPECT_TRUE(t.toggles.IsShown(Display::Temples));
	EXPECT_EQ(t.sounds.back(), static_cast<entt::id_type>(audio::SoundId::G_CitadelButtonUp_01));
}

TEST(TempleToggles, StayAsTheyAreLetGoOfElsewhere)
{
	Toggles t;
	t.toggles.Hold(true, k_CitadelChecked);
	EXPECT_TRUE(t.toggles.Hold(false, k_Scroll));
	EXPECT_TRUE(t.toggles.IsShown(Display::Temples));
	EXPECT_TRUE(t.sounds.empty());
	// A press elsewhere, or on the button not drawn, isn't theirs
	EXPECT_FALSE(t.toggles.Hold(true, k_Scroll));
	t.toggles.Hold(false, std::nullopt);
	EXPECT_FALSE(t.toggles.Hold(true, k_CitadelUnchecked));
}

TEST(TempleToggles, SayWhatTheyShowWhenHovered)
{
	Toggles t;
	const auto toggles = t.toggles.GetControls();
	const std::array scrolls = {TempleScrolls::Control {.subMesh = k_Scroll, .focused = false}};
	TempleToolTip toolTip = k_FirstTempleToolTip;
	TempleToolTipInput input {.room = TempleRoom::Main,
	                          .inControl = true,
	                          .hoveredSubMesh = k_CitadelChecked,
	                          .scrolls = scrolls,
	                          .toggles = toggles};
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, 0xE91 - 0xE73);
	EXPECT_EQ(toolTip.arrows, gui::ToolTipArrows::k_None);

	// The scroll's callback comes after the buttons', so looking at it shows the way back over them
	input.zoom = 1.0f;
	input.lookingAtScroll = true;
	UpdateTempleToolTip(toolTip, input);
	EXPECT_EQ(toolTip.index, 0xE8B - 0xE73);
}
