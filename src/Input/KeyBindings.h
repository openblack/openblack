/******************************************************************************
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
#include <string_view>
#include <utility>
#include <vector>

#include <SDL_scancode.h>

#include "BindableActions.h"

namespace openblack::input
{

/// A modifier key that must be held with a binding's key. Either side's key counts, as in the game.
enum class Modifier : uint8_t
{
	None,
	Ctrl,
	Shift,
	Alt,
};

/// The mouse inputs the game's options screen can bind
enum class MouseInput : uint8_t
{
	None,
	LeftButton,
	MiddleButton,
	WheelUp,
	WheelDown,
	RightButton,
};

/// A key, with a modifier that must be held with it
struct KeyChord
{
	SDL_Scancode key {SDL_SCANCODE_UNKNOWN};
	Modifier modifier {Modifier::None};

	[[nodiscard]] constexpr bool operator==(const KeyChord&) const = default;
};

/// How the options screen's actions are grouped for display
enum class BindCategory : uint8_t
{
	Hand,
	Camera,
	Places,
	Creature,
	Villagers,
	Interface,
	Game,
};

/// Whether an action does something yet
enum class BindStatus : uint8_t
{
	Implemented,
	/// The feature behind the action isn't built yet: pressing it only logs that
	NotYetImplemented,
	/// Handled by the leash work (player-leash branch)
	OwnedByLeashWork,
	/// Handled by the creature mode work (creature-mode branch)
	OwnedByCreatureModeWork,
};

/// One row of the options screen's controls list
struct KeyBinding
{
	BindableActionMap action;
	std::string_view name;
	BindCategory category;
	std::optional<KeyChord> key;
	MouseInput mouse {MouseInput::None};
	BindStatus status {BindStatus::Implemented};
};

inline constexpr size_t k_KeyBindingCount = 33;
using KeyBindingTable = std::array<KeyBinding, k_KeyBindingCount>;

// clang-format off
/// The game's defaults, in the order its options screen lists them. Where the game names the right Ctrl or Shift, it
/// treats either side's key as the same one.
inline constexpr KeyBindingTable k_DefaultKeyBindings {{
	{BindableActionMap::HELP,                   "Help",                        BindCategory::Interface, KeyChord {SDL_SCANCODE_F1},     MouseInput::None,         BindStatus::NotYetImplemented},
	{BindableActionMap::MOVE,                   "Move",                        BindCategory::Hand,      std::nullopt,                   MouseInput::LeftButton,   BindStatus::Implemented},
	{BindableActionMap::ACTION,                 "Action",                      BindCategory::Hand,      std::nullopt,                   MouseInput::RightButton,  BindStatus::Implemented},
	{BindableActionMap::ZOOM_OUT,               "Zoom Out",                    BindCategory::Camera,    std::nullopt,                   MouseInput::WheelDown,    BindStatus::Implemented},
	{BindableActionMap::ZOOM_IN,                "Zoom In",                     BindCategory::Camera,    std::nullopt,                   MouseInput::WheelUp,      BindStatus::Implemented},
	{BindableActionMap::TALK,                   "Talk",                        BindCategory::Interface, KeyChord {SDL_SCANCODE_T},      MouseInput::None,         BindStatus::NotYetImplemented},
	{BindableActionMap::ZOOM_ON,                "Zoom On",                     BindCategory::Camera,    KeyChord {SDL_SCANCODE_RCTRL},  MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::MOVE_LEFT,              "Move Left",                   BindCategory::Camera,    KeyChord {SDL_SCANCODE_LEFT},   MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::MOVE_RIGHT,             "Move Right",                  BindCategory::Camera,    KeyChord {SDL_SCANCODE_RIGHT},  MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::MOVE_FORWARDS,          "Move Forwards",               BindCategory::Camera,    KeyChord {SDL_SCANCODE_UP},     MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::MOVE_BACKWARDS,         "Move Backwards",              BindCategory::Camera,    KeyChord {SDL_SCANCODE_DOWN},   MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::TILT_UP,                "Tilt Up",                     BindCategory::Camera,    KeyChord {SDL_SCANCODE_A},      MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::TILT_DOWN,              "Tilt Down",                   BindCategory::Camera,    KeyChord {SDL_SCANCODE_Q},      MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ROTATE_LEFT,            "Rotate Left",                 BindCategory::Camera,    KeyChord {SDL_SCANCODE_Z},      MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ROTATE_RIGHT,           "Rotate Right",                BindCategory::Camera,    KeyChord {SDL_SCANCODE_U},      MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ROTATE_ON,              "Tilt/Rotate On",              BindCategory::Camera,    KeyChord {SDL_SCANCODE_RSHIFT}, MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ROTATE_AROUND_MOUSE_ON, "Tilt/Rotate Around Mouse On", BindCategory::Camera,    std::nullopt,                   MouseInput::MiddleButton, BindStatus::Implemented},
	{BindableActionMap::ZOOM_TO_TEMPLE,         "Zoom To Temple",              BindCategory::Places,    KeyChord {SDL_SCANCODE_SPACE},  MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ZOOM_TO_CREATURE,       "Zoom To Creature",            BindCategory::Creature,  KeyChord {SDL_SCANCODE_C},      MouseInput::None,         BindStatus::OwnedByCreatureModeWork},
	{BindableActionMap::ZOOM_TO_REALM,          "Zoom To Realm",               BindCategory::Places,    KeyChord {SDL_SCANCODE_F3},     MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ZOOM_TO_INSIDE_TEMPLE,  "Zoom To Inside Temple",       BindCategory::Places,    KeyChord {SDL_SCANCODE_F4},     MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ZOOM_TO_CREATURE_ROOM,  "Zoom To Creature Room",       BindCategory::Places,    KeyChord {SDL_SCANCODE_F5},     MouseInput::None,         BindStatus::OwnedByCreatureModeWork},
	{BindableActionMap::ZOOM_TO_CHALLENGE_ROOM, "Zoom To Challenge Room",      BindCategory::Places,    KeyChord {SDL_SCANCODE_F6},     MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ZOOM_TO_SAVE_GAME_ROOM, "Zoom To Save Game Room",      BindCategory::Places,    KeyChord {SDL_SCANCODE_F7},     MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ZOOM_TO_OPTIONS_ROOM,   "Zoom To Options Room",        BindCategory::Places,    KeyChord {SDL_SCANCODE_F8},     MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::ZOOM_TO_LIBRARY,        "Zoom To Library",             BindCategory::Places,    KeyChord {SDL_SCANCODE_F9},     MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::LEASH_UNLEASH_CREATURE, "Leash/UnLeash Creature",      BindCategory::Creature,  KeyChord {SDL_SCANCODE_L},      MouseInput::None,         BindStatus::OwnedByLeashWork},
	{BindableActionMap::SHOW_VILLAGER_NAMES,    "Show Villager Names",         BindCategory::Villagers, KeyChord {SDL_SCANCODE_N},      MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::SHOW_VILLAGER_DETAILS,  "Show Villager Details",       BindCategory::Villagers, KeyChord {SDL_SCANCODE_S},      MouseInput::None,         BindStatus::Implemented},
	{BindableActionMap::QUICK_SAVE,             "Quick Save",                  BindCategory::Game,      KeyChord {SDL_SCANCODE_S, Modifier::Ctrl}, MouseInput::None, BindStatus::NotYetImplemented},
	{BindableActionMap::QUICK_LOAD,             "Quick Load",                  BindCategory::Game,      KeyChord {SDL_SCANCODE_L, Modifier::Ctrl}, MouseInput::None, BindStatus::NotYetImplemented},
	{BindableActionMap::PREVIOUS_LEASH,         "Previous Leash",              BindCategory::Creature,  KeyChord {SDL_SCANCODE_V},      MouseInput::None,         BindStatus::OwnedByLeashWork},
	{BindableActionMap::NEXT_LEASH,             "Next Leash",                  BindCategory::Creature,  KeyChord {SDL_SCANCODE_B},      MouseInput::None,         BindStatus::OwnedByLeashWork},
}};
// clang-format on

/// The key that stands for both sides' modifier keys: the left one for either Ctrl, Shift or Alt
[[nodiscard]] SDL_Scancode CanonicalKey(SDL_Scancode key) noexcept;
/// Whether SDL's modifier state holds the modifier, on either side
[[nodiscard]] bool ModifierHeld(Modifier modifier, uint16_t sdlModifiers) noexcept;
/// The modifier a modifier key itself is, or none for any other key
[[nodiscard]] Modifier ModifierOfKey(SDL_Scancode key) noexcept;

/// The actions a key press starts. A binding that needs a held modifier wins over one on the same key without it, so
/// Ctrl+S quick saves rather than also showing villagers' details.
[[nodiscard]] BindableActionMap ActionsForKeyDown(std::span<const KeyBinding> bindings, SDL_Scancode key,
                                                  uint16_t sdlModifiers) noexcept;
/// Every action bound to the key, with or without a modifier, which letting go of it ends
[[nodiscard]] BindableActionMap ActionsForKey(std::span<const KeyBinding> bindings, SDL_Scancode key) noexcept;
/// The actions bound to a mouse input
[[nodiscard]] BindableActionMap ActionsForMouse(std::span<const KeyBinding> bindings, MouseInput mouse) noexcept;

/// Where an action is in the table
[[nodiscard]] std::optional<size_t> IndexOf(std::span<const KeyBinding> bindings, BindableActionMap action) noexcept;

/// Pairs of rows bound to the same key chord or the same mouse input
[[nodiscard]] std::vector<std::pair<size_t, size_t>> FindConflicts(std::span<const KeyBinding> bindings);

/// How the options screen would show a key chord, such as "Ctrl+S"
[[nodiscard]] std::string KeyChordName(const KeyChord& chord);
[[nodiscard]] std::string_view MouseInputName(MouseInput mouse) noexcept;
[[nodiscard]] std::string_view CategoryName(BindCategory category) noexcept;
[[nodiscard]] std::string_view StatusName(BindStatus status) noexcept;

} // namespace openblack::input
