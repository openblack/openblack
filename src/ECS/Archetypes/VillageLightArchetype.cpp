/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VillageLightArchetype.h"

#include <algorithm>
#include <array>

#include <glm/vec3.hpp>

#include "3D/VillageLights.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity VillageLightArchetype::Create(const glm::vec3& position, VillageLight::Kind kind)
{
	auto& registry = Locator::entitiesRegistry::value();
	// Everything about it is drawn from the C runtime's numbers rather than the game's
	auto& random = Locator::gameRandom::value();

	const auto light = registry.Create();
	registry.Assign<Transform>(light, position, glm::mat3(1.0f), glm::vec3(1.0f));
	registry.Assign<VillageLight>(light, VillageLight {
	                                         .kind = kind,
	                                         .flickerTimer = random.CrtRandom(0.0f, village_lights::k_FlickerMilliseconds),
	                                         .flicker = glm::vec2(0.0f),
	                                         .glowSize = village_lights::k_GlowSize,
	                                     });

	auto& textures = Locator::resources::value().GetTextures();
	const bool hasTextures =
	    std::ranges::all_of(std::array {village_lights::k_FlameTextureId, village_lights::k_FlameAlphaTextureId,
	                                    village_lights::k_GlowTextureId, village_lights::k_GlowAlphaTextureId},
	                        [&textures](const entt::hashed_string& id) { return textures.Contains(id.value()); });
	const auto texture = [&textures](entt::id_type id) { return textures.Handle(id)->GetNativeHandle(); };
	const float height =
	    kind == VillageLight::Kind::Town ? village_lights::k_TownSpriteHeight : village_lights::k_CountrySpriteHeight;
	auto& starts = registry.Context().villageLightFlames.starts;
	for (size_t i = 0; i < village_lights::k_Sprites; ++i)
	{
		const bool flame = i < village_lights::k_Flames;
		float size = village_lights::k_GlowSize;
		if (flame)
		{
			size = std::max(random.CrtRandom(-village_lights::k_FlameSizeVariation, village_lights::k_FlameSizeVariation) +
			                    village_lights::k_FlameSize,
			                village_lights::k_SmallestSprite);
		}
		// Each new light starts every light's sprites somewhere new in the flames' loop
		starts.at(i) = static_cast<int32_t>(random.CrtRandom(0.0f, static_cast<float>(village_lights::k_FlameCells)));

		if (!hasTextures)
		{
			continue;
		}
		const auto sprite = registry.Create();
		registry.Assign<VillageLightSprite>(sprite, light, static_cast<uint8_t>(i));
		const auto cell = village_lights::SpriteCellUv(flame ? 0 : village_lights::k_GlowCell);
		registry.Assign<Sprite>(
		    sprite, texture(flame ? village_lights::k_FlameTextureId.value() : village_lights::k_GlowTextureId.value()), cell,
		    glm::vec2(village_lights::k_SpriteCell), glm::vec4(village_lights::k_SpriteColour, 0.0f), true, true,
		    texture(flame ? village_lights::k_FlameAlphaTextureId.value() : village_lights::k_GlowAlphaTextureId.value()));
		registry.Assign<Transform>(sprite, position + glm::vec3(0.0f, height, 0.0f), glm::mat3(1.0f),
		                           glm::vec3(size, size, 1.0f));
	}
	return light;
}
