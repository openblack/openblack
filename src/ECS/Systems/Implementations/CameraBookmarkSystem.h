/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <chrono>
#include <memory>

#include "ECS/Systems/CameraBookmarkSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#warning "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

class CameraBookmarkSystem final: public CameraBookmarkSystemInterface
{
public:
	bool Initialize() override;
	void Update(const std::chrono::microseconds& dt) const override;

	[[nodiscard]] const std::array<entt::entity, 8>& GetBookmarks() const override { return _bookmarks; }

	void SetBookmark(uint8_t index, const glm::vec3& position) const override;
	void ClearBookmark(uint8_t index) const override;

private:
	std::array<entt::entity, 8> _bookmarks;
};
} // namespace openblack::ecs::systems
