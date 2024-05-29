/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraBookmarkArchetype.h"

#include "ECS/Components/CameraBookmark.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Registry.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

std::array<entt::entity, 8> CameraBookmarkArchetype::CreateAll()
{
	auto& registry = Locator::entitiesRegistry::value();
	auto texture = Locator::resources::value().GetTextures().Handle(entt::hashed_string("raw/misc0a"));
	if (!texture)
	{
		throw std::runtime_error("Failed to get Camera Bookmark sprite: misc0a");
	}

	auto result = std::array<entt::entity, 8>();

	registry.Create(result.begin(), result.end());
	const glm::vec2 extent = glm::vec2 {1.0f / 8.0f, 1.0f / 8.0f};
	const glm::vec4 tint = glm::vec4 {1.0f, 1.0f, 0.0f, 1.0f};

	// TODO (#749) use std::views::enumerate
	for (uint8_t i = 0; auto entity : result)
	{
		const float u = static_cast<float>(i) / static_cast<float>(result.size());
		registry.Assign<Sprite>(entity, texture->GetNativeHandle(), glm::vec2 {u, 3.0f / 8.0f}, extent, tint);
		++i;
		registry.Assign<CameraBookmark>(entity, i, 0.0f);
	}

	return result;
}
