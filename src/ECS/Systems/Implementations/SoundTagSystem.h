/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/SoundTagSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class SoundTagSystem final: public SoundTagSystemInterface
{
public:
	void ProcessTurn(const glm::vec3& camera) override;
	void SetActive(entt::entity entity, bool active) override;
};

} // namespace openblack::ecs::systems
