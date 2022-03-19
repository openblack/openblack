/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#pragma once

#include <array>

#include <filesystem>
#include <memory>

#include "entt/fwd.hpp"
#include "glm/vec3.hpp"

namespace openblack::graphics
{
class Texture2D;
}

namespace openblack::ecs::systems
{
class CameraBookmarkSystem
{
public:
	static CameraBookmarkSystem& instance();
	bool Initialize(const graphics::Texture2D& texture);
	void Update(const std::chrono::microseconds& dt) const;

	const std::array<entt::entity, 8>& GetBookmarks() const { return _bookmarks; }

	void SetBookmark(uint8_t index, const glm::vec3& position) const;
	void ClearBookmark(uint8_t index) const;

private:
	CameraBookmarkSystem();

	std::array<entt::entity, 8> _bookmarks;
};
} // namespace openblack::ecs::systems
