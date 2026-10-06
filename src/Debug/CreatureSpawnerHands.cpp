/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <numbers>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "3D/CreatureBody.h"
#include "3D/LandIslandInterface.h"
#include "Creature/CreatureFeedback.h"
#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureObjectActions.h"
#include "Creature/CreatureRig.h"
#include "CreatureSpawner.h"
#include "ECS/Archetypes/MobileObjectArchetype.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureObjectAction.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureHandSystemInterface.h"
#include "ECS/Systems/CreatureObjectActionSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::debug::gui;
using openblack::ecs::components::Creature;
using openblack::ecs::components::CreatureMindState;
using openblack::ecs::components::CreatureObjectAction;
using openblack::ecs::components::CreatureTownAttitude;
using openblack::ecs::components::MobileObject;
using openblack::ecs::components::Transform;

namespace
{
/// The things that can be put by a creature to pick up
constexpr std::array<std::pair<MobileObjectInfo, std::string_view>, 6> k_Objects {{
    {MobileObjectInfo::Ball, "Ball"},
    {MobileObjectInfo::EgyptBarrel, "Barrel"},
    {MobileObjectInfo::EgyptPotA, "Pot"},
    {MobileObjectInfo::MagicFood, "Food"},
    {MobileObjectInfo::WaterJug, "Water jug"},
    {MobileObjectInfo::MagicWood, "Wood"},
}};
/// What it can do to what it holds: stroke, shake, smell, examine
constexpr std::array<std::string_view, creature_object_actions::k_KeepAnimationCount> k_KeepNames {"Stroke", "Shake", "Smell",
                                                                                                   "Examine"};
/// Things are put this far from the creature for its size
constexpr float k_PutAtDistance = 10.0f;

std::string_view StateName(ecs::systems::CreatureObjectActionSystemInterface::State state)
{
	constexpr std::array<std::string_view, 4> k_Names {"idle", "busy", "done", "failed"};
	return k_Names.at(static_cast<size_t>(state));
}

std::string ObjectName(const ecs::Registry& registry, entt::entity entity)
{
	if (const auto* object = registry.TryGet<const MobileObject>(entity))
	{
		for (const auto& [type, name] : k_Objects)
		{
			if (type == object->type)
			{
				return fmt::format("{} {}", name, entt::to_integral(entity));
			}
		}
		return fmt::format("object {} (type {})", entt::to_integral(entity), static_cast<int>(object->type));
	}
	return fmt::format("entity {}", entt::to_integral(entity));
}
} // namespace

