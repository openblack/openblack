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

#include "ECS/Components/CameraPathNode.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CameraPathSystemInterface.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::debug::gui;

Camera::Camera()
    : Window("Camera", ImVec2(50.0f, 50.0f))
    , _selectedCameraPath(entt::hashed_string("cam"))
{
}

void Camera::Draw([[maybe_unused]] openblack::Game& game)
{
	const float fontSize = ImGui::GetFontSize();
	ImGui::BeginChild("cameraPaths", ImVec2(fontSize * 10.0f, 0));

	const ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
	DrawCameraResourceList();
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("cameraPathControls", ImVec2(0, 0), ImGuiChildFlags_Border, flags);
	if (ImGui::BeginMenuBar())
	{
		ImGui::TextUnformatted("Camera Paths");
		ImGui::EndMenuBar();
	}
	if (ImGui::Button("Create Instance"))
	{
		Locator::cameraPathSystem::value().Start(_selectedCameraPath);
	}
	ImGui::SameLine();
	auto& registry = Locator::entitiesRegistry::value();
	if (ImGui::Button("Delete Instance") && registry.Valid(_selectedCameraPathInstance))
	{
		Locator::cameraPathSystem::value().Stop(_selectedCameraPathInstance);
	}

	ImGui::BeginChild("cameraPathDetails", ImVec2(0, 0), ImGuiChildFlags_Border, flags);
	{
		DrawInstanceList();
		ImGui::SameLine();
		DrawControls();
	}
	ImGui::EndChild();
	ImGui::EndChild();
}

void Camera::DrawControls()
{
	auto& cameraPathSystem = Locator::cameraPathSystem::value();
	auto const& cameraPaths = Locator::resources::value().GetCameraPaths();
	const auto& cameraPath = cameraPaths.Handle(_selectedCameraPath);
	if (cameraPath)
	{
		ImGui::BeginChild("cameraPathControls");
		if (ImGui::Button(cameraPathSystem.IsPaused() ? "Resume" : "Pause"))
		{
			cameraPathSystem.Pause(!cameraPathSystem.IsPaused());
		}
		const auto& pointCount = cameraPath->GetPoints().size();
		if (ImGui::BeginMenuBar())
		{
			ImGui::TextUnformatted("Selected Camera Path Information");
			ImGui::EndMenuBar();
		}
		ImGui::Text("Points: %zu", pointCount);
		ImGui::Text("Movement Speed: %u", cameraPath->GetMovementSpeed());
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

void Camera::DrawInstanceList()
{
	auto meshSize = ImGui::GetItemRectSize();
	auto childSize = ImVec2(meshSize.x / 4, meshSize.y);
	const ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
	const bool isChildVisible = ImGui::BeginChild("cameraPathInstancesList", childSize, ImGuiChildFlags_Border, flags);
	if (ImGui::BeginMenuBar())
	{
		ImGui::TextUnformatted("Instances");
		ImGui::EndMenuBar();
	}
	if (isChildVisible)
	{
		auto& registry = Locator::entitiesRegistry::value();
		uint32_t instances = 0;
		registry.Each<const ecs::components::CameraPathStart>([this, &instances](entt::entity entity, auto component) {
			if (component.from != _selectedCameraPath)
			{
				return;
			}
			const auto& name = fmt::format("{}", instances);
			if (ImGui::Selectable(name.c_str(), entity == _selectedCameraPathInstance))
			{
				_selectedCameraPathInstance = entity;
			}
			instances++;
		});
	}
	ImGui::EndChild();
}

void Camera::Update([[maybe_unused]] openblack::Game& game, [[maybe_unused]] const openblack::Renderer& renderer) {}

void Camera::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void Camera::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}
