/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include <cmath>

#include <array>
#include <chrono>
#include <numbers>
#include <string_view>

#include <SDL_events.h>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Camera/ZoomToPlaces.h"
#include "Input/GameActionMap.h"
#include "Input/KeyBindings.h"

using namespace openblack;
using namespace openblack::input;

namespace
{
struct ExpectedBinding
{
	BindableActionMap action;
	std::string_view name;
	SDL_Scancode key;
	Modifier modifier;
	MouseInput mouse;
};

// The options screen's controls list as the game sets it up, in its order
// clang-format off
constexpr std::array<ExpectedBinding, k_KeyBindingCount> k_Documented {{
	{BindableActionMap::HELP,                   "Help",                        SDL_SCANCODE_F1,      Modifier::None, MouseInput::None},
	{BindableActionMap::MOVE,                   "Move",                        SDL_SCANCODE_UNKNOWN, Modifier::None, MouseInput::LeftButton},
	{BindableActionMap::ACTION,                 "Action",                      SDL_SCANCODE_UNKNOWN, Modifier::None, MouseInput::RightButton},
	{BindableActionMap::ZOOM_OUT,               "Zoom Out",                    SDL_SCANCODE_UNKNOWN, Modifier::None, MouseInput::WheelDown},
	{BindableActionMap::ZOOM_IN,                "Zoom In",                     SDL_SCANCODE_UNKNOWN, Modifier::None, MouseInput::WheelUp},
	{BindableActionMap::TALK,                   "Talk",                        SDL_SCANCODE_T,       Modifier::None, MouseInput::None},
	{BindableActionMap::ZOOM_ON,                "Zoom On",                     SDL_SCANCODE_RCTRL,   Modifier::None, MouseInput::None},
	{BindableActionMap::MOVE_LEFT,              "Move Left",                   SDL_SCANCODE_LEFT,    Modifier::None, MouseInput::None},
	{BindableActionMap::MOVE_RIGHT,             "Move Right",                  SDL_SCANCODE_RIGHT,   Modifier::None, MouseInput::None},
	{BindableActionMap::MOVE_FORWARDS,          "Move Forwards",               SDL_SCANCODE_UP,      Modifier::None, MouseInput::None},
	{BindableActionMap::MOVE_BACKWARDS,         "Move Backwards",              SDL_SCANCODE_DOWN,    Modifier::None, MouseInput::None},
	{BindableActionMap::TILT_UP,                "Tilt Up",                     SDL_SCANCODE_A,       Modifier::None, MouseInput::None},
	{BindableActionMap::TILT_DOWN,              "Tilt Down",                   SDL_SCANCODE_Q,       Modifier::None, MouseInput::None},
	{BindableActionMap::ROTATE_LEFT,            "Rotate Left",                 SDL_SCANCODE_Z,       Modifier::None, MouseInput::None},
	{BindableActionMap::ROTATE_RIGHT,           "Rotate Right",                SDL_SCANCODE_U,       Modifier::None, MouseInput::None},
	{BindableActionMap::ROTATE_ON,              "Tilt/Rotate On",              SDL_SCANCODE_RSHIFT,  Modifier::None, MouseInput::None},
	{BindableActionMap::ROTATE_AROUND_MOUSE_ON, "Tilt/Rotate Around Mouse On", SDL_SCANCODE_UNKNOWN, Modifier::None, MouseInput::MiddleButton},
	{BindableActionMap::ZOOM_TO_TEMPLE,         "Zoom To Temple",              SDL_SCANCODE_SPACE,   Modifier::None, MouseInput::None},
	{BindableActionMap::ZOOM_TO_CREATURE,       "Zoom To Creature",            SDL_SCANCODE_C,       Modifier::None, MouseInput::None},
	{BindableActionMap::ZOOM_TO_REALM,          "Zoom To Realm",               SDL_SCANCODE_F3,      Modifier::None, MouseInput::None},
	{BindableActionMap::ZOOM_TO_INSIDE_TEMPLE,  "Zoom To Inside Temple",       SDL_SCANCODE_F4,      Modifier::None, MouseInput::None},
	{BindableActionMap::ZOOM_TO_CREATURE_ROOM,  "Zoom To Creature Room",       SDL_SCANCODE_F5,      Modifier::None, MouseInput::None},
	{BindableActionMap::ZOOM_TO_CHALLENGE_ROOM, "Zoom To Challenge Room",      SDL_SCANCODE_F6,      Modifier::None, MouseInput::None},
	{BindableActionMap::ZOOM_TO_SAVE_GAME_ROOM, "Zoom To Save Game Room",      SDL_SCANCODE_F7,      Modifier::None, MouseInput::None},
	{BindableActionMap::ZOOM_TO_OPTIONS_ROOM,   "Zoom To Options Room",        SDL_SCANCODE_F8,      Modifier::None, MouseInput::None},
	{BindableActionMap::ZOOM_TO_LIBRARY,        "Zoom To Library",             SDL_SCANCODE_F9,      Modifier::None, MouseInput::None},
	{BindableActionMap::LEASH_UNLEASH_CREATURE, "Leash/UnLeash Creature",      SDL_SCANCODE_L,       Modifier::None, MouseInput::None},
	{BindableActionMap::SHOW_VILLAGER_NAMES,    "Show Villager Names",         SDL_SCANCODE_N,       Modifier::None, MouseInput::None},
	{BindableActionMap::SHOW_VILLAGER_DETAILS,  "Show Villager Details",       SDL_SCANCODE_S,       Modifier::None, MouseInput::None},
	{BindableActionMap::QUICK_SAVE,             "Quick Save",                  SDL_SCANCODE_S,       Modifier::Ctrl, MouseInput::None},
	{BindableActionMap::QUICK_LOAD,             "Quick Load",                  SDL_SCANCODE_L,       Modifier::Ctrl, MouseInput::None},
	{BindableActionMap::PREVIOUS_LEASH,         "Previous Leash",              SDL_SCANCODE_V,       Modifier::None, MouseInput::None},
	{BindableActionMap::NEXT_LEASH,             "Next Leash",                  SDL_SCANCODE_B,       Modifier::None, MouseInput::None},
}};
// clang-format on

[[nodiscard]] SDL_Event KeyEvent(uint32_t type, SDL_Scancode key, uint16_t modifiers = KMOD_NONE)
{
	SDL_Event event {};
	event.type = type;
	event.key.keysym.scancode = key;
	event.key.keysym.mod = modifiers;
	return event;
}

[[nodiscard]] bool Pressed(const GameActionMap& map, BindableActionMap action)
{
	return map.Get(action) && map.GetChanged(action);
}
} // namespace

