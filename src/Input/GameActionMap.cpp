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
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>

#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h" // For hand
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack::input;

GameActionMap::GameActionMap()
{
	// TODO(#604): Support remapping
	_keyboardBindings.emplace(SDL_SCANCODE_F1, BindableActionMap::HELP);
	_mouseBindings.emplace(SDL_BUTTON_LMASK, BindableActionMap::MOVE);
	_mouseBindings.emplace(SDL_BUTTON_RMASK, BindableActionMap::ACTION);
	_mouseWheelBinding[0] = BindableActionMap::ZOOM_IN;
	_mouseWheelBinding[1] = BindableActionMap::ZOOM_OUT;
	_keyboardBindings.emplace(SDL_SCANCODE_T, BindableActionMap::TALK);
	_keyboardBindings.emplace(SDL_SCANCODE_LCTRL, BindableActionMap::ZOOM_ON);
	_keyboardBindings.emplace(SDL_SCANCODE_RCTRL, BindableActionMap::ZOOM_ON);
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
	_mouseBindings.emplace(SDL_BUTTON_MMASK, BindableActionMap::ROTATE_AROUND_MOUSE_ON);
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
	if ((SDL_GetMouseState(nullptr, nullptr) & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK)) == (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK))
	{
		_unbindableMap = static_cast<UnbindableActionMap>(static_cast<uint8_t>(_unbindableMap) |
		                                                  static_cast<uint8_t>(UnbindableActionMap::TWO_BUTTON_CLICK));
		_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
		                                              ~(static_cast<uint64_t>(_mouseModBindings[SDL_BUTTON_LMASK].second) |
		                                                static_cast<uint64_t>(_mouseModBindings[SDL_BUTTON_RMASK].second)));
		_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
		                                              ~(static_cast<uint64_t>(_mouseBindings[SDL_BUTTON_LMASK]) |
		                                                static_cast<uint64_t>(_mouseBindings[SDL_BUTTON_LMASK])));
	}
	else
	{
		_unbindableMap = static_cast<UnbindableActionMap>(static_cast<uint8_t>(_unbindableMap) &
		                                                  ~static_cast<uint8_t>(UnbindableActionMap::TWO_BUTTON_CLICK));
	}

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
	get_print(TWO_BUTTON_CLICK);

