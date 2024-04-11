/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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
#include <variant>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

union SDL_Event;

namespace openblack::input
{
// clang-format off
enum class BindableActionMap : uint64_t
{
	NONE                   = 0b000000000000000000000000000000000,
	ALL                    = 0b111111111111111111111111111111111,
	HELP                   = 0b000000000000000000000000000000001, // F1
	MOVE                   = 0b000000000000000000000000000000010, // LMB
	ACTION                 = 0b000000000000000000000000000000100, // RMB
	ZOOM_OUT               = 0b000000000000000000000000000001000, // wheel down
	ZOOM_IN                = 0b000000000000000000000000000010000, // wheel up
	TALK                   = 0b000000000000000000000000000100000, // T
	ZOOM_ON                = 0b000000000000000000000000001000000, // ctrl + arrows / lmb + rmb + mouse_motion
	MOVE_LEFT              = 0b000000000000000000000000010000000, // left
	MOVE_RIGHT             = 0b000000000000000000000000100000000, // right
	MOVE_FORWARDS          = 0b000000000000000000000001000000000, // up
	MOVE_BACKWARDS         = 0b000000000000000000000010000000000, // down
	TILT_UP                = 0b000000000000000000000100000000000, // a
	TILT_DOWN              = 0b000000000000000000001000000000000, // q
	ROTATE_LEFT            = 0b000000000000000000010000000000000, // z
	ROTATE_RIGHT           = 0b000000000000000000100000000000000, // u
	ROTATE_ON              = 0b000000000000000001000000000000000, // shift
	ROTATE_AROUND_MOUSE_ON = 0b000000000000000010000000000000000, // mmb
	ZOOM_TO_TEMPLE         = 0b000000000000000100000000000000000, // space
	ZOOM_TO_CREATURE       = 0b000000000000001000000000000000000, // c
	ZOOM_TO_REALM          = 0b000000000000010000000000000000000, // f_3
	ZOOM_TO_INSIDE_TEMPLE  = 0b000000000000100000000000000000000, // f_4
	ZOOM_TO_CREATURE_ROOM  = 0b000000000001000000000000000000000, // f_5
	ZOOM_TO_CHALLENGE_ROOM = 0b000000000010000000000000000000000, // f_6
	ZOOM_TO_SAVE_GAME_ROOM = 0b000000000100000000000000000000000, // f_7
	ZOOM_TO_OPTIONS_ROOM   = 0b000000001000000000000000000000000, // f_8
	ZOOM_TO_LIBRARY        = 0b000000010000000000000000000000000, // f_9
	LEASH_UNLEASH_CREATURE = 0b000000100000000000000000000000000, // l
	SHOW_VILLAGER_NAMES    = 0b000001000000000000000000000000000, // n
	SHOW_VILLAGER_DETAILS  = 0b000010000000000000000000000000000, // s
	QUICK_SAVE             = 0b000100000000000000000000000000000, // ctrl + s
	QUICK_LOAD             = 0b001000000000000000000000000000000, // ctrl + l
	PREVIOUS_LEASH         = 0b010000000000000000000000000000000, // v
	NEXT_LEASH             = 0b100000000000000000000000000000000, // B
};
enum class UnbindableActionMap : uint8_t
{
	NONE                   = 0b00000000,
	DOUBLE_CLICK           = 0b00000001,
	TWO_BUTTON_CLICK       = 0b00000010, // Is in the bindable menu in vanilla but in practice it is not.
};
using ActionMap = std::variant<BindableActionMap, UnbindableActionMap>;
// clang-format on

class GameActionInterface
{
	template <typename T, typename... Args>
	static T GetAnyAccumulateActions(Args... actions)
	{
		T accumulator = T::NONE;
		((accumulator = static_cast<T>(static_cast<std::underlying_type_t<T>>(accumulator) |
		                               static_cast<std::underlying_type_t<T>>(actions))),
		 ...);
		return accumulator;
	}

public:
	// clang-format is having trouble with requires
	// clang-format off
	template <typename... Args>
	    requires(... && (std::is_same_v<Args, BindableActionMap> || std::is_same_v<Args, UnbindableActionMap>))
	[[nodiscard]] bool GetAny(Args... actions) const
	{
		const auto bindableAccumulator = GetAnyAccumulateActions<BindableActionMap>(actions...);
		const auto unbindableAccumulator = GetAnyAccumulateActions<UnbindableActionMap>(actions...);
		return GetBindable(bindableAccumulator) || GetUnbindable(unbindableAccumulator);
	}