TEST(KeyBindings, DefaultsMatchTheOptionsScreen)
{
	for (size_t i = 0; i < k_KeyBindingCount; ++i)
	{
		const auto& binding = k_DefaultKeyBindings.at(i);
		const auto& expected = k_Documented.at(i);
		SCOPED_TRACE(expected.name);
		EXPECT_EQ(binding.action, expected.action);
		EXPECT_EQ(binding.name, expected.name);
		EXPECT_EQ(binding.mouse, expected.mouse);
		if (expected.key == SDL_SCANCODE_UNKNOWN)
		{
			EXPECT_FALSE(binding.key.has_value());
		}
		else
		{
			ASSERT_TRUE(binding.key.has_value());
			EXPECT_EQ(binding.key->key, expected.key);
			EXPECT_EQ(binding.key->modifier, expected.modifier);
		}
	}
}

TEST(KeyBindings, EveryActionIsInTheTableOnce)
{
	uint64_t seen = 0;
	for (const auto& binding : k_DefaultKeyBindings)
	{
		const auto bit = static_cast<uint64_t>(binding.action);
		EXPECT_EQ(seen & bit, 0u) << binding.name;
		seen |= bit;
	}
	EXPECT_EQ(seen, static_cast<uint64_t>(BindableActionMap::ALL));
}

TEST(KeyBindings, DefaultsHaveNoConflicts)
{
	EXPECT_TRUE(FindConflicts(k_DefaultKeyBindings).empty());
}

TEST(KeyBindings, ConflictsAreFound)
{
	auto bindings = k_DefaultKeyBindings;
	bindings.at(*IndexOf(bindings, BindableActionMap::TALK)).key = KeyChord {SDL_SCANCODE_N};
	const auto conflicts = FindConflicts(bindings);
	ASSERT_EQ(conflicts.size(), 1u);
	EXPECT_EQ(bindings.at(conflicts[0].first).action, BindableActionMap::TALK);
	EXPECT_EQ(bindings.at(conflicts[0].second).action, BindableActionMap::SHOW_VILLAGER_NAMES);
}

