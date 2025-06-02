/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PathFinding.h"

#include <SDL.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/PlayerSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::debug::gui;

PathFinding::PathFinding() noexcept
    : Window("Path Finding", ImVec2(400, 400))
{
}

void PathFinding::Draw() noexcept
{
	ImGui::TextWrapped(
	    "Select Villager by left clicking on it, then select a pathfinding action and right click the destination");

	const uint8_t numChildren = 2;
	const float rounding = 5.0f;
	const auto size = ImGui::GetWindowSize();
	const auto pos = ImGui::GetCursorPos();
	const float halfHeight = (size.y - pos.y) / numChildren - 2 * rounding;

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);
	ImGui::BeginChild("Selected Villager", ImVec2(0, halfHeight), ImGuiChildFlags_Border);
	if (_selectedVillager.has_value())
	{
		using namespace ecs::components;
		auto& registry = Locator::entitiesRegistry::value();

		ImGui::Columns(2);

		ImGui::Text("Villager %u", static_cast<uint32_t>(*_selectedVillager));
		ImGui::NextColumn();

		if (ImGui::Button("Clear Selection"))
		{
			_selectedVillager.reset();
		}
		ImGui::NextColumn();

		ImGui::Columns(1);

		if (_selectedVillager.has_value())
		{
			auto& transform = registry.Get<Transform>(_selectedVillager.value());
			auto& wallHug = registry.Get<WallHug>(_selectedVillager.value());
			ImGui::DragFloat3("Position", glm::value_ptr(transform.position));
			ImGui::DragFloat2("Goal", glm::value_ptr(wallHug.goal));
			ImGui::DragFloat("Speed", &wallHug.speed);
		}
	}
	else
	{
		ImGui::Text("No villager selected");
	}
	ImGui::EndChild();

	ImGui::BeginChild("Actions", ImVec2(0, halfHeight), ImGuiChildFlags_Border);
	if (_selectedVillager.has_value())
	{
		if (ImGui::BeginTabBar("Actions"))
		{
			using namespace ecs::components;
			auto& registry = Locator::entitiesRegistry::value();
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

			if (ImGui::BeginTabItem("Move On Footpath"))
			{
				ImGui::DragFloat3("Destination", glm::value_ptr(_destination));
				ImGui::Columns(2);
				if (_selectedFootpath.has_value())
				{
					ImGui::Text("Footpath %u", static_cast<uint32_t>(*_selectedFootpath));
				}
				else
				{
					ImGui::TextWrapped("No footpath selected, left click near one");
				}
				ImGui::NextColumn();

				if (ImGui::Button("Clear Selection"))
				{
					_selectedFootpath.reset();
				}
				ImGui::NextColumn();

				ImGui::Columns(1);

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
				                    ImGui::GetStyle().Alpha * (_selectedFootpath.has_value() ? 1.0f : 0.5f));
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !_selectedFootpath.has_value());
				if (ImGui::Button("Execute"))
				{
					SPDLOG_LOGGER_ERROR(spdlog::get("pathfinding"), "TODO: Implement setting MOVE_TO_ON_FOOTPATH state");
					// TODO(bwrsandman):
					//   get current node from entity
					//   remove entity from current node's follower list
					//   update current node based on Transform(entity)->position
					//   add entity to new current node
					//   call SetupMobileMoveToPos
					//   call virtual function 0x8dc and if returns true, call 0x8fc and return true
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

void PathFinding::Update() noexcept
{
	using namespace ecs::components;
	auto& registry = Locator::entitiesRegistry::value();
	auto const& meshes = Locator::resources::value().GetMeshes();

	std::optional<glm::vec3> handPosition;
	const auto positions = Locator::handSystem::value().GetPlayerHandPositions(registry.GameContext().player);
	if (positions[static_cast<size_t>(PlayerHand::Side::Left)] || positions[static_cast<size_t>(PlayerHand::Side::Right)])
	{
		handPosition = positions[static_cast<size_t>(PlayerHand::Side::Left)].value_or(
		    positions[static_cast<size_t>(PlayerHand::Side::Right)].value_or(glm::zero<glm::vec3>()));
	}

	if (handPosition.has_value())
	{
		glm::bvec3 invalidValue {};
		for (glm::length_t l = 0; l < std::remove_reference_t<decltype(*handPosition)>::length(); ++l)
		{
			invalidValue[l] = glm::isnan((*handPosition)[l]) || glm::isinf((*handPosition)[l]);
		}
		if (glm::any(invalidValue))
		{
			return;
		}
		_handPosition = *handPosition;
		_handPosition.y = Locator::terrainSystem::value().GetHeightAt(glm::xz(_handPosition));
	}

	if (_handTo == HandTo::Entity)
	{
		bool found = false;

		if (!found)
		{
			auto& map = Locator::entitiesMap::value();
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
						found = true;
					}
				}
			}
		}

		if (!found)
		{
			registry.Each<Footpath>([this, &found](entt::entity entity, const Footpath& footpath) {
				if (found)
				{
					return;
				}
				for (int32_t i = 0; i < static_cast<int32_t>(footpath.nodes.size()) - 1; ++i)
				{
					const auto distance2Threshold = 4.0f;

					const auto& p1 = footpath.nodes[i].position;
					const auto& p2 = footpath.nodes[i + 1].position;
					const auto p1p2 = p2 - p1;

					const float t2 = glm::dot(_handPosition - p1, p1p2) / glm::length2(p1p2);
					const auto t = glm::clamp(t2, 0.0f, 1.0f);
					const auto proj = p1 + t * p1p2;

					if (glm::distance2(_handPosition, proj) < distance2Threshold)
					{
						_selectedFootpath = entity;
						found = true;
						return;
					}
				}
			});
		}
	}
	else if (_handTo == HandTo::Destination)
	{
		if (handPosition.has_value())
		{
			_destination = *handPosition;
		}
	}
	_handTo = HandTo::None;
}

void PathFinding::ProcessEventOpen(const SDL_Event& event) noexcept
{
	const ImGuiIO& io = ImGui::GetIO();
	switch (event.type)
	{
	default:
		break;
	case SDL_MOUSEBUTTONDOWN:
	{
		if (!io.WantCaptureMouse)
		{
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				_handTo = HandTo::Entity;
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

void PathFinding::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
