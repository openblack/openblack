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

#include <glm/vec3.hpp>

#include "3D/TempleDoors.h"
#include "3D/TempleInteriorInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{
class CameraModel;
class TempleCameraModel;

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
	[[nodiscard]] TempleDoors& GetDoors() override { return _doors; }
	[[nodiscard]] const TempleDoors& GetDoors() const override { return _doors; }
	[[nodiscard]] std::optional<TempleCursorHit> GetCursorHit() const override;
	void Escape() override;
	void RequestLeave() override { _leaveRequested = true; }
	void Update() override;
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
};
} // namespace openblack