#undef get_print

	{
		glm::ivec2 absoluteMousePosition;
		SDL_GetMouseState(&absoluteMousePosition.x, &absoluteMousePosition.y);
		const auto screenSize =
		    Locator::windowing::has_value() ? Locator::windowing::value().GetSize() : glm::ivec2(1.0f, 1.0f);
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
		const auto mod = static_cast<SDL_Keymod>(SDL_GetModState());
		const uint8_t buttonMask = SDL_BUTTON(event.button.button);
		const uint8_t newComboMask = _currentMouseButtons | buttonMask;
		if (newComboMask != buttonMask)
		{
			if (_mouseModBindings.contains(newComboMask) && (_mouseModBindings[newComboMask].first & mod) != 0)
			{
				_bindableMap = static_cast<BindableActionMap>(
				    // Remove non-modded action
				    (static_cast<uint64_t>(_bindableMap) & ~static_cast<uint64_t>(_mouseBindings[newComboMask])) |
				    // Add modded action
				    static_cast<uint64_t>(_mouseModBindings[newComboMask].second));
				// Remove non-combo keys
				for (uint8_t i = 0; i < SDL_BUTTON_X2; ++i)
				{
					const uint8_t keyMask = (1 << i);
					if ((newComboMask & keyMask) != 0u && _mouseModBindings.contains(keyMask) &&
					    (_mouseModBindings[keyMask].first & mod) != 0)
					{
						_bindableMap = static_cast<BindableActionMap>(
						    static_cast<uint64_t>(_bindableMap) & ~static_cast<uint64_t>(_mouseModBindings[keyMask].second));
					}
				}
			}
			else if (_mouseBindings.contains(newComboMask))
			{
				_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) |
				                                              static_cast<uint64_t>(_mouseBindings[newComboMask]));
				// Remove non-combo keys
				for (uint8_t i = 0; i < SDL_BUTTON_X2; ++i)
				{
					const uint8_t keyMask = (1 << i);
					if ((newComboMask & keyMask) != 0u && _mouseBindings.contains(keyMask))
					{
						_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
						                                              ~static_cast<uint64_t>(_mouseBindings[keyMask]));
					}
				}
			}
		}
		else if (_mouseModBindings.contains(buttonMask) && (_mouseModBindings[buttonMask].first & mod) != 0)
		{
			_bindableMap = static_cast<BindableActionMap>(
			    // Remove non-modded action
			    (static_cast<uint64_t>(_bindableMap) & ~static_cast<uint64_t>(_mouseBindings[buttonMask])) |
			    // Add modded action
			    static_cast<uint64_t>(_mouseModBindings[buttonMask].second));
		}
		else if (_mouseBindings.contains(buttonMask))
		{
			_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) |
			                                              static_cast<uint64_t>(_mouseBindings[buttonMask]));
		}
		_currentMouseButtons |= buttonMask;
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
	else if (event.type == SDL_MOUSEBUTTONUP && (event.button.button & SDL_BUTTON_LMASK) != 0 && event.button.clicks == 2)
	{
		_unbindableMap = static_cast<UnbindableActionMap>(static_cast<uint8_t>(_unbindableMap) &
		                                                  ~static_cast<uint8_t>(UnbindableActionMap::DOUBLE_CLICK));
	}
	else if (event.type == SDL_MOUSEBUTTONUP)
	{
		const uint8_t buttonMask = SDL_BUTTON(event.button.button);
		if (_mouseBindings.contains(buttonMask))
		{
			_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
			                                              ~static_cast<uint64_t>(_mouseModBindings[buttonMask].second));
			_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
			                                              ~static_cast<uint64_t>(_mouseBindings[buttonMask]));
		}
		// Remove combo keys
		for (uint8_t i = 0; i < SDL_BUTTON_X2; ++i)
		{
			const uint8_t keyMask = (1 << i);
			const uint8_t comboMask = keyMask | buttonMask;
			if (buttonMask != comboMask && _mouseBindings.contains(comboMask))
			{
				_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
				                                              ~static_cast<uint64_t>(_mouseModBindings[comboMask].second));
				_bindableMap = static_cast<BindableActionMap>(static_cast<uint64_t>(_bindableMap) &
				                                              ~static_cast<uint64_t>(_mouseBindings[comboMask]));
			}
		}
		_currentMouseButtons &= ~buttonMask;
	}
	else if (event.type == SDL_MOUSEMOTION)
	{
		_mouseDelta = {event.motion.xrel, event.motion.yrel};
	}
}

std::array<std::optional<glm::vec3>, 2> GameActionMap::GetHandPositions() const
{
	// Assume left-handed player since that's what the hand mesh is at the time of writing
	const auto& registry = Locator::entitiesRegistry::value();

	const auto& leftHand = std::make_optional(Game::Instance()->GetHand());
	// Only one hand at this point in time
	const auto& rightHand = std::optional<entt::entity>();

	std::optional<glm::vec3> leftPosition;
	std::optional<glm::vec3> rightPosition;
	auto getHandPosition = [&registry](auto hand) -> std::optional<glm::vec3> {
		return registry.Get<ecs::components::Transform>(hand).position;
	};
	leftPosition = leftHand.and_then(getHandPosition);
	rightPosition = rightHand.and_then(getHandPosition);

	// TODO(#693): Hand Getter should return an optional if the hand doesn't have a valid position
	// When the position is zero, it probably means it's not on the map (e.g. mouse is in the sky)
	if (leftPosition == glm::zero<glm::vec3>())
	{
		leftPosition = std::nullopt;
	}
	if (rightPosition == glm::zero<glm::vec3>())
	{
		rightPosition = std::nullopt;
	}

	return {{leftPosition, rightPosition}};
}