TEST(KeyBindings, KeysMapToActions)
{
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_SPACE, KMOD_NONE), BindableActionMap::ZOOM_TO_TEMPLE);
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_F3, KMOD_NONE), BindableActionMap::ZOOM_TO_REALM);
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_X, KMOD_NONE), BindableActionMap::NONE);
}

TEST(KeyBindings, EitherSidesModifierKeyCounts)
{
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_LCTRL, KMOD_LCTRL), BindableActionMap::ZOOM_ON);
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_RCTRL, KMOD_RCTRL), BindableActionMap::ZOOM_ON);
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_LSHIFT, KMOD_LSHIFT), BindableActionMap::ROTATE_ON);
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_RSHIFT, KMOD_RSHIFT), BindableActionMap::ROTATE_ON);
}

TEST(KeyBindings, ModifierBindingWinsOverPlainOne)
{
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_S, KMOD_NONE), BindableActionMap::SHOW_VILLAGER_DETAILS);
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_S, KMOD_RCTRL), BindableActionMap::QUICK_SAVE);
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_L, KMOD_NONE), BindableActionMap::LEASH_UNLEASH_CREATURE);
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_L, KMOD_LCTRL), BindableActionMap::QUICK_LOAD);
	// Shift isn't the quick save's modifier, so S with Shift is still the details
	EXPECT_EQ(ActionsForKeyDown(k_DefaultKeyBindings, SDL_SCANCODE_S, KMOD_LSHIFT), BindableActionMap::SHOW_VILLAGER_DETAILS);
}

TEST(KeyBindings, LettingGoOfAKeyEndsAllItsActions)
{
	EXPECT_EQ(ActionsForKey(k_DefaultKeyBindings, SDL_SCANCODE_S),
	          static_cast<BindableActionMap>(static_cast<uint64_t>(BindableActionMap::SHOW_VILLAGER_DETAILS) |
	                                         static_cast<uint64_t>(BindableActionMap::QUICK_SAVE)));
}

TEST(KeyBindings, MouseMapsToActions)
{
	EXPECT_EQ(ActionsForMouse(k_DefaultKeyBindings, MouseInput::LeftButton), BindableActionMap::MOVE);
	EXPECT_EQ(ActionsForMouse(k_DefaultKeyBindings, MouseInput::RightButton), BindableActionMap::ACTION);
	EXPECT_EQ(ActionsForMouse(k_DefaultKeyBindings, MouseInput::MiddleButton), BindableActionMap::ROTATE_AROUND_MOUSE_ON);
	EXPECT_EQ(ActionsForMouse(k_DefaultKeyBindings, MouseInput::WheelUp), BindableActionMap::ZOOM_IN);
	EXPECT_EQ(ActionsForMouse(k_DefaultKeyBindings, MouseInput::WheelDown), BindableActionMap::ZOOM_OUT);
	EXPECT_EQ(ActionsForMouse(k_DefaultKeyBindings, MouseInput::None), BindableActionMap::NONE);
}

TEST(KeyBindings, Names)
{
	EXPECT_EQ(KeyChordName({SDL_SCANCODE_S, Modifier::Ctrl}), "Ctrl+S");
	EXPECT_EQ(KeyChordName({SDL_SCANCODE_RCTRL}), "Ctrl");
	EXPECT_EQ(KeyChordName({SDL_SCANCODE_F3}), "F3");
}

TEST(GameActionMap, KeyEventsPressAndReleaseActions)
{
	GameActionMap map;
	map.Frame();
	map.ProcessEvent(KeyEvent(SDL_KEYDOWN, SDL_SCANCODE_N));
	EXPECT_TRUE(Pressed(map, BindableActionMap::SHOW_VILLAGER_NAMES));
	map.Frame();
	EXPECT_TRUE(map.Get(BindableActionMap::SHOW_VILLAGER_NAMES));
	EXPECT_FALSE(map.GetChanged(BindableActionMap::SHOW_VILLAGER_NAMES));
	map.ProcessEvent(KeyEvent(SDL_KEYUP, SDL_SCANCODE_N));
	EXPECT_FALSE(map.Get(BindableActionMap::SHOW_VILLAGER_NAMES));
}

