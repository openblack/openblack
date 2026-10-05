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

#include "3D/SnowCover.h"
#include "ECS/Systems/SnowSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class SnowSystem final: public SnowSystemInterface
{
public:
	SnowSystem();

	void Reset() override;
	void ProcessTurn(std::span<const components::Storm> storms) override;

	[[nodiscard]] std::span<const float> GetDepths() const override { return _depths; }
	[[nodiscard]] float GetDepth(glm::vec2 xz) const override { return snow_cover::DepthAt(_depths, xz); }
	[[nodiscard]] uint32_t GetRevision() const override { return _revision; }

private:
	std::vector<float> _depths;
	snow_cover::Melting _melting;
	uint32_t _revision {0};
};

} // namespace openblack::ecs::systems
