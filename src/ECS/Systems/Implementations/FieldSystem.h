/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/FieldSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class FieldSystem final: public FieldSystemInterface
{
public:
	void ProcessTurn(uint32_t turn) override;
	void Update(std::chrono::duration<float, std::milli> gameTime) override;
	[[nodiscard]] std::optional<field_crop::Look> GetLook(const components::Field& field) const override;
};

} // namespace openblack::ecs::systems
