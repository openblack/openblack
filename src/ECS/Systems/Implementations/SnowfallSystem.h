/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>

#include "ECS/Systems/SnowfallSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class SnowfallSystem final: public SnowfallSystemInterface
{
public:
	SnowfallSystem();

	void Reset() override;
	void Update(float seconds, const rain::Fall& fall) override;
	[[nodiscard]] std::vector<snowfall::Tile> TakeTiles(const glm::vec3& camera) override;
	[[nodiscard]] std::span<const snowfall::Flake> GetFlakes() const override { return _flakes; }

private:
	std::array<snowfall::Flake, snowfall::k_Flakes> _flakes {};
	/// Whether the flakes were drawn since the last update, which moves them on only then
	bool _drawn {false};
};

} // namespace openblack::ecs::systems
