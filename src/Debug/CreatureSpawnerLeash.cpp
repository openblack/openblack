/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <limits>
#include <optional>
#include <string>

#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <imgui.h>

#include "Camera/Camera.h"
#include "Creature/CreatureDesires.h"
#include "Creature/LeashRope.h"
#include "Creature/LeashRules.h"
#include "CreatureSpawner.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureLeash.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::debug::gui;
using openblack::ecs::components::Creature;
using openblack::ecs::components::CreatureLeash;
using openblack::ecs::components::CreatureMindState;
using openblack::ecs::components::LeashPost;
using openblack::ecs::components::Mesh;
using openblack::ecs::components::Transform;
namespace leash = openblack::creature_leash;

namespace
{
constexpr float k_BarWidth = 160.0f;
/// The posts placed by hand stand this far apart round the hand
constexpr float k_PostSpacing = 6.0f;
/// How far round the creature to look for something to tie the leash to
constexpr float k_TieSearch = 400.0f;

std::optional<glm::vec3> HandPoint()
{
	if (!Locator::handSystem::has_value())
	{
		return std::nullopt;
	}
	return Locator::handSystem::value()
	    .GetPlayerHandPositions()[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)];
}

/// The nearest thing with a mesh, other than creatures and leash posts, within reach of a point
std::optional<entt::entity> NearestObject(const ecs::Registry& registry, const glm::vec3& point)
{
	std::optional<entt::entity> nearest;
	float best = k_TieSearch;
	registry.Each<const Mesh, const Transform>([&](entt::entity entity, const Mesh& /*mesh*/, const Transform& at) {
		if (registry.TryGet<const Creature>(entity) != nullptr || registry.TryGet<const LeashPost>(entity) != nullptr)
		{
			return;
		}
		if (const auto distance = glm::distance(at.position, point); distance < best)
		{
			best = distance;
			nearest = entity;
		}
	});
	return nearest;
}

/// Another creature than this one, the nearest
std::optional<entt::entity> OtherCreature(const ecs::Registry& registry, entt::entity self, const glm::vec3& point)
{
	std::optional<entt::entity> nearest;
	float best = std::numeric_limits<float>::max();
	registry.Each<const Creature, const Transform>([&](entt::entity entity, const Creature& /*creature*/, const Transform& at) {
		if (entity == self)
		{
			return;
		}
		if (const auto distance = glm::distance(at.position, point); distance < best)
		{
			best = distance;
			nearest = entity;
		}
	});
	return nearest;
}
} // namespace

