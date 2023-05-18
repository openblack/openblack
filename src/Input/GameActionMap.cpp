/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "GameActionMap.h"

#include <SDL_events.h>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>

#include "Game.h"
#include "GameWindow.h"

using namespace openblack::input;

GameActionMap::GameActionMap()
{
	// TODO(#604): Support remapping
	_keyboardBindings.emplace(SDL_SCANCODE_F1, BindableActionMap::HELP);
	_mouseBindings.emplace(SDL_BUTTON_LEFT, BindableActionMap::MOVE);
	_mouseBindings.emplace(SDL_BUTTON_RIGHT, BindableActionMap::ACTION);
	_mouseWheelBinding[0] = BindableActionMap::ZOOM_IN;
	_mouseWheelBinding[1] = BindableActionMap::ZOOM_OUT;
	_mouseBindings.emplace(SDL_BUTTON_RIGHT, BindableActionMap::ACTION);
	_keyboardBindings.emplace(SDL_SCANCODE_T, BindableActionMap::TALK);
	_mouseModBindings.emplace(SDL_BUTTON_LEFT, std::make_pair(KMOD_LCTRL, BindableActionMap::ZOOM_ON));
	_mouseModBindings.emplace(SDL_BUTTON_LEFT, std::make_pair(KMOD_RCTRL, BindableActionMap::ZOOM_ON));
	_mouseModBindings.emplace(SDL_BUTTON_RIGHT, std::make_pair(KMOD_LCTRL, BindableActionMap::ZOOM_ON));
	_mouseModBindings.emplace(SDL_BUTTON_RIGHT, std::make_pair(KMOD_RCTRL, BindableActionMap::ZOOM_ON));
	_keyboardModBindings.emplace(SDL_SCANCODE_LEFT, std::make_pair(KMOD_LCTRL, BindableActionMap::ZOOM_ON));
	_keyboardModBindings.emplace(SDL_SCANCODE_LEFT, std::make_pair(KMOD_RCTRL, BindableActionMap::ZOOM_ON));
	_keyboardModBindings.emplace(SDL_SCANCODE_RIGHT, std::make_pair(KMOD_LCTRL, BindableActionMap::ZOOM_ON));
	_keyboardModBindings.emplace(SDL_SCANCODE_RIGHT, std::make_pair(KMOD_RCTRL, BindableActionMap::ZOOM_ON));
	_keyboardModBindings.emplace(SDL_SCANCODE_UP, std::make_pair(KMOD_LCTRL, BindableActionMap::ZOOM_ON));
	_keyboardModBindings.emplace(SDL_SCANCODE_UP, std::make_pair(KMOD_RCTRL, BindableActionMap::ZOOM_ON));
	_keyboardModBindings.emplace(SDL_SCANCODE_DOWN, std::make_pair(KMOD_LCTRL, BindableActionMap::ZOOM_ON));
	_keyboardModBindings.emplace(SDL_SCANCODE_DOWN, std::make_pair(KMOD_RCTRL, BindableActionMap::ZOOM_ON));
	_keyboardBindings.emplace(SDL_SCANCODE_LEFT, BindableActionMap::MOVE_LEFT);
	_keyboardBindings.emplace(SDL_SCANCODE_RIGHT, BindableActionMap::MOVE_RIGHT);
	_keyboardBindings.emplace(SDL_SCANCODE_UP, BindableActionMap::MOVE_FORWARDS);
	_keyboardBindings.emplace(SDL_SCANCODE_DOWN, BindableActionMap::MOVE_BACKWARDS);
	_keyboardBindings.emplace(SDL_SCANCODE_A, BindableActionMap::TILT_UP);
	_keyboardBindings.emplace(SDL_SCANCODE_Q, BindableActionMap::TILT_DOWN);
	_keyboardBindings.emplace(SDL_SCANCODE_Z, BindableActionMap::ROTATE_LEFT);
	_keyboardBindings.emplace(SDL_SCANCODE_U, BindableActionMap::ROTATE_RIGHT);
	_keyboardBindings.emplace(SDL_SCANCODE_LSHIFT, BindableActionMap::ROTATE_ON);
	_keyboardBindings.emplace(SDL_SCANCODE_RSHIFT, BindableActionMap::ROTATE_ON);
	_mouseBindings.emplace(SDL_BUTTON_MIDDLE, BindableActionMap::ROTATE_AROUND_MOUSE_ON);
	_keyboardBindings.emplace(SDL_SCANCODE_SPACE, BindableActionMap::ZOOM_TO_TEMPLE);
	_keyboardBindings.emplace(SDL_SCANCODE_C, BindableActionMap::ZOOM_TO_CREATURE);
	_keyboardBindings.emplace(SDL_SCANCODE_F3, BindableActionMap::ZOOM_TO_REALM);
	_keyboardBindings.emplace(SDL_SCANCODE_F4, BindableActionMap::ZOOM_TO_INSIDE_TEMPLE);
	_keyboardBindings.emplace(SDL_SCANCODE_F5, BindableActionMap::ZOOM_TO_CREATURE_ROOM);
	_keyboardBindings.emplace(SDL_SCANCODE_F6, BindableActionMap::ZOOM_TO_CHALLENGE_ROOM);
	_keyboardBindings.emplace(SDL_SCANCODE_F7, BindableActionMap::ZOOM_TO_SAVE_GAME_ROOM);
	_keyboardBindings.emplace(SDL_SCANCODE_F8, BindableActionMap::ZOOM_TO_OPTIONS_ROOM);
	_keyboardBindings.emplace(SDL_SCANCODE_F9, BindableActionMap::ZOOM_TO_LIBRARY);
	_keyboardBindings.emplace(SDL_SCANCODE_L, BindableActionMap::LEASH_UNLEASH_CREATURE);
	_keyboardBindings.emplace(SDL_SCANCODE_N, BindableActionMap::SHOW_VILLAGER_NAMES);
	_keyboardBindings.emplace(SDL_SCANCODE_S, BindableActionMap::SHOW_VILLAGER_DETAILS);
	_keyboardModBindings.emplace(SDL_SCANCODE_S, std::make_pair(KMOD_CTRL, BindableActionMap::QUICK_SAVE));
	_keyboardModBindings.emplace(SDL_SCANCODE_L, std::make_pair(KMOD_CTRL, BindableActionMap::QUICK_LOAD));
	_keyboardBindings.emplace(SDL_SCANCODE_V, BindableActionMap::PREVIOUS_LEASH);
	_keyboardBindings.emplace(SDL_SCANCODE_B, BindableActionMap::NEXT_LEASH);
}

