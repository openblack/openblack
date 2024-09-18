/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Camera.h"

#include <glm/gtx/euler_angles.hpp>

#include "ECS/Registry.h"
#include "Game.h"
#include "Camera/Camera.h"
#include "3D/TempleInteriorInterface.h"
#include "ECS/Systems/CameraPathSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::debug::gui;

Camera::Camera()
    : Window("Camera", ImVec2(50.0f, 25.0f))
    , _selectedCameraPath(entt::hashed_string("cam"))
{
}

void Camera::Draw() noexcept
{
	auto& cameraPathSystem = Locator::cameraPathSystem::value();
	const float fontSize = ImGui::GetFontSize();
	ImGui::BeginChild("cameraPaths", ImVec2(fontSize * 10.0f, 0));

	const ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
	DrawCameraResourceList();
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("cameraPathControls", ImVec2(0, 0), ImGuiChildFlags_Border, flags);
	if (ImGui::BeginMenuBar())
	{
		ImGui::TextUnformatted("Controls");
		ImGui::EndMenuBar();
	}
	if (ImGui::Button("Run"))
	{
		if (Locator::temple::value().Active())
		{
			Locator::camera::value().SetOrigin(Locator::temple::value().GetPosition());
		}
		cameraPathSystem.Start(_selectedCameraPath);
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop"))
	{
		cameraPathSystem.Stop();
	}
	ImGui::SameLine();
	bool paused = cameraPathSystem.IsPaused();
	if (ImGui::Button(paused ? "Resume" : "Pause"))
	{
		if (paused)
		{
			cameraPathSystem.Play();
		} else
		{
			cameraPathSystem.Pause();
		}
	}

	DrawControls();
	ImGui::EndChild();
}

void Camera::DrawControls()
{
	auto const& cameraPaths = Locator::resources::value().GetCameraPaths();
	const auto& cameraPath = cameraPaths.Handle(_selectedCameraPath);
	if (cameraPath)
	{
		ImGui::BeginChild("cameraPathControls");
		const auto& pointCount = cameraPath->GetPoints().size();
		ImGui::Text("Points: %zu", pointCount);
		auto duration = std::chrono::duration<unsigned int, std::milli>(cameraPath->GetDuration()).count();
		ImGui::Text("Duration (milliseconds): %u", duration);
		ImGui::EndChild();
	}
	else
	{
		ImGui::Text("No paths selected");
	}
}

void Camera::DrawCameraResourceList()
{
	uint32_t displayedCameraPaths = 0;
	auto meshSize = ImGui::GetItemRectSize();
	const ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
	const bool isChildVisible =
	    ImGui::BeginChild("cameraPathList", ImVec2(meshSize.x, meshSize.y), ImGuiChildFlags_Border, flags);
	if (ImGui::BeginMenuBar())
	{
		ImGui::TextUnformatted("Camera Paths");
		ImGui::EndMenuBar();
	}
	if (isChildVisible)
	{
		auto const& cameraPaths = Locator::resources::value().GetCameraPaths();
		cameraPaths.Each([this, &displayedCameraPaths](entt::id_type id, const CameraPath& cameraPath) {
			displayedCameraPaths++;
			const auto& name = cameraPath.GetDebugName();
			if (ImGui::Selectable(!name.empty() ? name.c_str() : "unnamed path", id == _selectedCameraPath))
			{
				_selectedCameraPath = id;
			}
		});
	}
	ImGui::EndChild();
}

void Camera::Update() noexcept {}

void Camera::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) noexcept {}

void Camera::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
