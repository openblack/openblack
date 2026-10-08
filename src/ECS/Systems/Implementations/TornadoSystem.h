/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/TornadoSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class TornadoSystem final: public TornadoSystemInterface
{
public:
	[[nodiscard]] std::vector<particles::TornadoCandidate> Candidates(glm::vec3 foot, float reach) const override;
	std::optional<glm::mat3> Carry(const std::shared_ptr<particles::CarriedObject>& carried) override;
	void CatchCreature(entt::entity creature) override;
	entt::entity TakeFromPile(entt::entity pile, uint32_t amount, float sizeShare) override;
	void ProcessTurn() override;
	void Update(float turnFraction) override;
	[[nodiscard]] size_t CarriedCount() const override;
	void Reset() override;

private:
	/// What a particle carried is put down where it went: the living dead on the land, anything else gone
	void LetGo(entt::entity object, const particles::CarriedObject& carried);
};

} // namespace openblack::ecs::systems