TEST(GameActionMap, ChordReplacesThePlainKey)
{
	GameActionMap map;
	map.Frame();
	map.ProcessEvent(KeyEvent(SDL_KEYDOWN, SDL_SCANCODE_LCTRL, KMOD_LCTRL));
	map.ProcessEvent(KeyEvent(SDL_KEYDOWN, SDL_SCANCODE_S, KMOD_LCTRL));
	EXPECT_TRUE(map.Get(BindableActionMap::ZOOM_ON));
	EXPECT_TRUE(Pressed(map, BindableActionMap::QUICK_SAVE));
	EXPECT_FALSE(map.Get(BindableActionMap::SHOW_VILLAGER_DETAILS));
	map.ProcessEvent(KeyEvent(SDL_KEYUP, SDL_SCANCODE_S, KMOD_LCTRL));
	EXPECT_FALSE(map.Get(BindableActionMap::QUICK_SAVE));
}

TEST(GameActionMap, RebindingMovesTheAction)
{
	GameActionMap map;
	map.SetKeyBinding(BindableActionMap::ZOOM_TO_TEMPLE, KeyChord {SDL_SCANCODE_HOME});
	map.Frame();
	map.ProcessEvent(KeyEvent(SDL_KEYDOWN, SDL_SCANCODE_SPACE));
	EXPECT_FALSE(map.Get(BindableActionMap::ZOOM_TO_TEMPLE));
	map.ProcessEvent(KeyEvent(SDL_KEYDOWN, SDL_SCANCODE_HOME));
	EXPECT_TRUE(Pressed(map, BindableActionMap::ZOOM_TO_TEMPLE));
	map.ResetKeyBindings();
	EXPECT_EQ(map.GetKeyBindings()[*IndexOf(map.GetKeyBindings(), BindableActionMap::ZOOM_TO_TEMPLE)].key,
	          KeyChord {SDL_SCANCODE_SPACE});
}

TEST(GameActionMap, QueuedPressGoesThroughTheKeyPath)
{
	GameActionMap map;
	map.Frame();
	// Every action, pressed from the debug window, shows as pressed for one frame and let go of the next
	for (const auto& binding : k_DefaultKeyBindings)
	{
		SCOPED_TRACE(binding.name);
		map.QueuePress(binding.action);
		EXPECT_TRUE(map.HasQueuedPresses());
		map.Frame();
		EXPECT_TRUE(Pressed(map, binding.action));
		map.Frame();
		EXPECT_FALSE(map.Get(binding.action));
		map.Frame();
		EXPECT_FALSE(map.HasQueuedPresses());
	}
}

TEST(GameActionMap, QueuedChordPressesOnlyTheChordsAction)
{
	GameActionMap map;
	map.Frame();
	map.QueuePress(BindableActionMap::QUICK_SAVE);
	map.Frame();
	EXPECT_TRUE(Pressed(map, BindableActionMap::QUICK_SAVE));
	EXPECT_FALSE(map.Get(BindableActionMap::SHOW_VILLAGER_DETAILS));
}

namespace
{
constexpr float k_Tolerance = 1e-3f;

[[nodiscard]] float PitchOf(const zoom_to::CameraView& view)
{
	const auto towards = view.origin - view.focus;
	return std::asin(towards.y / glm::length(towards));
}
} // namespace

TEST(ZoomToPlaces, SingleTapKeepsHeadingAndFocus)
{
	zoom_to::ZoomToPlaces zoomTo;
	const zoom_to::CameraView current {.origin = {0.0f, 200.0f, -300.0f}, .focus = {0.0f, 0.0f, 0.0f}};
	const auto view = zoomTo.PressTemple(std::chrono::milliseconds(10000), current, {0.0f, 0.0f, 0.0f},
	                                     glm::vec3(1000.0f, 10.0f, 1000.0f), {500.0f, 0.0f, 500.0f});
	ASSERT_TRUE(view.has_value());
	EXPECT_NEAR(view->focus.y, zoom_to::k_FocusAboveGround, k_Tolerance);
	EXPECT_NEAR(glm::distance(view->origin, view->focus), zoom_to::k_TapDistance, k_Tolerance);
	EXPECT_NEAR(PitchOf(*view), zoom_to::k_TapPitch, k_Tolerance);
	EXPECT_NEAR(zoom_to::HeadingOf(*view), zoom_to::HeadingOf(current), k_Tolerance);
	EXPECT_FALSE(zoomTo.IsZoomedTo());
}

