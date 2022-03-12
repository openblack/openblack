/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "PathFinding.h"

#include <SDL.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/LandIsland.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"

using namespace openblack::gui;

PathFinding::PathFinding()
    : DebugWindow("Path Finding", ImVec2(400, 400))
    , _handTo(HandTo::None)
    , _handPosition(glm::zero<glm::vec3>())
    , _destination(glm::zero<glm::vec3>())
    , _selectedVillager()
{
}

void PathFinding::Draw(Game& game)
{
	ImGui::TextWrapped(
	    "Select Villager by left clicking on it, then select a pathfinding action and right click the destination");

	const uint8_t numChildren = 2;
	const float rounding = 5.0f;
	const auto size = ImGui::GetWindowSize();
	const auto pos = ImGui::GetCursorPos();
	const float halfHeight = (size.y - pos.y) / numChildren - 2 * rounding;

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);
	ImGui::BeginChild("Selected Villager", ImVec2(0, halfHeight), true);
	if (_selectedVillager.has_value())
	{
		using namespace ecs::components;
		auto& registry = game.GetEntityRegistry();

		ImGui::Columns(2);

		ImGui::Text("Villager %u", static_cast<uint32_t>(*_selectedVillager));
		ImGui::NextColumn();

		if (ImGui::Button("Clear Selection"))
		{
			_selectedVillager.reset();
		}
		ImGui::NextColumn();

		ImGui::Columns(1);

		auto& transform = registry.Get<Transform>(_selectedVillager.value());
		auto& wallHug = registry.Get<WallHug>(_selectedVillager.value());
		ImGui::DragFloat3("Position", glm::value_ptr(transform.position));
		ImGui::DragFloat2("Goal", glm::value_ptr(wallHug.goal));
		ImGui::DragFloat("Speed", &wallHug.speed);
	}
	else
	{
		ImGui::Text("No villager selected");
	}
	ImGui::EndChild();

	ImGui::BeginChild("Actions", ImVec2(0, halfHeight), true);
	if (_selectedVillager.has_value())
	{
		if (ImGui::BeginTabBar("Actions"))
		{
			using namespace ecs::components;
			auto& registry = game.GetEntityRegistry();
			if (ImGui::BeginTabItem("Teleport"))
			{
				ImGui::DragFloat3("Destination", glm::value_ptr(_destination));

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
				                    ImGui::GetStyle().Alpha * (_selectedVillager.has_value() ? 1.0f : 0.5f));
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !_selectedVillager.has_value());
				if (ImGui::Button("Execute"))
				{
					registry.Get<Transform>(*_selectedVillager).position = _destination;
				}
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Move To Point"))
			{
				ImGui::DragFloat3("Destination", glm::value_ptr(_destination));

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
				                    ImGui::GetStyle().Alpha * (_selectedVillager.has_value() ? 1.0f : 0.5f));
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !_selectedVillager.has_value());
				if (ImGui::Button("Execute"))
				{
					auto& wallHug = registry.Get<WallHug>(*_selectedVillager);
					wallHug.goal = glm::xz(_destination);
					registry.Remove<MoveStateLinearTag, MoveStateOrbitTag, MoveStateExitCircleTag, MoveStateStepThroughTag,
					                MoveStateFinalStepTag, MoveStateArrivedTag>(*_selectedVillager);
					registry.Assign<MoveStateLinearTag>(*_selectedVillager);
				}
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
	else
	{
		ImGui::Text("No action available");
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
}

void PathFinding::Update(Game& game, [[maybe_unused]] const openblack::Renderer& renderer)
{
	using namespace ecs::components;
	auto& registry = game.GetEntityRegistry();
	const auto& handTransform = registry.Get<Transform>(game.GetHand());
	auto const& meshes = Locator::resources::ref().GetMeshes();
	if (glm::any(glm::isnan(handTransform.position) || glm::isinf(handTransform.position)))
	{
		return;
	}
	_handPosition = handTransform.position;
	_handPosition.y = game.GetLandIsland().GetHeightAt(glm::xz(_handPosition));

	if (_handTo == HandTo::Villager)
	{
		auto& map = game.GetEntityMap();
		for (const auto& entity : map.GetMobileInGridCell(_handPosition))
		{
			if (registry.AllOf<Villager>(entity))
			{
				const auto& mesh = meshes.Handle(registry.Get<Mesh>(entity).id);
				const auto& transform = registry.Get<Transform>(entity);
				auto vectorTo = transform.position - _handPosition;
				const auto& bb = mesh->GetBoundingBox();
				auto point = glm::transpose(transform.rotation) * vectorTo;
				point.y = bb.Center().y; // Fix to ignore altitude
				if (bb.Contains(point))
				{
					_selectedVillager = entity;
				}
			}
		}
	}
	else if (_handTo == HandTo::Destination)
	{
		_destination = handTransform.position;
	}
	_handTo = HandTo::None;
}

void PathFinding::ProcessEventOpen(const SDL_Event& event)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (event.type)
	{
	case SDL_MOUSEBUTTONDOWN:
	{
		if (!io.WantCaptureMouse)
		{
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				_handTo = HandTo::Villager;
			}
			else if (event.button.button == SDL_BUTTON_RIGHT)
			{
				_handTo = HandTo::Destination;
			}
		}
	}
	break;
	}
}

void PathFinding::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}
