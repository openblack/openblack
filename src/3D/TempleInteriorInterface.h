/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <glm/vec3.hpp>

namespace openblack
{

enum class TempleRoom
{
	Challenge,
	CreatureCave,
	Credits,
	Main,
	Multi,
	Options,
	SaveGame,
	Unknown
};

class TempleInteriorInterface
{
public:
	[[nodiscard]] virtual bool Active() const = 0;
	[[nodiscard]] virtual glm::vec3 GetPosition() const = 0;
	/// The room the player is in. Temple::Draw draws it, and the main room when it is another.
	[[nodiscard]] virtual TempleRoom GetCurrentRoom() const = 0;
	virtual void SetCurrentRoom(TempleRoom room) = 0;
	/// The room the camera is on its way into through a door, drawn as well while it is
	[[nodiscard]] virtual std::optional<TempleRoom> GetTransitionRoom() const = 0;
	virtual void SetTransitionRoom(std::optional<TempleRoom> room) = 0;
	virtual void Activate() = 0;
	virtual void Activate(TempleRoom room) = 0;
	virtual void Deactivate() = 0;
};
} // namespace openblack
