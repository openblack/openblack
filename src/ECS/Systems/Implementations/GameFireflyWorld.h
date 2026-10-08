/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

#include "Nature/FireflyWorld.h"

namespace openblack::ecs::systems
{

/// The fireflies' world in the game: the land's map and things, its clock, the camera and the miracles
class GameFireflyWorld final: public fireflies::FireflyWorldInterface
{
public:
	[[nodiscard]] Registry& Entities() override;
	uint32_t GameRand(uint32_t n) override;
	float GameFloatRand(float x) override;
	[[nodiscard]] float VisualHour() const override;
	[[nodiscard]] std::array<float, 4> SkyHours() const override;
	[[nodiscard]] float TurnSeconds() const override;
	[[nodiscard]] glm::vec3 ToWorld(const map_coords::MapCoords& coords) const override;
	[[nodiscard]] map_coords::MapCoords FromWorld(glm::vec3 point) const override;
	[[nodiscard]] std::vector<entt::entity> ThingsInCell(glm::ivec2 cell) const override;
	[[nodiscard]] std::optional<map_coords::MapCoords> SpotOf(entt::entity thing) const override;
	[[nodiscard]] bool IsHidingPlace(entt::entity thing) const override;
	[[nodiscard]] bool IsRock(entt::entity thing) const override;
	[[nodiscard]] bool IsBuilding(entt::entity thing) const override;
	[[nodiscard]] float HeightOf(entt::entity thing) const override;
	[[nodiscard]] std::optional<size_t> MagicKindNamed(std::string_view name) const override;
	void MakeReward(size_t kind, const map_coords::MapCoords& spot) override;
	[[nodiscard]] glm::vec3 CameraPosition() const override;
	[[nodiscard]] bool InView(glm::vec3 point) const override;
	[[nodiscard]] std::optional<components::Sprite> Look() const override;
};

} // namespace openblack::ecs::systems
