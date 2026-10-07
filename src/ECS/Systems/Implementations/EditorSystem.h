/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>
#include <optional>

#include "Camera/KeyboardMoveSpeed.h"
#include "ECS/Systems/EditorSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack
{
class CameraModel;
class EditorCameraModel;
} // namespace openblack

namespace openblack::ecs::systems
{

class EditorSystem final: public EditorSystemInterface
{
public:
	EditorSystem();
	~EditorSystem() override;

	void Update(std::chrono::microseconds dt) override;
	void SetOpen(bool open) override;
	[[nodiscard]] bool IsOpen() const override { return _open; }
	[[nodiscard]] editor::EditorSelection& GetSelection() override { return _selection; }
	[[nodiscard]] const editor::EditorSelection& GetSelection() const override { return _selection; }
	void SetTool(Tool tool) override { _tool = tool; }
	[[nodiscard]] Tool GetTool() const override { return _tool; }
	[[nodiscard]] editor::Snapping& GetSnapping() override { return _snapping; }
	void SetCameraMode(CameraMode mode) override;
	[[nodiscard]] CameraMode GetCameraMode() const override { return _cameraMode; }
	void TurnCamera(glm::vec2 radians) override;
	void ZoomCamera(float steps) override;
	void SetCameraMoveSpeed(float speed) override;
	[[nodiscard]] float GetCameraMoveSpeed() const override { return _cameraMoveSpeed; }
	void FrameSelection() override;
	void StepTurn() override;
	[[nodiscard]] bool IsStepping() const override { return _stepFrom.has_value(); }

private:
	/// Whether the camera is driven by the editor's model now
	[[nodiscard]] bool OwnsCamera() const;
	/// Hands the camera back to the model it had before the editor took it
	void ReleaseCamera();
	/// Tells the editor's camera where the picked thing is
	void AimCamera();
	/// Gives the camera the editor's movement speed while open, and the game's own while closed
	void ApplyCameraMoveSpeed() const;

	bool _open {false};
	editor::EditorSelection _selection;
	Tool _tool {Tool::Select};
	editor::Snapping _snapping;
	CameraMode _cameraMode {CameraMode::Free};
	float _cameraMoveSpeed {k_KeyboardMoveSpeedDefault};
	/// The editor's camera model while it has the camera, and the player's, kept to be handed back
	EditorCameraModel* _cameraModel {nullptr};
	std::unique_ptr<CameraModel> _playerCameraModel;
	/// The turn a step started from, until the next turn has been played
	std::optional<uint32_t> _stepFrom;
};

} // namespace openblack::ecs::systems