bool GameActionMap::GetBindable(BindableActionMap action) const
{
	return (static_cast<uint64_t>(_bindableMap) & static_cast<uint64_t>(action)) != 0;
}

bool GameActionMap::GetUnbindable(UnbindableActionMap action) const
{
	return (static_cast<uint8_t>(_unbindableMap) & static_cast<uint8_t>(action)) != 0;
}

bool GameActionMap::GetBindableChanged(BindableActionMap action) const
{
	return ((static_cast<uint64_t>(_bindableMap) ^ static_cast<uint64_t>(_bindableMapPrevious)) &
	        static_cast<uint64_t>(action)) != 0;
}

bool GameActionMap::GetUnbindableChanged(UnbindableActionMap action) const
{
	return ((static_cast<uint8_t>(_unbindableMap) ^ static_cast<uint8_t>(_unbindableMapPrevious)) &
	        static_cast<uint8_t>(action)) != 0;
}

bool GameActionMap::GetBindableRepeat(BindableActionMap action) const
{
	return ((static_cast<uint64_t>(_bindableMap) & static_cast<uint64_t>(_bindableMapPrevious)) &
	        static_cast<uint64_t>(action)) != 0;
}

bool GameActionMap::GetUnbindableRepeat(UnbindableActionMap action) const
{
	return ((static_cast<uint8_t>(_unbindableMap) & static_cast<uint8_t>(_unbindableMapPrevious)) &
	        static_cast<uint8_t>(action)) != 0;
}

