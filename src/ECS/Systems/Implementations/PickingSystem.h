/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <vector>

#include "ECS/Systems/PickingSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

class PickingSystem final: public PickingSystemInterface
{
public:
	[[nodiscard]] std::optional<glm::vec3> LandAlong(glm::vec3 from, glm::vec3 to) const override;
	[[nodiscard]] std::optional<glm::vec3> LandOrSeaAlong(glm::vec3 from, glm::vec3 to, glm::vec3 camera) const override;
	[[nodiscard]] std::optional<glm::vec3> LandUnderPixel(glm::vec3 camera, glm::vec3 nearPoint, bool withSea) const override;

	void PickUnderCursor(const Frame& frame) override;
	[[nodiscard]] const Pick& GetPick() const override { return _pick; }

	[[nodiscard]] std::optional<screen_pick::MeshHit> FeelModel(entt::entity object, glm::vec3 origin,
	                                                            glm::vec3 direction) const override;

private:
	Pick _pick;
	/// Room the pick reuses from frame to frame
	std::vector<screen_pick::Candidate> _candidates;
	std::vector<entt::entity> _candidateEntities;
	std::vector<glm::mat4> _candidateModels;
	std::vector<screen_pick::ClipCorner> _corners;
	std::vector<glm::vec3> _placed;
	std::vector<glm::vec2> _uvs;
	screen_pick::PickScratch _scratch;
};

} // namespace openblack::ecs::systems
