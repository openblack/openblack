/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The Magic window's tab of running miracles: casting any miracle, putting down dispensers and one-shot bubbles,
// giving a miracle to the hand, and what the miracles, the dispensers and the creatures' spells are doing

#include <cstring>

#include <algorithm>
#include <string>

#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>

#include "Camera/Camera.h"
#include "Creature/CreatureSpells.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureSpells.h"
#include "ECS/Components/Player.h"
#include "ECS/Components/PrayerPower.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/GestureEventsInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "EngineConfig.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic.h"
#include "Magic/MagicTables.h"
#include "Magic/SpellRules.h"

using namespace openblack;
using namespace openblack::debug::gui;

namespace
{
constexpr std::array k_PhaseNames {"off", "waiting", "starting", "holding", "finishing"};
constexpr std::array k_HandResults {"",           "took a miracle",  "readied",          "cast",
                                    "cast, held", "not ready",       "can't cast there", "let go",
                                    "dropped",    "no circle drawn", "powered up"};
constexpr std::array k_CreatureSpellNames {"Freeze", "Small", "Big",    "Weak",       "Strong", "Fat", "Thin",    "Invisible",
                                           "Nice",   "Nasty", "Hungry", "Frightened", "Tired",  "Ill", "Thirsty", "Itchy"};

std::string MiracleName(MagicType type)
{
	if (!Locator::infoConstants::has_value())
	{
		return fmt::format("{}", static_cast<int>(type));
	}
	const auto& name = magic::GetMagicEffectInfo(Locator::infoConstants::value(), type).debugString;
	return std::string(name.data(), strnlen(name.data(), name.size()));
}

/// The nearest creature to a point, if any
std::optional<entt::entity> NearestCreature(glm::vec3 point)
{
	std::optional<entt::entity> nearest;
	float best = std::numeric_limits<float>::max();
	Locator::entitiesRegistry::value().Each<const ecs::components::Creature, const ecs::components::Transform>(
	    [&](entt::entity entity, const ecs::components::Creature&, const ecs::components::Transform& transform) {
		    const float distance = glm::distance(point, transform.position);
		    if (distance < best)
		    {
			    best = distance;
			    nearest = entity;
		    }
	    });
	return nearest;
}
} // namespace

void Magic::DrawCasting(ecs::systems::MagicSystemInterface& magic, glm::vec3 point) noexcept
{
	constexpr std::array k_States {"idle", "armed", "locked on"};
	const auto state = magic.GetHandCastState();
	ImGui::Text("Action %s, %s%s", k_States.at(static_cast<size_t>(state.state)), state.holding ? "held down" : "up",
	            state.pointValid ? "" : ", can't cast here");
	if (magic.GetHeldSeed().has_value())
	{
		ImGui::Text(
		    "Seed from %s, %s, power-up %d, charge %.0f%s", state.origin == magic::SeedOrigin::Worship ? "worship" : "a bubble",
		    state.ready ? "ready" : fmt::format("ready in {:.1f} s", state.readyIn).c_str(), state.powerUp, state.chantStore,
		    state.storedChants >= 0.0f ? fmt::format(", kept {:.0f}", state.storedChants).c_str() : "");
	}
	ImGui::Text("Hand moving %.0f, would throw at %.0f", glm::length(state.velocity), state.throwSpeed);
	if (state.pour.raise != 0.0f || state.pour.tilt != 0.0f)
	{
		ImGui::Text("Pouring: up %.1f, tipped %.0f degrees", state.pour.raise, glm::degrees(state.pour.tilt));
	}
	if (state.circleCentre.has_value())
	{
		ImGui::Text("Circle of %.0f at (%.0f, %.0f), %.1f s left", state.circleRadius, state.circleCentre->x,
		            state.circleCentre->z, state.circleSecondsLeft);
	}

	// The gestures, as if drawn, until the hand can draw them
	if (!Locator::gestureEvents::has_value())
	{
		return;
	}
	auto& gestures = Locator::gestureEvents::value();
	ImGui::SetNextItemWidth(120.0f);
	ImGui::SliderFloat("##radius", &_circleRadius, 5.0f, 200.0f, "radius %.0f");
	ImGui::SameLine();
	if (ImGui::Button("Draw circle here"))
	{
		gestures.Inject({.kind = ecs::systems::GestureEvent::Kind::Circle,
		                 .gesture = GestureType::Circle,
		                 .centre = point,
		                 .radius = _circleRadius,
		                 .powerUpLevel = -1});
	}
	ImGui::SameLine();
	if (ImGui::Button("Power up 1"))
	{
		gestures.Inject({.kind = ecs::systems::GestureEvent::Kind::PowerUp, .powerUpLevel = 0});
	}
	ImGui::SameLine();
	if (ImGui::Button("Power up 2"))
	{
		gestures.Inject({.kind = ecs::systems::GestureEvent::Kind::PowerUp, .powerUpLevel = 1});
	}
	ImGui::SameLine();
	if (ImGui::Button("Scribble"))
	{
		gestures.Inject({.kind = ecs::systems::GestureEvent::Kind::Scribble, .gesture = GestureType::Scribble});
	}
}