glm::uvec2 GameActionMap::GetMousePosition() const
{
	return _mousePosition;
}

glm::ivec2 GameActionMap::GetMouseDelta() const
{
	return _mouseDelta;
}

void GameActionMap::Frame()
{
	if (_bindableMap != BindableActionMap::NONE || _unbindableMap != UnbindableActionMap::NONE)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("input"), "GameActionMap:");
	}
#define get_print(x)                                               \
	do                                                             \
	{                                                              \
		if (Get(BindableActionMap::x))                             \
		{                                                          \
			SPDLOG_LOGGER_DEBUG(spdlog::get("input"), "\t{}", #x); \
		}                                                          \
	} while (0)

	get_print(HELP);
	get_print(MOVE);
	get_print(ACTION);
	get_print(ZOOM_OUT);
	get_print(ZOOM_IN);
	get_print(TALK);
	get_print(ZOOM_ON);
	get_print(MOVE_LEFT);
	get_print(MOVE_RIGHT);
	get_print(MOVE_FORWARDS);
	get_print(MOVE_BACKWARDS);
	get_print(TILT_UP);
	get_print(TILT_DOWN);
	get_print(ROTATE_LEFT);
	get_print(ROTATE_RIGHT);
	get_print(ROTATE_ON);
	get_print(ROTATE_AROUND_MOUSE_ON);
	get_print(ZOOM_TO_TEMPLE);
	get_print(ZOOM_TO_CREATURE);
	get_print(ZOOM_TO_REALM);
	get_print(ZOOM_TO_INSIDE_TEMPLE);
	get_print(ZOOM_TO_CREATURE_ROOM);
	get_print(ZOOM_TO_CHALLENGE_ROOM);
	get_print(ZOOM_TO_SAVE_GAME_ROOM);
	get_print(ZOOM_TO_OPTIONS_ROOM);
	get_print(ZOOM_TO_LIBRARY);
	get_print(LEASH_UNLEASH_CREATURE);
	get_print(SHOW_VILLAGER_NAMES);
	get_print(SHOW_VILLAGER_DETAILS);
	get_print(QUICK_SAVE);
	get_print(QUICK_LOAD);
	get_print(PREVIOUS_LEASH);
	get_print(NEXT_LEASH);

#undef get_print

#define get_print(x)                                               \
	do                                                             \
	{                                                              \
		if (Get(UnbindableActionMap::x))                           \
		{                                                          \
			SPDLOG_LOGGER_DEBUG(spdlog::get("input"), "\t{}", #x); \
		}                                                          \
	} while (0)

	get_print(DOUBLE_CLICK);

#undef get_print

	{
		glm::ivec2 absoluteMousePosition;
		glm::ivec2 screenSize;
		SDL_GetMouseState(&absoluteMousePosition.x, &absoluteMousePosition.y);
		Game::Instance()->GetWindow()->GetSize(screenSize.x, screenSize.y);
		_mousePosition = glm::clamp(absoluteMousePosition, glm::zero<decltype(screenSize)>(), screenSize);
	}
	_mouseDelta = glm::ivec2(0, 0);
	_bindableMapPrevious = _bindableMap;
	_bindableMap =
	    static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
	                                   ~(static_cast<uint64_t>(_mouseWheelBinding[0].value_or(BindableActionMap::NONE)) |
	                                     static_cast<uint64_t>(_mouseWheelBinding[1].value_or(BindableActionMap::NONE))));
}

