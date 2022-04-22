/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#include "CameraBookmarkArchetype.h"

#include "ECS/Components/CameraBookmark.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

std::array<entt::entity, 8> CameraBookmarkArchetype::CreateAll()
{
	auto& registry = Game::instance()->GetEntityRegistry();
	auto texture = Locator::resources::ref().GetTextures().Handle(entt::hashed_string("raw/misc0a"));
	if (!texture)
	{
		throw std::runtime_error("Failed to get Camera Bookmark sprite: misc0a");
	}

	auto result = std::array<entt::entity, 8>();

	registry.Create(result.begin(), result.end());
	glm::vec2 extent = glm::vec2 {1.0f / 8.0f, 1.0f / 8.0f};
	glm::vec4 tint = glm::vec4 {1.0f, 1.0f, 0.0f, 1.0f};
	for (size_t i = 0; i < result.size(); ++i)
	{
		float u = static_cast<float>(i) / static_cast<float>(result.size());
		registry.Assign<Sprite>(result[i], texture->GetNativeHandle(), glm::vec2 {u, 3.0f / 8.0f}, extent, tint);
		registry.Assign<CameraBookmark>(result[i], static_cast<uint8_t>(i + 1), 0.0f);
	}

	return result;
}
