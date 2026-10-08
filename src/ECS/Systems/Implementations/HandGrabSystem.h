/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/HandGrabSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

#include "Hand/HandGrabRules.h"

namespace openblack::ecs::components
{
struct HandGrab;
}

namespace openblack::ecs::systems
{

class HandGrabSystem final: public HandGrabSystemInterface
{
public:
	bool Press(glm::vec3 rayOrigin, glm::vec3 rayDirection, uint32_t nowMs, uint32_t turn) override;
	std::optional<entt::entity> Release(uint32_t nowMs, uint32_t turn) override;
	glm::vec3 UpdateFrame(const Frame& frame) override;
	void ProcessTurn() override;
	void ForceDrop() override;
	void Reset() override;

	[[nodiscard]] std::optional<entt::entity> GetHeld() const override;
	[[nodiscard]] bool IsBusy() const override;
	[[nodiscard]] std::optional<HeldPose> GetHeldPose() const override;
	[[nodiscard]] float GetCursorRaise() const override;

private:
	/// The player's hand's grab, made on the hand when first needed; none without a hand
	[[nodiscard]] components::HandGrab* Grab();
	[[nodiscard]] const components::HandGrab* Grab() const;
	/// The nearest thing a line meets, drawn where it is
	[[nodiscard]] std::optional<entt::entity> ObjectAlong(glm::vec3 origin, glm::vec3 direction) const;
	/// How the hand treats a thing
	[[nodiscard]] hand_grab::GrabKind KindOf(entt::entity object) const;
	/// How a thing hangs in the hand
	[[nodiscard]] hand_grab::HoldFacts HoldOfObject(entt::entity object) const;
	/// The hand takes a thing it waited for or pulled free
	void Take(components::HandGrab& grab, entt::entity object);
	/// The hand lets go of what it holds at a velocity, put down or thrown; made to, it starts from where it is held
	/// and is never planted again
	void LetGo(components::HandGrab& grab, glm::vec3 velocity, bool forced);
	/// The hand holds nothing
	static void Empty(components::HandGrab& grab);

	/// The hand's size last frame, which sets how far what it takes hangs
	float _handSize {1.0f};
	/// Where the cursor's point on the land was last frame
	std::optional<glm::vec3> _cursorGround;
};

} // namespace openblack::ecs::systems
