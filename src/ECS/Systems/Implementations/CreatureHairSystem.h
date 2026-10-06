/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/CreatureHairSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CreatureHairSystem final: public CreatureHairSystemInterface
{
public:
	void Update(std::chrono::duration<float, std::milli> gameTime) override;
	[[nodiscard]] bool IsShown() const override { return _shown; }
	void SetShown(bool shown) override;

private:
	bool _shown {true};
};

} // namespace openblack::ecs::systems
