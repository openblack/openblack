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
#include "3D/TempleMap.h"
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
	[[nodiscard]] const std::vector<OrientedTextVertex>& GetMap() const override { return _mapTriangles; }
	[[nodiscard]] uint32_t GetVisits() const override { return _visits; }
	[[nodiscard]] const std::vector<TempleMapMarker>& GetMapMarkers() const override { return _mapMarkers; }
	[[nodiscard]] const std::vector<TempleCaveTrophy>& GetCaveTrophies() const override { return _caveTrophies; }
	[[nodiscard]] const TempleLight& GetLight() const override { return _light; }
	[[nodiscard]] float GetAlignment() const override { return _alignment; }
	void SetAlignment(float alignment) override { _alignment = alignment; }
	[[nodiscard]] float GetMapMarkerTurn() const override { return _mapMarkerTurn; }
	[[nodiscard]] float GetPoolTime() const override { return _poolTime; }
	void Escape() override;
	void RequestLeave() override { _leaveRequested = true; }
	void FadeToWhite() override;
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
	/// The island over the main room's pool, and its triangles this frame
	TempleMap _map;
	std::vector<OrientedTextVertex> _mapTriangles;
	/// The markers on the map this frame, and how far they have turned
	std::vector<TempleMapMarker> _mapMarkers;
	std::vector<TempleCaveTrophy> _caveTrophies;
	/// The alignment of the most influential player where the camera was outside, which fn_005E2240 keeps for the sky
	/// TODO(raffclar): openblack has no players' alignments yet, so the temple is neutral unless the debug window says
	float _alignment {0.0f};
	TempleLight _light;
	/// CreatureRoom::UpdateBeltsAndMedals: the belts for how the creature fights and the medals for its miracles, at the
	/// points of the room's mesh
	void UpdateCaveTrophies();
	float _mapMarkerTurn {0.0f};
	/// How long the main room's pool has shimmered, in seconds (0xE3A16C)
	float _poolTime {0.0f};
	/// WorldRoom::DrawAdditional's markers of the temples and creatures
	void UpdateMapMarkers(float seconds);
	/// Where the camera is to look from and at as the player leaves for a place double clicked on the map
	std::optional<std::pair<glm::vec3, glm::vec3>> _leaveTo;
	/// Whether the temple is fading to white to leave for the place on the map, which it does once the screen is white
	bool _leavingForMapPoint {false};
	/// WorldRoomCamera's double click on the map: leaves the temple for that place
	void LeaveForMapPoint(glm::vec3 point);
	/// Covers the screen as the player is put straight into a room, and fades it back over a second and a fifth
	void FadeIntoRoom();
	uint32_t _visits {0};
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
