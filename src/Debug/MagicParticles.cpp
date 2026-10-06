/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The Magic window's particles tab: spawn any particle type or file at the hand or where the camera looks, and watch
// the running effects

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <imgui.h>

#include "3D/LandIslandInterface.h"
#include "Camera/Camera.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "Locator.h"
#include "Magic.h"
#include "Particles/ParticleTypes.h"

using namespace openblack;
using namespace openblack::debug::gui;

namespace
{
constexpr int k_PlayerCount = 8;
constexpr float k_MaxMagnitude = 10.0f;
constexpr float k_MaxHeight = 50.0f;
constexpr float k_MaxCloseAfter = 30.0f;
constexpr int k_MaxTargets = 10;

std::string TypeLabel(ParticleType type)
{
	const auto file = particles::ParticleTypeFile(type);
	return fmt::format("{} {} ({})", static_cast<uint32_t>(type), particles::ParticleTypeName(type),
	                   file.empty() ? "no file" : file);
}
} // namespace

glm::vec3 Magic::SpawnPoint() const noexcept
{
	glm::vec3 point(0.0f);
	if (_spawnAt == SpawnAt::Hand && Locator::handSystem::has_value())
	{
		for (const auto& hand : Locator::handSystem::value().GetPlayerHandPositions())
		{
			if (hand.has_value())
			{
				return *hand + glm::vec3(0.0f, _spawnHeight, 0.0f);
			}
		}
	}
	if (Locator::camera::has_value())
	{
		point = Locator::camera::value().GetFocus();
	}
	if (Locator::terrainSystem::has_value())
	{
		point.y = Locator::terrainSystem::value().GetHeightAt({point.x, point.z});
	}
	return point + glm::vec3(0.0f, _spawnHeight, 0.0f);
}

