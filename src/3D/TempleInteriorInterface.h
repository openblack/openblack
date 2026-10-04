/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Graphics/GraphicsHandle.h"

namespace openblack
{
namespace graphics
{
class Texture2D;
}
namespace gui
{
class GameInterface;
} // namespace gui
struct OrientedTextVertex;
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
	/// The room whose mesh the cursor is over, and the submesh of it, when it is over one
	TempleRoom room {TempleRoom::Unknown};
	std::optional<uint32_t> subMesh;
};

/// A texture a room's mesh draws one of its submeshes with, in place of its skin
struct TempleSubMeshTexture
{
	uint32_t subMesh;
	graphics::TextureHandle texture;
};

/// A colour added to one of a room's submeshes, from 0 to 1, as a control glows under the cursor
struct TempleSubMeshGlow
{
	uint32_t subMesh;
	glm::vec3 colour;
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
	/// Whether a room is drawn whole (Temple::Draw): the room the player is in, the room the camera is on its way into,
	/// and the main room from the others while one of its doors is open (WorldRoom::DrawDoors)
	[[nodiscard]] virtual bool IsRoomDrawn(TempleRoom room) const = 0;
	/// The main room's doors, which swing open as the camera walks through them
	[[nodiscard]] virtual TempleDoors& GetDoors() = 0;
	[[nodiscard]] virtual const TempleDoors& GetDoors() const = 0;
	/// Where the cursor last met the room, which HandStateCitadel puts the hand by
	[[nodiscard]] virtual std::optional<TempleCursorHit> GetCursorHit() const = 0;
	/// The game's interface, whose text and font the rooms write their scrolls and signs with, whose options the options
	/// room opens and over which the future room shows its words, or null without one
	virtual void SetInterface(gui::GameInterface* interface) = 0;
	/// A press on one of the rooms' controls takes the mouse (TempleRoom::UpdateMouse): on a scroll, dragging the mouse up
	/// and down turns it, and letting go over a button presses it. True while a control has the press, which the camera
	/// then leaves alone.
	virtual bool HoldControl(bool pressed, float mouseY) = 0;
	/// The scrolls' textures for a room's mesh
	[[nodiscard]] virtual std::vector<TempleSubMeshTexture> GetScrollTextures(TempleRoom room) const = 0;
	/// The submeshes of a room's mesh its controls leave undrawn, as each of the main room's buttons draws only one of
	/// its pair (SubOptionEntry::GetSubMeshData)
	[[nodiscard]] virtual std::vector<uint32_t> GetHiddenSubMeshes(TempleRoom room) const = 0;
	/// The glow of the control the cursor is over in a room's mesh, if it glows (SubOptionEntry::GetSubMeshData)
	[[nodiscard]] virtual std::vector<TempleSubMeshGlow> GetControlGlows(TempleRoom room) const = 0;
	/// The text written in the rooms this frame, the signs' labels and the text of the scroll the camera is close to, in
	/// triangles of the font's texture, in the temple
	[[nodiscard]] virtual const std::vector<OrientedTextVertex>& GetText() const = 0;
	/// The font's texture the text is drawn from, null without the game's interface
	[[nodiscard]] virtual const graphics::Texture2D* GetTextTexture() const = 0;
	/// Escape takes the player back to the main room, and out of the temple from there (Temple's key handling)
	virtual void Escape() = 0;
	/// Leaves the temple once the frame is done with it
	virtual void RequestLeave() = 0;
	/// Carries out what the frame asked of the temple, and moves on what goes on in its rooms by themselves
	virtual void Update(std::chrono::microseconds dt) = 0;
	/// How far the creature's room's waterfall has slid down it, the offset of its texture (CreatureRoom::Draw)
	[[nodiscard]] virtual glm::vec2 GetWaterfallSlide() const = 0;
	virtual void Activate() = 0;
	virtual void Activate(TempleRoom room) = 0;
	virtual void Deactivate() = 0;
};
} // namespace openblack
