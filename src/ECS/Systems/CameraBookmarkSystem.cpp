/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#include "CameraBookmarkSystem.h"

#include "3D/LandIsland.h"
#include "ECS/Archetypes/CameraBookmarkArchetype.h"
#include "ECS/Components/CameraBookmark.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

CameraBookmarkSystem& CameraBookmarkSystem::Instance()
{
	static std::unique_ptr<CameraBookmarkSystem> instance = nullptr;
	if (instance == nullptr)
	{
		instance.reset(new CameraBookmarkSystem());
	}
	return *instance;
}

CameraBookmarkSystem::CameraBookmarkSystem() = default;

bool CameraBookmarkSystem::Initialize()
{
	_bookmarks = archetypes::CameraBookmarkArchetype::CreateAll();
	return true;
}

void CameraBookmarkSystem::Update(const std::chrono::microseconds& dt) const
{
	Game::Instance()->GetEntityRegistry().Each<CameraBookmark, Transform>(
	    [&dt](CameraBookmark& bookmark, Transform& transform) {
		    std::chrono::duration<float> seconds = dt;
		    auto t = bookmark.animationTime * 5.0f;
		    transform.scale = glm::vec3(glm::sin(t) * 0.5f + 0.5f, glm::cos(t) * 0.5f + 0.5f, 1.0f);
		    bookmark.animationTime += seconds.count();
	    });
}

void CameraBookmarkSystem::SetBookmark(uint8_t index, const glm::vec3& position) const
{
	if (index < _bookmarks.size())
	{
		auto& registry = Game::Instance()->GetEntityRegistry();
		auto entity = _bookmarks.at(index);
		registry.AssignOrReplace<Transform>(entity, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
		auto& bookmark = registry.Get<CameraBookmark>(entity);
		bookmark.animationTime = 0.0f;
	}
	else
	{
		assert(false);
	}
}

void CameraBookmarkSystem::ClearBookmark(uint8_t index) const
{
	if (index < _bookmarks.size())
	{
		Game::Instance()->GetEntityRegistry().Remove<Transform>(_bookmarks.at(index));
	}
	else
	{
		assert(false);
	}
}