void CreatureSpawner::DrawLeash(entt::entity entity) noexcept
{
	if (!Locator::leashSystem::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	auto& leashes = Locator::leashSystem::value();
	const auto* transform = registry.TryGet<const Transform>(entity);
	if (transform == nullptr)
	{
		return;
	}
	ImGui::SeparatorText("Leash");

	// Who the creature belongs to, and whether it is the one creature its owner can lead
	const auto& body = registry.Get<const Creature>(entity);
	const auto ownerName = [](PlayerNames owner) {
		return owner == PlayerNames::NEUTRAL ? std::string("Neutral") : fmt::format("Player {}", static_cast<int>(owner) + 1);
	};
	ImGui::SetNextItemWidth(k_BarWidth);
	if (ImGui::BeginCombo("Owner", ownerName(body.owner).c_str()))
	{
		for (size_t i = 0; i < static_cast<size_t>(PlayerNames::_COUNT); ++i)
		{
			const auto owner = static_cast<PlayerNames>(i);
			if (ImGui::Selectable(ownerName(owner).c_str(), owner == body.owner))
			{
				leashes.SetOwner(entity, owner);
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	bool leashable = leashes.IsLeashable(entity);
	ImGui::BeginDisabled(!leash::CanLead(body.owner));
	if (ImGui::Checkbox("Leashable", &leashable))
	{
		leashes.SetLeashable(entity, leashable);
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("The one creature its owner can lead; making it so stops their other creature being it");
	}
	if (const auto refused = leashes.LastRefusal())
	{
		ImGui::TextDisabled("Last refused: player %d, creature %u: %s", static_cast<int>(refused->player) + 1,
		                    entt::to_integral(refused->creature), leash::Describe(refused->why));
	}

	// Which leashes it knows, as the scripts grant them
	ImGui::TextUnformatted("Knows:");
	for (const auto type : leash::k_Types)
	{
		ImGui::SameLine();
		bool known = leashes.Knows(entity, type);
		if (ImGui::Checkbox(leash::Name(type), &known))
		{
			leashes.SetKnown(entity, type, known);
		}
	}
	if (ImGui::Button("Grant all"))
	{
		for (const auto type : leash::k_Types)
		{
			leashes.SetKnown(entity, type, true);
		}
	}

	// Putting each leash on, held in the hand
	for (const auto type : leash::k_Types)
	{
		ImGui::SameLine();
		ImGui::BeginDisabled(!leashes.Knows(entity, type) || !leashes.Knows(entity, LeashType::Rope));
		if (ImGui::Button(fmt::format("Put on {}", leash::Name(type)).c_str()))
		{
			if (leashes.IsLeashed(entity))
			{
				leashes.ChangeType(entity, type);
			}
			else
			{
				leashes.PutOn(entity, type);
			}
		}
		ImGui::EndDisabled();
	}

	const auto* state = registry.TryGet<const CreatureLeash>(entity);
	const bool leashed = leashes.IsLeashed(entity);
	ImGui::BeginDisabled(!leashed);
	if (ImGui::Button("Take off"))
	{
		leashes.TakeOff(entity);
	}
	ImGui::SameLine();
	if (ImGui::Button("Tie to nearest thing"))
	{
		if (const auto object = NearestObject(registry, transform->position))
		{
			leashes.TieTo(entity, *object);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Tie to other creature"))
	{
		if (const auto other = OtherCreature(registry, entity, transform->position))
		{
			leashes.TieTo(entity, *other);
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!leashes.TiedTo(entity).has_value());
	if (ImGui::Button("Untie to hand"))
	{
		leashes.UntieToHand(entity);
	}
	ImGui::EndDisabled();
	ImGui::EndDisabled();

	if (state != nullptr && state->worn.has_value())
	{
		const auto& worn = *state->worn;
		bool works = worn.works;
		if (ImGui::Checkbox("Leash works", &works))
		{
			leashes.SetWorks(entity, works);
		}
		ImGui::SameLine();
		bool drawn = state->drawn;
		if (ImGui::Checkbox("Draw rope", &drawn))
		{
			leashes.SetDrawn(drawn);
		}
		ImGui::Text("%s leash, %s", leash::Name(worn.type),
		            worn.tiedTo.has_value() ? fmt::format("tied to entity {}", static_cast<uint32_t>(*worn.tiedTo)).c_str()
		                                    : "held in the hand");
		ImGui::Text("Tension");
		ImGui::SameLine();
		ImGui::ProgressBar(worn.rope.tension, ImVec2(k_BarWidth, 0.0f), fmt::format("{:.2f}", worn.rope.tension).c_str());
		ImGui::SameLine();
		ImGui::TextUnformatted(leash::ShouldPull(worn.rope.tension) ? "pulling" : "");
		ImGui::Text("Length %.1f (rest %.1f, full %.1f)", glm::distance(worn.rope.start, worn.rope.end), worn.rope.slackLength,
		            worn.rope.maxLength);
	}
	else
	{
		ImGui::Text("No leash on (picked: %s)", state != nullptr ? leash::Name(state->selected) : "none");
	}
	if (state != nullptr)
	{
		ImGui::Text("Led: %s, pull %.2f",
		            state->control == CreatureLeash::Control::WalkingToHand ? "walking to the hand" : "not controlling",
		            state->pull);
		if (state->confinementRadius > 0.0f)
		{
			ImGui::Text("Kept within %.1f of (%.0f, %.0f)%s", state->confinementRadius, state->confinementCentre.x,
			            state->confinementCentre.z, state->returning ? ", returning" : "");
		}
	}
	ImGui::Text("Free of home: %s", leashes.FreeOfHome(entity) ? "yes" : "no");
	if (ImGui::Button("Keep at home"))
	{
		leashes.ConfineToHome(entity, leash::k_HomeConfinement);
	}
	ImGui::SameLine();
	if (ImGui::Button("Let roam"))
	{
		leashes.ClearConfinement(entity);
	}

	// What the leash tells the mind, for the learning to come
	if (const auto* mind = registry.TryGet<const CreatureMindState>(entity))
	{
		const auto& hooks = mind->leash;
		ImGui::Text(
		    "Mind: %s%s, miracles count x%u, %zu shown, %zu to act on",
		    hooks.forcedDesire.has_value()
		        ? fmt::format("forced {} {:.0f}", creature_desires::Name(*hooks.forcedDesire), hooks.forcedValue).c_str()
		        : "no forced desire",
		    hooks.learningInHand ? ", copies in hand" : "", hooks.miracleSightingWeight, hooks.shown.size(),
		    hooks.actOn.size());
	}

	if (ImGui::Button("Place leash posts at the hand"))
	{
		if (const auto hand = HandPoint())
		{
			const auto owner = registry.Get<const Creature>(entity).owner;
			leashes.PlacePosts(
			    owner, {*hand + glm::vec3(-k_PostSpacing, 0.0f, 0.0f), *hand, *hand + glm::vec3(k_PostSpacing, 0.0f, 0.0f)});
		}
	}
	ImGui::SameLine();
	ImGui::Checkbox("Show rope points", &_showRope);
	if (_showRope)
	{
		DrawRope(entity);
	}
}

void CreatureSpawner::DrawRope(entt::entity entity) noexcept
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* state = registry.TryGet<const CreatureLeash>(entity);
	if (state == nullptr || !state->worn.has_value() || !state->worn->ropeStarted)
	{
		return;
	}
	const auto& rope = state->worn->rope;
	const auto& camera = Locator::camera::value();
	const auto display = ImGui::GetIO().DisplaySize;
	const glm::vec4 viewport {0.0f, 0.0f, display.x, display.y};
	auto* drawList = ImGui::GetBackgroundDrawList();
	const auto project = [&](const glm::vec3& world) -> std::optional<ImVec2> {
		glm::vec3 screen;
		if (!camera.ProjectWorldToScreen(world, viewport, screen))
		{
			return std::nullopt;
		}
		return ImVec2(screen.x, screen.y);
	};
	// Each segment from white, slack, to red, stretched by a tenth or more
	for (size_t i = 1; i < leash_rope::k_PointCount; ++i)
	{
		const auto from = project(leash_rope::Point(rope, i - 1));
		const auto to = project(leash_rope::Point(rope, i));
		if (!from || !to)
		{
			continue;
		}
		const auto stretch = i - 1 < leash_rope::k_NodeCount ? rope.nodes.at(i - 1).stretch : 0.0f;
		const auto heat = static_cast<int>(std::clamp(stretch * 10.0f, 0.0f, 1.0f) * 255.0f);
		drawList->AddLine(*from, *to, IM_COL32(255, 255 - heat, 255 - heat, 255), 1.5f);
		drawList->AddCircleFilled(*to, 2.0f, IM_COL32(255, 220, 60, 255));
	}
	if (const auto start = project(rope.start))
	{
		drawList->AddCircle(*start, 6.0f, IM_COL32(80, 200, 255, 255), 12, 2.0f);
		drawList->AddText(ImVec2(start->x + 8.0f, start->y), IM_COL32(255, 255, 255, 255),
		                  fmt::format("tension {:.2f}", rope.tension).c_str());
	}
}
