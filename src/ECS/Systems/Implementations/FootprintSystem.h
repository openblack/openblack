/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/FootprintSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class FootprintSystem final: public FootprintSystemInterface
{
public:
	void Update(std::chrono::duration<float, std::milli> gameTime) override;
	void Step(entt::entity creature) override;
	void Reset() override;
	[[nodiscard]] std::span<const creature_footprints::Footprint> GetPrints() const override { return _trail.prints; }
	[[nodiscard]] size_t GetDroppedCount() const override { return _dropped; }
	[[nodiscard]] bool IsShown() const override { return _shown; }
	void SetShown(bool shown) override { _shown = shown; }
	[[nodiscard]] std::optional<bool> GetAprilFoolsOverride() const override { return _aprilFools; }
	void SetAprilFoolsOverride(std::optional<bool> aprilFools) override { _aprilFools = aprilFools; }
	[[nodiscard]] bool IsAprilFools() const override;

private:
	creature_footprints::Trail _trail;
	size_t _dropped {0};
	bool _shown {true};
	std::optional<bool> _aprilFools;
};

} // namespace openblack::ecs::systems
