/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <unordered_map>

#include <entt/entity/fwd.hpp>

#include "3D/VillageLights.h"
#include "Components/Footpath.h"
#include "Components/MapScriptGlobals.h"
#include "Components/Stream.h"
#include "Components/Town.h"
#include "Components/VillageLight.h"

namespace openblack::ecs
{
struct RegistryContext
{
	std::unordered_map<components::Footpath::Id, entt::entity> footpaths;
	std::unordered_map<components::Stream::Id, entt::entity> streams;
	std::unordered_map<uint32_t, entt::entity> towns;
	components::VillageLightFlames villageLightFlames {.clock = 0, .starts = village_lights::k_FlameStarts};
	components::MapScriptGlobals mapScriptGlobals;
};
} // namespace openblack::ecs
