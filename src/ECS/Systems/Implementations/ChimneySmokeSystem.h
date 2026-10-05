/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <glm/vec3.hpp>

#include "3D/ChimneySmoke.h"
#include "ECS/Systems/ChimneySmokeSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class ChimneySmokeSystem final: public ChimneySmokeSystemInterface
{
public:
	void Attach(entt::entity abode, const graphics::L3DMesh& mesh, const components::Transform& transform,
	            bool workshop) override;
	void ProcessTurn() override;
	void Update(std::chrono::duration<float, std::milli> gameTime) override;

private:
	/// The hand's velocity, easing towards its motion each turn, and where it was at the last turn
	glm::vec3 _handVelocity {0.0f};
	std::optional<glm::vec3> _lastHandPosition;
	chimney_smoke::HandWind _handWind;
};

} // namespace openblack::ecs::systems