void GameActionMap::ProcessEvent(const SDL_Event& event)
{
	if (event.type == SDL_KEYDOWN)
	{
		if (_keyboardModBindings.contains(event.key.keysym.scancode) &&
		    (_keyboardModBindings[event.key.keysym.scancode].first & event.key.keysym.mod) != 0)
		{
			_bindableMap = static_cast<BindableActionMap>(
			    // Remove non-modded action
			    (static_cast<uint64_t>(_bindableMap) & ~static_cast<uint64_t>(_keyboardBindings[event.key.keysym.scancode])) |
			    // Add modded action
			    static_cast<uint64_t>(_keyboardModBindings[event.key.keysym.scancode].second));
		}
		else if (_keyboardBindings.contains(event.key.keysym.scancode))
		{
			_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) |
			                                              static_cast<uint64_t>(_keyboardBindings[event.key.keysym.scancode]));
		}
	}
	// Double click will not count as a single click
	else if (event.type == SDL_MOUSEBUTTONDOWN && (event.button.button & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0 &&
	         event.button.clicks == 2)
	{
		_unbindableMap = static_cast<UnbindableActionMap>(static_cast<uint8_t>(_unbindableMap) |
		                                                  static_cast<uint8_t>(UnbindableActionMap::DOUBLE_CLICK));
	}
	else if (event.type == SDL_MOUSEBUTTONDOWN)
	{
		auto mod = SDL_GetModState();
		if (_mouseModBindings.contains(event.button.button) && (_mouseModBindings[event.button.button].first & mod) != 0)
		{
			_bindableMap = static_cast<BindableActionMap>(
			    // Remove non-modded action
			    (static_cast<uint64_t>(_bindableMap) & ~static_cast<uint64_t>(_mouseBindings[event.button.button])) |
			    // Add modded action
			    static_cast<uint64_t>(_mouseModBindings[event.button.button].second));
		}
		else if (_mouseBindings.contains(event.button.button))
		{
			_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) |
			                                              static_cast<uint64_t>(_mouseBindings[event.button.button]));
		}
	}
	else if (event.type == SDL_MOUSEWHEEL)
	{
		if (event.wheel.y > 0)
		{
			_bindableMap =
			    static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) |
			                                   static_cast<uint64_t>(_mouseWheelBinding[0].value_or(BindableActionMap::NONE)));
		}
		else if (event.wheel.y < 0)
		{
			_bindableMap =
			    static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) |
			                                   static_cast<uint64_t>(_mouseWheelBinding[1].value_or(BindableActionMap::NONE)));
		}
	}
	else if (event.type == SDL_KEYUP)
	{
		if (_keyboardBindings.contains(event.key.keysym.scancode))
		{
			_bindableMap =
			    static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
			                                   ~static_cast<uint64_t>(_keyboardModBindings[event.key.keysym.scancode].second));
			_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
			                                              ~static_cast<uint64_t>(_keyboardBindings[event.key.keysym.scancode]));
		}
	}
	else if (event.type == SDL_MOUSEBUTTONUP && (event.button.button & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0 &&
	         event.button.clicks == 2)
	{
		_unbindableMap = static_cast<UnbindableActionMap>(static_cast<uint8_t>(_unbindableMap) &
		                                                  ~static_cast<uint8_t>(UnbindableActionMap::DOUBLE_CLICK));
	}
	else if (event.type == SDL_MOUSEBUTTONUP)
	{
		if (_mouseBindings.contains(event.button.button))
		{
			_bindableMap = static_cast<BindableActionMap>(
			    static_cast<uint64_t>(_bindableMap) & ~static_cast<uint64_t>(_mouseModBindings[event.button.button].second));
			_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
			                                              ~static_cast<uint64_t>(_mouseBindings[event.button.button]));
		}
	}
	else if (event.type == SDL_MOUSEMOTION)
	{
		_mouseDelta = {event.motion.xrel, event.motion.yrel};
	}
}