void Magic::DrawPrayer() noexcept
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<const ecs::components::Player, ecs::components::PrayerPower>(
	    [](entt::entity, const ecs::components::Player& player, ecs::components::PrayerPower& prayer) {
		    if (player.name != PlayerNames::PLAYER_ONE)
		    {
			    return;
		    }
		    ImGui::SetNextItemWidth(160.0f);
		    ImGui::InputFloat("Prayer power", &prayer.chants, 1000.0f, 10000.0f, "%.0f");
		    ImGui::SameLine();
		    ImGui::Checkbox("Infinite", &prayer.infinite);
	    });
}

void Magic::DrawMiracles() noexcept
{
	if (!Locator::magicSystem::has_value())
	{
		return;
	}
	auto& magic = Locator::magicSystem::value();
	const auto& info = Locator::infoConstants::value();
	auto& config = Locator::config::value();

	bool ignore = magic.IsIgnoringInfluence();
	if (ImGui::Checkbox("Cast outside influence", &ignore))
	{
		magic.SetIgnoreInfluence(ignore);
	}
	ImGui::SameLine();
	ImGui::Checkbox("Label dispensers", &config.showDispenserNames);

	if (ImGui::BeginCombo("Miracle", MiracleName(_miracle).c_str()))
	{
		for (size_t i = 1; i < magic::k_MagicTypeCount; ++i)
		{
			const auto type = static_cast<MagicType>(i);
			if (!magic::FindFirstSpellSeedForMagicType(info, type).has_value())
			{
				continue;
			}
			if (ImGui::Selectable(MiracleName(type).c_str(), type == _miracle))
			{
				_miracle = type;
			}
		}
		ImGui::EndCombo();
	}
	const auto point = SpawnPoint();
	const auto isCreatureSpell = magic::ClassOf(_miracle) == magic::SpellClass::Creature;
	if (ImGui::Button("Place dispenser"))
	{
		magic.CreateDispenser(point, _miracle, 0.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Place bubble"))
	{
		magic.CreateOneOffSeedFor(point + glm::vec3(0.0f, 3.0f, 0.0f), _miracle);
	}
	ImGui::SameLine();
	if (ImGui::Button("Give to hand"))
	{
		if (const auto seed = magic::FindFirstSpellSeedForMagicType(info, _miracle))
		{
			const auto step = magic::GetPowerUpGesture(magic::GetSpellSeedInfo(info, *seed), _miracle);
			magic.GiveSeedToHand(PlayerNames::PLAYER_ONE, *seed, step.level, 1.0f);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Summon from worship"))
	{
		if (const auto seed = magic::FindFirstSpellSeedForMagicType(info, _miracle))
		{
			const auto step = magic::GetPowerUpGesture(magic::GetSpellSeedInfo(info, *seed), _miracle);
			magic.SummonSeed(PlayerNames::PLAYER_ONE, *seed, step.level);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button(isCreatureSpell ? "Cast on nearest creature" : "Cast here"))
	{
		const auto seed = magic::FindFirstSpellSeedForMagicType(info, _miracle).value_or(SpellSeedType::None);
		float size = 1.0f;
		if (const auto* radius = magic::GetMagicInfoAs<GMagicRadiusSpellInfo>(info, _miracle))
		{
			size = radius->radiusForNormalCost;
		}
		const auto cast = magic::SeedCastData(info, _miracle, seed, 1.0f, size);
		// From a little above the point, thrown down onto it
		const particles::ProcessInfo process {.handPosition = point + glm::vec3(0.0f, 15.0f, 0.0f),
		                                      .cameraForward = Locator::camera::value().GetForward(),
		                                      .direction = glm::vec3(0.0f, -10.0f, 0.0f)};
		if (isCreatureSpell)
		{
			if (const auto creature = NearestCreature(point))
			{
				magic.CastOnObject(_miracle, PlayerNames::PLAYER_ONE, *creature, cast, process);
			}
		}
		else
		{
			magic.CastAtPoint(_miracle, PlayerNames::PLAYER_ONE, point, cast, process);
		}
	}
	ImGui::TextDisabled("At the %s: (%.0f, %.0f)", _spawnAt == SpawnAt::Hand ? "hand" : "camera focus", point.x, point.z);

	ImGui::SeparatorText("Hand");
	if (const auto held = magic.GetHeldSeed())
	{
		ImGui::Text("Holds seed %u", static_cast<uint32_t>(*held));
		ImGui::SameLine();
		if (ImGui::SmallButton("Drop"))
		{
			magic.DiscardHeldSeed();
		}
	}
	else
	{
		ImGui::TextUnformatted("Empty: tap a dispenser's bubble to take its miracle");
	}
	ImGui::TextDisabled("Last: %s", k_HandResults.at(static_cast<size_t>(magic.GetLastHandResult())));
	DrawCasting(magic, point);
	DrawPrayer();

	ImGui::SeparatorText("Running miracles");
	constexpr auto k_Flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
	if (ImGui::BeginTable("Spells", 6, k_Flags))
	{
		ImGui::TableSetupColumn("Miracle");
		ImGui::TableSetupColumn("Age");
		ImGui::TableSetupColumn("Prayer power");
		ImGui::TableSetupColumn("Strength");
		ImGui::TableSetupColumn("Upkeep");
		ImGui::TableSetupColumn("State");
		ImGui::TableHeadersRow();
		for (const auto& spell : magic.GetSpells())
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(MiracleName(spell.magicType).c_str());
			ImGui::TableNextColumn();
			ImGui::Text(spell.duration >= 0.0f ? "%.1f/%.0f s" : "%.1f s", spell.age, spell.duration);
			ImGui::TableNextColumn();
			ImGui::Text("%.0f/%.0f", spell.chants, spell.initialChants);
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", spell.strength);
			ImGui::TableNextColumn();
			ImGui::Text("%.1f", spell.upkeep);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(spell.closing ? "closing" : (spell.fromHand ? "in hand" : "running"));
		}
		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Dispensers"))
	{
		if (ImGui::BeginTable("Dispensers", 3, k_Flags))
		{
			for (const auto& dispenser : magic.GetDispensers())
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(MiracleName(dispenser.magicType).c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(dispenser.hasOrb ? "bubble ready" : (dispenser.active ? "making one" : "inactive"));
				ImGui::TableNextColumn();
				ImGui::Text("%u/%u turns", dispenser.tick, dispenser.period);
			}
			ImGui::EndTable();
		}
	}

	if (ImGui::CollapsingHeader("Creatures' spells", ImGuiTreeNodeFlags_DefaultOpen))
	{
		Locator::entitiesRegistry::value().Each<const ecs::components::CreatureSpells>(
		    [](entt::entity entity, const ecs::components::CreatureSpells& component) {
			    std::string line = fmt::format("Creature {}:", static_cast<uint32_t>(entity));
			    for (size_t i = 0; i < creature_spells::k_SpellCount; ++i)
			    {
				    const auto& slot = component.spells.slots.at(i);
				    if (slot.phase != creature_spells::Phase::Off)
				    {
					    line += fmt::format(" {} ({}, {} turns)", k_CreatureSpellNames.at(i),
					                        k_PhaseNames.at(static_cast<size_t>(slot.phase)), slot.turnsLeft);
				    }
			    }
			    ImGui::TextUnformatted(line.c_str());
			    if (component.freeze > 0.0f || component.fizz > 0.0f)
			    {
				    ImGui::TextDisabled("  frozen %.2f, fizzed %.2f", component.freeze, component.fizz);
			    }
		    });
	}
}
