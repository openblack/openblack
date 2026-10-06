/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Components/HandOnCreature.h"
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
	bool Stroke(entt::entity creature, creature_feedback::BodyPart part) override;
	bool Slap(entt::entity creature, float heightShare, bool gentle, bool sweepsRight) override;
	[[nodiscard]] std::optional<entt::entity> GetCreature() const override;
	[[nodiscard]] bool IsHeldByCommand() const override;
	[[nodiscard]] std::optional<entt::entity> CreatureAlong(const glm::vec3& rayOrigin,
	                                                        const glm::vec3& rayDirection) const override;
	[[nodiscard]] float GetFeedbackSum() const override;
	[[nodiscard]] float GetLastFeedbackSum() const override;

private:
	/// Takes hold of a creature by a command, keeping the session if already held to it
	components::HandOnCreature* HoldByCommand(entt::entity creature);
};

} // namespace openblack::ecs::systems
