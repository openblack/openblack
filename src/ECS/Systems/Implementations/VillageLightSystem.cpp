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

#include <cmath>

#include <algorithm>

#include "3D/DayNightClock.h"
#include "3D/LandLightTable.h"
#include "3D/SkyInterface.h"
#include "3D/VillageLights.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Transform.h"
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
	auto& registry = Locator::entitiesRegistry::value();
	const bool dark = IsDark();
	if (dark)
	{
		// The game flickers them from the C runtime's numbers, the newest light first as the registry walks them
		auto& random = Locator::gameRandom::value();
		registry.Each<VillageLight>([&random, gameTime](VillageLight& light) {
			const auto step = village_lights::AdvanceFlicker(light.flickerTimer, gameTime.count());
			light.flickerTimer = step.timer;
			if (!step.flickers)
			{
				return;
			}
			const auto x = random.CrtRandom(-village_lights::k_FlickerReach, village_lights::k_FlickerReach);
			const auto z = random.CrtRandom(-village_lights::k_FlickerReach, village_lights::k_FlickerReach);
			light.flicker = {x, z};
			light.glowSize = std::max(random.CrtRandom(-village_lights::k_GlowFlicker, village_lights::k_GlowFlicker) +
			                              village_lights::k_GlowSize,
			                          village_lights::k_SmallestSprite);
		});
	}

	// The flames and glows show at half the lights' brightness, in whole steps, and only while it is dark; their loop
	// only moves on while they show
	float alpha = 0.0f;
	if (dark && Locator::skySystem::has_value())
	{
		alpha = std::trunc(village_lights::Intensity(Locator::skySystem::value().GetClock().GetScriptTime()) * 0.5f);
	}
	auto& flames = registry.Context().villageLightFlames;
	int32_t step = 0;
	if (alpha != 0.0f)
	{
		const auto advanced = village_lights::AdvanceFlames(flames.clock, static_cast<int32_t>(gameTime.count()));
		flames.clock = advanced.clock;
		step = advanced.step;
	}
	registry.Each<const VillageLightSprite, Sprite, Transform>(
	    [&registry, &flames, alpha, step](const VillageLightSprite& lightSprite, Sprite& sprite, Transform& transform) {
		    sprite.tint = glm::vec4(village_lights::k_SpriteColour, alpha / 255.0f);
		    if (alpha == 0.0f)
		    {
			    return;
		    }
		    if (lightSprite.index < village_lights::k_Flames)
		    {
			    const auto cell = village_lights::FlameCell(step, lightSprite.index, flames.starts.at(lightSprite.index));
			    sprite.uvMin = village_lights::SpriteCellUv(cell);
		    }
		    else
		    {
			    const auto glowSize = registry.Get<const VillageLight>(lightSprite.light).glowSize;
			    transform.scale = glm::vec3(glowSize, glowSize, 1.0f);
		    }
	    });
}
