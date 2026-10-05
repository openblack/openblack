/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "VillageLightSystem.h"

#include <algorithm>

#include "3D/LandLightTable.h"
#include "3D/SkyInterface.h"
#include "3D/VillageLights.h"
#include "Common/GameRandom.h"
#include "ECS/Components/VillageLight.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

namespace
{
/// Whether the land's colour of the frame is dark enough for the lights
bool IsDark()
{
	const auto& palettes = Locator::resources::value().GetLandLightPalettes();
	if (!palettes.Contains(LandLightPalette::k_Id.value()) || !Locator::skySystem::has_value())
	{
		return false;
	}
	const auto skyType = Locator::skySystem::value().GetCurrentSkyType();
	const auto alignment = Locator::alignmentSystem::has_value() ? Locator::alignmentSystem::value().GetSkyAlignment() : 0.0f;
	return village_lights::IsDark(
	    LandLightTable::GetLandColour(*palettes.Handle(LandLightPalette::k_Id.value()), skyType, alignment));
}
} // namespace

void VillageLightSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	if (!IsDark())
	{
		return;
	}
	// The game flickers them from the C runtime's numbers, the newest light first as the registry walks them
	auto& random = Locator::gameRandom::value();
	Locator::entitiesRegistry::value().Each<VillageLight>([&random, gameTime](VillageLight& light) {
		const auto step = village_lights::AdvanceFlicker(light.flickerTimer, gameTime.count());
		light.flickerTimer = step.timer;
		if (!step.flickers)
		{
			return;
		}
		const auto x = random.CrtRandom(-village_lights::k_FlickerReach, village_lights::k_FlickerReach);
		const auto z = random.CrtRandom(-village_lights::k_FlickerReach, village_lights::k_FlickerReach);
		light.flicker = {x, z};
		constexpr float k_SmallestGlow = 1e-4f;
		light.glowSize = std::max(random.CrtRandom(-village_lights::k_GlowFlicker, village_lights::k_GlowFlicker) +
		                              village_lights::k_GlowSize,
		                          k_SmallestGlow);
	});
}
