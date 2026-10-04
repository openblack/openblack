/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <memory>
#include <vector>

#include <glm/vec3.hpp>

#include "3D/OrientedText.h"
#include "3D/TempleDoors.h"
#include "3D/TempleInteriorInterface.h"
#include "3D/TempleToggles.h"
#include "3D/TempleToolTips.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{
class CameraModel;
class CreatureCaveEffects;
class TempleCameraModel;
class TempleScrolls;
class TempleSigns;

class TempleInterior final: public TempleInteriorInterface
{
public:
	TempleInterior();
	~TempleInterior() override;

	[[nodiscard]] bool Active() const override { return _active; }
	[[nodiscard]] glm::vec3 GetPosition() const override { return _templePosition; }
	[[nodiscard]] TempleRoom GetCurrentRoom() const override { return _currentRoom; }
	void SetCurrentRoom(TempleRoom room) override { _currentRoom = room; }
	[[nodiscard]] std::optional<TempleRoom> GetTransitionRoom() const override { return _transitionRoom; }
	void SetTransitionRoom(std::optional<TempleRoom> room) override { _transitionRoom = room; }
	void GoToRoom(TempleRoom room) override;
	void EnterRoom(TempleRoom room) override;
	[[nodiscard]] bool IsRoomDrawn(TempleRoom room) const override;
	[[nodiscard]] TempleDoors& GetDoors() override { return _doors; }
	[[nodiscard]] const TempleDoors& GetDoors() const override { return _doors; }
	[[nodiscard]] std::optional<TempleCursorHit> GetCursorHit() const override;
	void SetInterface(gui::GameInterface* interface) override;
	bool HoldControl(bool pressed, float mouseY) override;
	[[nodiscard]] std::vector<TempleSubMeshTexture> GetScrollTextures(TempleRoom room) const override;
	[[nodiscard]] std::vector<uint32_t> GetHiddenSubMeshes(TempleRoom room) const override;
	[[nodiscard]] const std::vector<OrientedTextVertex>& GetText() const override { return _text; }
	[[nodiscard]] std::vector<TempleSubMeshGlow> GetControlGlows(TempleRoom room) const override;
	[[nodiscard]] const graphics::Texture2D* GetTextTexture() const override;
	void Escape() override;
	void RequestLeave() override { _leaveRequested = true; }
	void Update(std::chrono::microseconds dt) override;
	[[nodiscard]] glm::vec2 GetWaterfallSlide() const override;
	void Activate() override { Activate(TempleRoom::Main); }
	void Activate(TempleRoom room) override;
	void Deactivate() override;

private:
	/// Looks through the camera model's lens, or the configured one without
	void ApplyLens() const;

	bool _active {false};
	bool _leaveRequested {false};
	/// Temple::InitEngine starts in the main room unless told of another
	TempleRoom _currentRoom {TempleRoom::Main};
	std::optional<TempleRoom> _transitionRoom;
	/// InnerRoom::InitMesh places each room at the origin, unturned: the temple is a scene of its own
	glm::vec3 _templePosition {0.0f};
	glm::vec3 _templeRotation {0.0f};
	glm::vec3 _playerPositionOutside {0.0f};
	glm::vec3 _playerRotationOutside {0.0f};
	/// The camera's model outside, while the temple's has the camera
	std::unique_ptr<CameraModel> _outsideCameraModel;
	TempleCameraModel* _cameraModel {nullptr};
	TempleDoors _doors;
	/// The main room's buttons of what its map shows, which keep their state from one visit to the next
	TempleToggles _toggles;
	/// How far through its slide the waterfall's texture is, from 0 to 1
	float _waterfallSlide {0.0f};
	/// Whether the creature's room's sounds of its water and fire are playing
	bool _soundsOfCreatureCave {false};
	/// The flames, smoke, spray and mist of the creature's room
	std::unique_ptr<CreatureCaveEffects> _creatureCaveEffects;
	/// The game's interface, whose text and font the scrolls and signs are written with
	gui::GameInterface* _interface {nullptr};
	/// The rooms' scrolls, written as the temple opens, and the labels of their signs
	std::unique_ptr<TempleScrolls> _scrolls;
	std::unique_ptr<TempleSigns> _signs;
	/// The rooms' InitEngine: makes and writes the scrolls, and finds the signs
	void CreateScrolls();
	/// This frame's text in the rooms
	std::vector<OrientedTextVertex> _text;
	/// Whether the options room has opened the game's options (GameOptionsRoom +0x160)
	bool _optionsShown {false};
	/// How long the future room has shown its words, which fade in (UniverseRoom +0x168)
	float _futureTime {0.0f};
	/// The submesh the cursor is over, and how long is left of the glow it set off, in milliseconds (TempleRoom +0xC8
	/// and 0xE36134). Moving onto another submesh sets the glow off again, unless a control is being dragged.
	std::optional<std::pair<TempleRoom, uint32_t>> _hovered;
	float _hoverGlow {0.0f};
	/// What the hand shows, which the rooms choose every frame and submit every turn, and how long since the last turn
	TempleToolTip _toolTip {k_FirstTempleToolTip};
	float _toolTipTurnTime {0.0f};
	/// GameOptionsRoom::Update and UniverseRoom::Update and DrawAdditional
	void UpdateOptionsAndFutureRooms(float seconds);
	/// The tooltip the hand shows, and where on the screen the hand is
	void UpdateToolTips(float milliseconds);
	void StopCreatureCaveSounds();
	/// Whether a submesh of a room's mesh is one of its controls drawn, and whether a control has the mouse
	[[nodiscard]] bool IsControl(TempleRoom room, uint32_t subMesh) const;
	[[nodiscard]] bool IsControlHeld() const;
};
} // namespace openblack