TEST(ZoomToPlaces, DoubleTapFliesToTheTempleAndBack)
{
	zoom_to::ZoomToPlaces zoomTo;
	const zoom_to::CameraView start {.origin = {0.0f, 200.0f, -300.0f}, .focus = {0.0f, 0.0f, 0.0f}};
	const glm::vec3 temple {1000.0f, 10.0f, 1000.0f};
	const glm::vec3 realm {500.0f, 0.0f, 500.0f};
	const auto first = zoomTo.PressTemple(std::chrono::milliseconds(10000), start, {}, temple, realm);
	ASSERT_TRUE(first.has_value());
	const auto second = zoomTo.PressTemple(std::chrono::milliseconds(10300), *first, {}, temple, realm);
	ASSERT_TRUE(second.has_value());
	EXPECT_TRUE(zoomTo.IsZoomedTo());
	EXPECT_NEAR(second->focus.x, temple.x, k_Tolerance);
	EXPECT_NEAR(second->focus.y, temple.y + zoom_to::k_FocusAboveGround, k_Tolerance);
	EXPECT_NEAR(glm::distance(second->origin, second->focus), zoom_to::k_TempleDistance, k_Tolerance);
	EXPECT_NEAR(PitchOf(*second), zoom_to::k_TemplePitch, k_Tolerance);

	// A tap long after is a single tap again; a double tap while looking at the temple flies back
	const auto third = zoomTo.PressTemple(std::chrono::milliseconds(20000), *second, temple, temple, realm);
	ASSERT_TRUE(third.has_value());
	const auto back = zoomTo.PressTemple(std::chrono::milliseconds(20200), *third, temple, temple, realm);
	ASSERT_TRUE(back.has_value());
	EXPECT_NEAR(glm::distance(back->origin, first->origin), 0.0f, k_Tolerance);
	EXPECT_FALSE(zoomTo.IsZoomedTo());
}

TEST(ZoomToPlaces, RealmFliesOverTheIslandAndBack)
{
	zoom_to::ZoomToPlaces zoomTo;
	const zoom_to::CameraView start {.origin = {0.0f, 200.0f, -300.0f}, .focus = {0.0f, 0.0f, 0.0f}};
	const glm::vec3 realm {2500.0f, 20.0f, 2600.0f};
	const auto view = zoomTo.PressRealm(start, realm);
	ASSERT_TRUE(view.has_value());
	EXPECT_NEAR(glm::distance(view->origin, view->focus), zoom_to::k_RealmDistance, 0.01f);
	EXPECT_NEAR(PitchOf(*view), zoom_to::k_RealmPitch, k_Tolerance);
	const auto back = zoomTo.PressRealm(*view, realm);
	ASSERT_TRUE(back.has_value());
	EXPECT_NEAR(glm::distance(back->origin, start.origin), 0.0f, k_Tolerance);
}

TEST(ZoomToPlaces, DoubleTapWithoutATempleGoesOverTheRealm)
{
	zoom_to::ZoomToPlaces zoomTo;
	const zoom_to::CameraView start {.origin = {0.0f, 200.0f, -300.0f}, .focus = {0.0f, 0.0f, 0.0f}};
	const glm::vec3 realm {2500.0f, 20.0f, 2600.0f};
	const auto first = zoomTo.PressTemple(std::chrono::milliseconds(1000), start, {}, std::nullopt, realm);
	const auto second = zoomTo.PressTemple(std::chrono::milliseconds(1100), *first, {}, std::nullopt, realm);
	ASSERT_TRUE(second.has_value());
	EXPECT_NEAR(second->focus.x, realm.x, k_Tolerance);
	EXPECT_NEAR(glm::distance(second->origin, second->focus), zoom_to::k_RealmDistance, 0.01f);
}