void Magic::DrawParticles() noexcept
{
	if (!Locator::particleSystem::has_value())
	{
		ImGui::TextUnformatted("No particle system");
		return;
	}
	auto& particles = Locator::particleSystem::value();
	if (_particleFiles.empty())
	{
		_particleFiles = particles.GetFileNames();
	}

	ImGui::Checkbox("By file", &_byFile);
	if (_byFile)
	{
		const auto preview = _particleFiles.empty() ? std::string("no files")
		                                            : _particleFiles.at(static_cast<size_t>(std::clamp(
		                                                  _particleFile, 0, static_cast<int>(_particleFiles.size()) - 1)));
		if (ImGui::BeginCombo("File", preview.c_str()))
		{
			for (int i = 0; i < static_cast<int>(_particleFiles.size()); ++i)
			{
				if (ImGui::Selectable(_particleFiles.at(static_cast<size_t>(i)).c_str(), i == _particleFile))
				{
					_particleFile = i;
				}
			}
			ImGui::EndCombo();
		}
	}
	else if (ImGui::BeginCombo("Type", TypeLabel(_particleType).c_str()))
	{
		for (size_t i = 0; i < particles::k_ParticleTypeCount; ++i)
		{
			const auto type = static_cast<ParticleType>(i);
			ImGui::BeginDisabled(particles::ParticleTypeFile(type).empty());
			if (ImGui::Selectable(TypeLabel(type).c_str(), type == _particleType))
			{
				_particleType = type;
			}
			ImGui::EndDisabled();
		}
		ImGui::EndCombo();
	}

	ImGui::RadioButton("At what the camera looks at", reinterpret_cast<int*>(&_spawnAt),
	                   static_cast<int>(SpawnAt::CameraFocus));
	ImGui::SameLine();
	ImGui::RadioButton("At the hand", reinterpret_cast<int*>(&_spawnAt), static_cast<int>(SpawnAt::Hand));
	ImGui::SliderFloat("Height", &_spawnHeight, 0.0f, k_MaxHeight, "%.1f");
	ImGui::SliderFloat("Magnitude", &_magnitude, 0.0f, k_MaxMagnitude, "%.2f");
	ImGui::SliderInt("Player", &_player, 0, k_PlayerCount - 1);
	ImGui::SliderFloat("Close after", &_closeAfter, 0.0f, k_MaxCloseAfter, _closeAfter > 0.0f ? "%.1f s" : "never");
	ImGui::Checkbox("Synced random numbers", &_synced);
	if (ImGui::BeginCombo("Drawn", particles::draw::k_DrawPathNames.at(static_cast<size_t>(_drawPath)).data()))
	{
		for (size_t i = 0; i < particles::draw::k_DrawPathNames.size(); ++i)
		{
			const auto path = static_cast<particles::draw::DrawPath>(i);
			if (ImGui::Selectable(particles::draw::k_DrawPathNames.at(i).data(), path == _drawPath))
			{
				_drawPath = path;
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Sorted: each sprite, model, mist and ribbon in its own place among what blends.\n"
		                  "Queued: the whole effect at its origin, drawn in its own order, as some spot visuals are.\n"
		                  "Immediate: drawn just after the hand, as the miracle in the hand is.");
	}
	ImGui::SliderInt("Targets", &_targets, 0, k_MaxTargets, _targets > 0 ? "%d nearest" : "none");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The creatures and villagers nearest the effect are given to it to act on, as the heal "
		                  "miracle's chakra is given the people it heals.");
	}

	if (ImGui::Button("Spawn"))
	{
		const auto point = SpawnPoint();
		const auto id = _byFile && !_particleFiles.empty()
		                    ? particles.Start(_particleFiles.at(static_cast<size_t>(_particleFile)), point, _magnitude, _synced)
		                    : particles.Start(_particleType, point, _magnitude, _synced);
		particles.SetPlayer(id, _player);
		particles.SetDrawPath(id, _drawPath);
		GiveNearestTargets(id, point);
		if (id != ecs::systems::ParticleSystemInterface::k_NoEffect && _closeAfter > 0.0f)
		{
			_timedEffects.push_back({id, _closeAfter});
		}
	}
	ImGui::SameLine();
	bool paused = particles.IsPaused();
	if (ImGui::Checkbox("Pause", &paused))
	{
		particles.SetPaused(paused);
	}
	ImGui::SameLine();
	if (ImGui::Button("Close all"))
	{
		for (const auto& effect : particles.GetEffects())
		{
			particles.CloseDown(effect.id);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete all"))
	{
		for (const auto& effect : particles.GetEffects())
		{
			particles.Delete(effect.id);
		}
	}
	ImGui::Separator();
	DrawParticleStats();
	DrawRunningEffects();
}

void Magic::DrawParticleStats() const noexcept
{
	const auto stats = Locator::particleSystem::value().GetDrawStats();
	ImGui::Text("Drawn: %zu sprites, %zu ribbons, %zu models, %zu mists, %zu light maps, from %zu effects", stats.sprites,
	            stats.chains, stats.meshes, stats.mists, stats.lightStamps, stats.effects);
}

void Magic::GiveNearestTargets(uint32_t effect, const glm::vec3& origin) const noexcept
{
	if (_targets <= 0 || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<std::pair<float, entt::entity>> nearest;
	const auto add = [&nearest, &origin](entt::entity entity, const ecs::components::Transform& transform) {
		nearest.emplace_back(glm::distance(transform.position, origin), entity);
	};
	registry.Each<const ecs::components::Creature, const ecs::components::Transform>(
	    [&add](entt::entity entity, const ecs::components::Creature&, const ecs::components::Transform& transform) {
		    add(entity, transform);
	    });
	registry.Each<const ecs::components::Villager, const ecs::components::Transform>(
	    [&add](entt::entity entity, const ecs::components::Villager&, const ecs::components::Transform& transform) {
		    add(entity, transform);
	    });
	std::ranges::sort(nearest, {}, &std::pair<float, entt::entity>::first);
	const auto count = std::min(nearest.size(), static_cast<size_t>(_targets));
	// The rules take the last given first: the nearest goes last
	for (size_t i = count; i > 0; --i)
	{
		Locator::particleSystem::value().AddTarget(effect, nearest.at(i - 1).second);
	}
}

void Magic::DrawRunningEffects() noexcept
{
	auto& particles = Locator::particleSystem::value();
	const auto effects = particles.GetEffects();
	ImGui::Text("%zu running", effects.size());
	if (!ImGui::BeginTable("ParticleEffects", 7, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
	{
		return;
	}
	ImGui::TableSetupColumn("File");
	ImGui::TableSetupColumn("Drawn");
	ImGui::TableSetupColumn("Age");
	ImGui::TableSetupColumn("Atoms");
	ImGui::TableSetupColumn("Groups");
	ImGui::TableSetupColumn("State");
	ImGui::TableSetupColumn("");
	ImGui::TableHeadersRow();
	for (const auto& effect : effects)
	{
		ImGui::PushID(static_cast<int>(effect.id));
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(effect.file.c_str());
		if (!effect.unportedClasses.empty() && ImGui::IsItemHovered())
		{
			std::string list = "Not run yet:";
			for (const auto& name : effect.unportedClasses)
			{
				list += "\n  " + name;
			}
			ImGui::SetTooltip("%s", list.c_str());
		}
		ImGui::TableNextColumn();
		// Click to draw it the next way
		if (ImGui::SmallButton(particles::draw::k_DrawPathNames.at(static_cast<size_t>(effect.path)).data()))
		{
			const auto next = (static_cast<size_t>(effect.path) + 1) % particles::draw::k_DrawPathNames.size();
			particles.SetDrawPath(effect.id, static_cast<particles::draw::DrawPath>(next));
		}
		ImGui::TableNextColumn();
		ImGui::Text("%.1f s", effect.age);
		ImGui::TableNextColumn();
		if (effect.targets > 0)
		{
			ImGui::Text("%zu (%zu to act on)", effect.atoms, effect.targets);
		}
		else
		{
			ImGui::Text("%zu", effect.atoms);
		}
		ImGui::TableNextColumn();
		ImGui::Text("%zu", effect.collections);
		ImGui::TableNextColumn();
		if (effect.closing)
		{
			ImGui::TextUnformatted("closing");
		}
		else if (effect.secondsLeft.has_value())
		{
			ImGui::Text("%.1f s left", *effect.secondsLeft);
		}
		else
		{
			ImGui::TextUnformatted(effect.ownedBySpell ? "miracle" : "running");
		}
		ImGui::TableNextColumn();
		if (ImGui::SmallButton("Close"))
		{
			particles.CloseDown(effect.id);
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Delete"))
		{
			particles.Delete(effect.id);
		}
		ImGui::PopID();
	}
	ImGui::EndTable();
}

void Magic::UpdateTimedEffects(float seconds) noexcept
{
	if (!Locator::particleSystem::has_value() || _timedEffects.empty())
	{
		return;
	}
	auto& particles = Locator::particleSystem::value();
	std::erase_if(_timedEffects, [&](TimedEffect& timed) {
		if (!particles.IsRunning(timed.id))
		{
			return true;
		}
		timed.secondsLeft -= seconds;
		if (timed.secondsLeft > 0.0f)
		{
			return false;
		}
		particles.CloseDown(timed.id);
		return true;
	});
}