	template <typename... Args>
	    requires(... && (std::is_same_v<Args, BindableActionMap> || std::is_same_v<Args, UnbindableActionMap>))
	[[nodiscard]] bool GetChangedAny(Args... actions) const
	{
		const auto bindableAccumulator = GetAnyAccumulateActions<BindableActionMap>(actions...);
		const auto unbindableAccumulator = GetAnyAccumulateActions<UnbindableActionMap>(actions...);
		return GetBindableChanged(bindableAccumulator) || GetUnbindableChanged(unbindableAccumulator);
	}

	template <typename... Args>
	    requires(... && (std::is_same_v<Args, BindableActionMap> || std::is_same_v<Args, UnbindableActionMap>))
	[[nodiscard]] bool GetRepeatAny(Args... actions) const
	{
		const auto bindableAccumulator = GetAnyAccumulateActions<BindableActionMap>(actions...);
		const auto unbindableAccumulator = GetAnyAccumulateActions<UnbindableActionMap>(actions...);
		return GetBindableRepeat(bindableAccumulator) || GetUnbindableRepeat(unbindableAccumulator);
	}

	template <typename... Args>
	    requires(... && (std::is_same_v<Args, BindableActionMap> || std::is_same_v<Args, UnbindableActionMap>))
	[[nodiscard]] bool GetAll(Args... actions) const
	{
		return (Get(actions) && ...);
	}

	template <typename... Args>
	    requires(... && (std::is_same_v<Args, BindableActionMap> || std::is_same_v<Args, UnbindableActionMap>))
	[[nodiscard]] bool GetChangedAll(Args... actions) const
	{
		return (GetChanged(actions) && ...);
	}

	template <typename... Args>
	    requires(... && (std::is_same_v<Args, BindableActionMap> || std::is_same_v<Args, UnbindableActionMap>))
	[[nodiscard]] bool GetRepeatAll(Args... actions) const
	{
		return (GetRepeat(actions) && ...);
	}
	// clang-format on

	[[nodiscard]] bool Get(ActionMap action) const
	{
		return std::visit(
		    [&](auto&& arg) {
			    using T = std::decay_t<decltype(arg)>;
			    if constexpr (std::is_same_v<T, BindableActionMap>)
			    {
				    return GetBindable(arg);
			    }
			    else if constexpr (std::is_same_v<T, UnbindableActionMap>)
			    {
				    return GetUnbindable(arg);
			    }
		    },
		    action);
	}

	[[nodiscard]] bool GetChanged(ActionMap action) const
	{
		return std::visit(
		    [&](auto&& arg) {
			    using T = std::decay_t<decltype(arg)>;
			    if constexpr (std::is_same_v<T, BindableActionMap>)
			    {
				    return GetBindableChanged(arg);
			    }
			    else if constexpr (std::is_same_v<T, UnbindableActionMap>)
			    {
				    return GetUnbindableChanged(arg);
			    }
		    },
		    action);
	}

	[[nodiscard]] bool GetRepeat(ActionMap action) const
	{
		return std::visit(
		    [&](auto&& arg) {
			    using T = std::decay_t<decltype(arg)>;
			    if constexpr (std::is_same_v<T, BindableActionMap>)
			    {
				    return GetBindableRepeat(arg);
			    }
			    else if constexpr (std::is_same_v<T, UnbindableActionMap>)
			    {
				    return GetUnbindableRepeat(arg);
			    }
		    },
		    action);
	}

	[[nodiscard]] virtual bool GetBindable(BindableActionMap action) const = 0;
	[[nodiscard]] virtual bool GetUnbindable(UnbindableActionMap action) const = 0;
	[[nodiscard]] virtual bool GetBindableChanged(BindableActionMap action) const = 0;
	[[nodiscard]] virtual bool GetUnbindableChanged(UnbindableActionMap action) const = 0;
	[[nodiscard]] virtual bool GetBindableRepeat(BindableActionMap action) const = 0;
	[[nodiscard]] virtual bool GetUnbindableRepeat(UnbindableActionMap action) const = 0;
	[[nodiscard]] virtual glm::uvec2 GetMousePosition() const = 0;
	[[nodiscard]] virtual glm::ivec2 GetMouseDelta() const = 0;
	[[nodiscard]] virtual std::array<std::optional<glm::vec3>, 2> GetHandPositions() const = 0;

	virtual void Frame() = 0;
	virtual void ProcessEvent(const SDL_Event& event) = 0;
};
} // namespace openblack::input