void CreatureSpawner::DrawHands(entt::entity entity) noexcept
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* creature = registry.TryGet<const Creature>(entity);
	const auto* transform = registry.TryGet<const Transform>(entity);
	if (creature == nullptr || transform == nullptr || !Locator::creatureObjectActionSystem::has_value())
	{
		return;
	}
	auto& hands = Locator::creatureObjectActionSystem::value();

	ImGui::SeparatorText("Hands");
	const auto held = hands.GetHeld(entity);
	ImGui::Text("Holding %s", held.has_value() ? ObjectName(registry, *held).c_str() : "nothing");
	if (const auto* action = registry.TryGet<const CreatureObjectAction>(entity))
	{
		const auto state = hands.GetState(entity);
		ImGui::Text("%s: %s%s%s", creature_object_actions::Name(action->kind).data(), StateName(state).data(),
		            action->failure.empty() ? "" : ", ", action->failure.c_str());
		if (action->phase == CreatureObjectAction::Phase::Playing &&
		    state == ecs::systems::CreatureObjectActionSystemInterface::State::Busy)
		{
			std::string slots;
			for (size_t i = 0; i < action->animationCount; ++i)
			{
				slots +=
				    fmt::format("{}{} {:+.2f}", slots.empty() ? "" : ", ", action->animations.at(i), action->weights.at(i));
			}
			ImGui::TextWrapped("%.0f of %.0f ms, %s at %.0f ms%s: %s", static_cast<double>(action->timeMs),
			                   static_cast<double>(action->durationMs), action->eventDone ? "acted" : "acts",
			                   static_cast<double>(action->eventMs), action->mirrored ? ", mirrored" : "", slots.c_str());
		}
		else if (action->phase == CreatureObjectAction::Phase::Approach && action->reach.has_value())
		{
			ImGui::Text("Getting in reach, try %u, reaches %.1f", action->attempts, static_cast<double>(action->maxReach));
		}
	}
	if (const auto* seen = registry.TryGet<const CreatureTownAttitude>(entity))
	{
		ImGui::Text("Its town sees it with %s, %.0f s more", creature_object_actions::Name(seen->attitude).data(),
		            static_cast<double>(seen->secondsLeft));
	}
	if (const auto* mind = registry.TryGet<const CreatureMindState>(entity))
	{
		if (mind->lastFeedback.has_value())
		{
			ImGui::Text("Last feedback %+.2f while %s", static_cast<double>(mind->lastFeedback->value),
			            creature_mind::Name(mind->lastFeedback->activity).data());
		}
		else
		{
			ImGui::TextUnformatted("No feedback from the player yet");
		}
		ImGui::Text("Attitude to the player %+.3f, average feedback %+.2f", static_cast<double>(mind->attitudeToPlayer),
		            static_cast<double>(mind->averageFeedback));
	}
	if (Locator::creatureHandSystem::has_value() && Locator::creatureHandSystem::value().GetCreature() == entity)
	{
		ImGui::Text("The hand is on it: %+.2f so far",
		            static_cast<double>(Locator::creatureHandSystem::value().GetFeedbackSum()));
	}
	const auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto rigId = creature::GetRigId(creature->species);
	if (rigs.Contains(rigId) && rigs.Handle(rigId)->actionPoints.has_value())
	{
		const auto& points = *rigs.Handle(rigId)->actionPoints;
		ImGui::TextDisabled("Hand %u, foot %u, head %u; pick up %.0f, eat %.0f, throw %.0f, put down %.0f ms", points.rightHand,
		                    points.rightFoot, points.head, static_cast<double>(points.pickUpMs),
		                    static_cast<double>(points.eatMs), static_cast<double>(points.throwMs),
		                    static_cast<double>(points.putDownMs));
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Armpit %u, belly %u, groin %u; destroy %.0f, discard %.0f ms", points.rightArmpit, points.belly,
			                  points.groin, static_cast<double>(points.destroyMs), static_cast<double>(points.discardMs));
		}
	}

	// Things to put by it
	if (ImGui::BeginCombo("Thing", k_Objects.at(static_cast<size_t>(_objectType)).second.data()))
	{
		for (size_t i = 0; i < k_Objects.size(); ++i)
		{
			if (ImGui::Selectable(k_Objects.at(i).second.data(), static_cast<int>(i) == _objectType))
			{
				_objectType = static_cast<int>(i);
			}
		}
		ImGui::EndCombo();
	}
	const auto putAt = [&](float angle) {
		if (!Locator::terrainSystem::has_value() || !Locator::infoConstants::has_value())
		{
			return;
		}
		const auto ahead = transform->rotation * glm::vec3(std::sin(angle), 0.0f, -std::cos(angle));
		auto at = transform->position + (ahead * k_PutAtDistance * creature->size);
		at.y = Locator::terrainSystem::value().GetHeightAt(glm::xz(at));
		ecs::archetypes::MobileObjectArchetype::Create(at, k_Objects.at(static_cast<size_t>(_objectType)).first, 0.0f, 1.0f);
	};
	constexpr float k_Quarter = std::numbers::pi_v<float> / 2.0f;
	ImGui::TextUnformatted("Put it");
	ImGui::SameLine();
	if (ImGui::SmallButton("in front"))
	{
		putAt(0.0f);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("front left"))
	{
		putAt(-k_Quarter / 2.0f);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("front right"))
	{
		putAt(k_Quarter / 2.0f);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("left"))
	{
		putAt(-k_Quarter);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("right"))
	{
		putAt(k_Quarter);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("behind"))
	{
		putAt(2.0f * k_Quarter);
	}

	// What to do with what it holds
	const auto told = [this](std::string_view what, bool started) {
		_lastHands = fmt::format("{}: {}", what, started ? "started" : "can't");
	};
	ImGui::BeginDisabled(!held.has_value());
	if (ImGui::Button("Put down"))
	{
		told("Put down", hands.PutDown(entity));
	}
	ImGui::SameLine();
	if (ImGui::Button("Toss away"))
	{
		told("Toss away", hands.Discard(entity));
	}
	ImGui::SameLine();
	if (ImGui::Button("Lob"))
	{
		told("Lob", hands.Lob(entity));
	}
	ImGui::SameLine();
	if (ImGui::Button("Eat it"))
	{
		told("Eat", hands.EatHeld(entity));
	}
	ImGui::SameLine();
	if (ImGui::Button("Drop"))
	{
		hands.Drop(entity);
	}
	ImGui::SetNextItemWidth(100.0f);
	if (ImGui::BeginCombo("##keep", k_KeepNames.at(static_cast<size_t>(_keepAction)).data()))
	{
		for (size_t i = 0; i < k_KeepNames.size(); ++i)
		{
			if (ImGui::Selectable(k_KeepNames.at(i).data(), static_cast<int>(i) == _keepAction))
			{
				_keepAction = static_cast<int>(i);
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Button("Look it over"))
	{
		told(k_KeepNames.at(static_cast<size_t>(_keepAction)),
		     hands.Keep(entity, creature_object_actions::k_FirstKeepAnimation + static_cast<size_t>(_keepAction)));
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Stop it"))
	{
		hands.Cancel(entity);
	}
	if (!_lastHands.empty())
	{
		ImGui::TextUnformatted(_lastHands.c_str());
	}
	ImGui::TextDisabled("Command it to pick up, throw, knock down or point with a right click");
}
