/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VillagerClips.h"

#include <chrono>

#include <LNDFile.h>
#include <glm/common.hpp>
#include <glm/vec2.hpp>

#include "3D/L3DAnim.h"
#include "3D/LandIslandInterface.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;

std::optional<float> villager_clips::ClipMilliseconds(AnimId clip)
{
	if (!Locator::resources::has_value())
	{
		return std::nullopt;
	}
	auto& animations = Locator::resources::value().GetAnimations();
	const auto key = static_cast<entt::id_type>(clip);
	if (!animations.Contains(key))
	{
		return std::nullopt;
	}
	return static_cast<float>(animations.Handle(key)->GetDuration());
}

bool villager_clips::ClipPlayed(const components::LivingAction& action, AnimId clip, uint32_t times)
{
	constexpr auto k_TurnMilliseconds = std::chrono::milliseconds(systems::TimeSystemInterface::k_TurnDuration).count();
	return static_cast<float>(action.turnsSinceStateChange) * static_cast<float>(k_TurnMilliseconds) >=
	       ClipMilliseconds(clip).value_or(0.0f) * static_cast<float>(times);
}

bool villager_clips::IsOnWater(glm::vec3 point)
{
	if (!Locator::terrainSystem::has_value() || point.x < 0.0f || point.z < 0.0f)
	{
		return false;
	}
	const auto* cell = Locator::terrainSystem::value().FindCell(glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / 10.0f)));
	return cell != nullptr && cell->properties.hasWater != 0;
}
