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

#include "Blast/CameraShake.h"
#include "Blast/DustPuff.h"
#include "ECS/Systems/ExplosionSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class ExplosionSystem final: public ExplosionSystemInterface
{
public:
	void AddRubble(const glm::vec3& centre, float yaw, float scale) override;
	void AddShake(const glm::vec3& position, float radius, float strength, float seconds) override;
	[[nodiscard]] bool IsShaking() const override { return !_shakes.empty(); }
	void Update(float milliseconds) override;
	void CollectDrawFrame(particles::draw::Frame& frame) const override;
	void Reset() override;

private:
	std::vector<camera_shake::Shake> _shakes;
	std::vector<dust_puff::Puff> _puffs;
};

} // namespace openblack::ecs::systems
