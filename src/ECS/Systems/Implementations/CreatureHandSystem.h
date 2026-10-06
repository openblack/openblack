/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/CreatureHandSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CreatureHandSystem final: public CreatureHandSystemInterface
{
public:
	bool Grab(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) override;
	std::optional<HandPose> Update(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec2 cursor,
	                               float seconds) override;
	void Release() override;
	[[nodiscard]] std::optional<entt::entity> GetCreature() const override;
	[[nodiscard]] float GetFeedbackSum() const override;
};

} // namespace openblack::ecs::systems
