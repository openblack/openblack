/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GripLandscapeEffect.h"

#include <numbers>
#include <vector>

#include <entt/core/hashed_string.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>

#include "Common/RandomNumberManager.h"
#include "ECS/Components/GripLandscapeParticle.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;

namespace
{
// SF_GripLandscape: CreateRuleAnAtom0 puts the centre above the ground, CreateRuleSphere0 the sprites around it
constexpr auto k_CentreHeight = 0.935841f;
constexpr auto k_SpriteCount = 8;
constexpr auto k_SphereRadius = std::numbers::sqrt2_v<float>;
// UR_ChangeScale3: the centre grows from nothing, and the sprites, its children, with it
constexpr auto k_FinalScale = 1.43009f;
constexpr auto k_GrowTime = 1.01f;
// AR_FadeAlpha0
constexpr auto k_StartAlpha = 78.0f / 255.0f;
constexpr auto k_EndAlpha = 2.0f / 255.0f;
constexpr auto k_FadeTime = 0.865714f;
// RemoveRuleOldAgeOnly0 removes the centre, and its sprites with it
constexpr auto k_DieAge = 2.65752f;
// ParticleSpriteCreator0: the first 32 frames of the 8 by 8 sheet are dust, played looping from a random frame
constexpr auto k_FrameRate = 20.7611f;
constexpr auto k_FrameCount = 32;
constexpr auto k_SheetColumns = 8;
constexpr auto k_Colour = glm::vec3(255.0f, 182.0f, 198.0f) / 255.0f;
constexpr entt::hashed_string k_SpriteSheetId = entt::hashed_string("raw/S_SpriteSheet3a");

glm::vec2 FrameOrigin(float frame)
{
	const auto index = static_cast<int>(frame) % k_FrameCount;
	return glm::vec2(index % k_SheetColumns, index / k_SheetColumns) / static_cast<float>(k_SheetColumns);
}

/// PSysManager::PSysRandR3: a point uniformly within the unit sphere
glm::vec3 RandomInUnitSphere()
{
	auto& rng = Locator::rng::value();
	glm::vec3 point;
	do
	{
		point = glm::vec3(rng.NextValue(-1.0f, 1.0f), rng.NextValue(-1.0f, 1.0f), rng.NextValue(-1.0f, 1.0f));
	} while (glm::dot(point, point) > 1.0f);
	return point;
}
} // namespace

void GripLandscapeEffect::Spawn(glm::vec3 groundPosition)
{
	auto texture = Locator::resources::value().GetTextures().Handle(k_SpriteSheetId);
	if (!texture)
	{
		return;
	}

	auto& registry = Locator::entitiesRegistry::value();
	const auto centre = groundPosition + glm::vec3(0.0f, k_CentreHeight, 0.0f);
	for (int i = 0; i < k_SpriteCount; ++i)
	{
		const auto frame = static_cast<float>(Locator::rng::value().NextValue(0, k_FrameCount - 1));
		const auto entity = registry.Create();
		registry.Assign<GripLandscapeParticle>(entity, centre, RandomInUnitSphere() * k_SphereRadius, 0.0f, frame);
		registry.Assign<Sprite>(entity, texture->GetNativeHandle(), FrameOrigin(frame),
		                        glm::vec2(1.0f / static_cast<float>(k_SheetColumns)), glm::vec4(k_Colour, k_StartAlpha), false);
		registry.Assign<Transform>(entity, centre, glm::mat3(1.0f), glm::vec3(0.0f));
	}
}

void GripLandscapeEffect::Update(float deltaSeconds)
{
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> dead;
	registry.Each<GripLandscapeParticle, Sprite, Transform>(
	    [&dead, deltaSeconds](entt::entity entity, GripLandscapeParticle& particle, Sprite& sprite, Transform& transform) {
		    particle.age += deltaSeconds;
		    if (particle.age >= k_DieAge)
		    {
			    dead.push_back(entity);
			    return;
		    }
		    particle.frame = glm::mod(particle.frame + (k_FrameRate * deltaSeconds), static_cast<float>(k_FrameCount));

		    const auto scale = k_FinalScale * glm::min(particle.age / k_GrowTime, 1.0f);
		    const auto alpha = glm::mix(k_StartAlpha, k_EndAlpha, glm::min(particle.age / k_FadeTime, 1.0f));

		    // A sprite of unit scale spans two units, like LH3DSprite's quad of half its size either side
		    transform.position = particle.centre + particle.offset * scale;
		    transform.scale = glm::vec3(scale);
		    sprite.uvMin = FrameOrigin(particle.frame);
		    sprite.tint.a = alpha;
	    });
	for (const auto entity : dead)
	{
		registry.Destroy(entity);
	}
}
