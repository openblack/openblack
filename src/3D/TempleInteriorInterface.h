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
class TempleDoors;

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

/// Where the cursor meets the temple's room, and the way the surface there faces
struct TempleCursorHit
{
	glm::vec3 point;
	glm::vec3 normal;
};

class TempleInteriorInterface
{
public:
	virtual ~TempleInteriorInterface() = default;

	[[nodiscard]] virtual bool Active() const = 0;
	[[nodiscard]] virtual glm::vec3 GetPosition() const = 0;
	/// The room the player is in. Temple::Draw draws it, and the main room when it is another.
	[[nodiscard]] virtual TempleRoom GetCurrentRoom() const = 0;
	virtual void SetCurrentRoom(TempleRoom room) = 0;
	/// The room the camera is on its way into through a door, drawn as well while it is
	[[nodiscard]] virtual std::optional<TempleRoom> GetTransitionRoom() const = 0;
	virtual void SetTransitionRoom(std::optional<TempleRoom> room) = 0;
	/// Temple::GoToRoom: cuts to a room
	virtual void GoToRoom(TempleRoom room) = 0;
	/// Walks through the main room's door to a room, or cuts to it from elsewhere
	virtual void EnterRoom(TempleRoom room) = 0;
	/// The main room's doors, which swing open as the camera walks through them
	[[nodiscard]] virtual TempleDoors& GetDoors() = 0;
	[[nodiscard]] virtual const TempleDoors& GetDoors() const = 0;
	/// Where the cursor last met the room, which HandStateCitadel puts the hand by
	[[nodiscard]] virtual std::optional<TempleCursorHit> GetCursorHit() const = 0;
	/// Escape takes the player back to the main room, and out of the temple from there (Temple's key handling)
	virtual void Escape() = 0;
	/// Leaves the temple once the frame is done with it
	virtual void RequestLeave() = 0;
	/// Carries out what the frame asked of the temple
	virtual void Update() = 0;
	virtual void Activate() = 0;
	virtual void Activate(TempleRoom room) = 0;
	virtual void Deactivate() = 0;
};
} // namespace openblack
