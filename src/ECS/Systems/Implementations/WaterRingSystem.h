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

#include "ECS/Systems/WaterRingSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class WaterRingSystem final: public WaterRingSystemInterface
{
public:
	void Reset() override { _rings.clear(); }
	bool Add(const water_rings::Ring& ring) override;
	void Update(std::chrono::duration<float, std::milli> gameTime) override;
	[[nodiscard]] std::span<const water_rings::Ring> GetRings() const override { return _rings; }

private:
	std::vector<water_rings::Ring> _rings;
};

} // namespace openblack::ecs::systems
