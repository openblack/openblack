/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GlowArchetype.h"

#include <entt/core/hashed_string.hpp>
#include <glm/ext/matrix_float3x3.hpp>

#include "3D/Light.h"
#include "3D/TempleInteriorInterface.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

std::array<entt::entity, 2> GlowArchetype::Create(const LightEmitter& emitter, TempleRoom room)
{
	auto texture = Locator::resources::value().GetTextures().Handle(entt::hashed_string("raw/ATMOS"));
	auto& registry = Locator::entitiesRegistry::value();
	const auto extent = glm::vec2 {1.0f / 8.0f, 1.0f / 8.0f};

	// Softer glow that uses a larger sprite
	auto glowEntity = registry.Create();
	{
		registry.Assign<ecs::components::TempleInteriorPart>(glowEntity, room);
		registry.Assign<Sprite>(glowEntity, texture->GetNativeHandle(), glm::vec2 {.75f, .25f}, extent,
		                        emitter.glow.backgroundColour);
		registry.Assign<ecs::components::Transform>(glowEntity, emitter.glow.position, glm::mat3(1.0f),
		                                            glm::vec3(emitter.glow.backgroundScale));
	}
	// A small bright shine at the center
	auto shineEntity = registry.Create();
	{
		registry.Assign<ecs::components::TempleInteriorPart>(shineEntity, room);
		registry.Assign<Sprite>(shineEntity, texture->GetNativeHandle(), glm::vec2 {.75f, .25f}, extent,
		                        emitter.glow.brightSpotColour);
		registry.Assign<ecs::components::Transform>(shineEntity, emitter.glow.position, glm::mat3(1.0f),
		                                            glm::vec3(emitter.glow.brightSpotScale));
	}
	return {glowEntity, shineEntity};
}
