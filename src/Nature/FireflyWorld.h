/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <string_view>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/MapCoords.h"
#include "ECS/Components/Sprite.h"

namespace openblack::ecs
{
class Registry;
}

namespace openblack::fireflies
{

/// What the fireflies need of the land: its things and their spots, the game's random numbers, the shown clock, the
/// camera and the miracles. The game gives the real land; tests give a world of their own.
class FireflyWorldInterface
{
public:
	virtual ~FireflyWorldInterface() = default;

	/// Where the fireflies, and the trees and rocks they hide in, are kept
	[[nodiscard]] virtual ecs::Registry& Entities() = 0;

	/// The game's random numbers, which every player's game draws alike: below n, and from 0 to x
	virtual uint32_t GameRand(uint32_t n) = 0;
	virtual float GameFloatRand(float x) = 0;

	/// The hour of the shown clock, and the hours of full night, the start and end of dusk, and full day
	[[nodiscard]] virtual float VisualHour() const = 0;
	[[nodiscard]] virtual std::array<float, 4> SkyHours() const = 0;
	/// The length of a game turn, in seconds
	[[nodiscard]] virtual float TurnSeconds() const = 0;

	/// A map position as a world point, and back
	[[nodiscard]] virtual glm::vec3 ToWorld(const map_coords::MapCoords& coords) const = 0;
	[[nodiscard]] virtual map_coords::MapCoords FromWorld(glm::vec3 point) const = 0;

	/// The things in a cell, in the order a search meets them
	[[nodiscard]] virtual std::vector<entt::entity> ThingsInCell(glm::ivec2 cell) const = 0;
	/// A thing's map position, none for one no longer there
	[[nodiscard]] virtual std::optional<map_coords::MapCoords> SpotOf(entt::entity thing) const = 0;
	/// A rock (a bonfire too) or any kind of tree, living, dead or felled: where a firefly hides
	[[nodiscard]] virtual bool IsHidingPlace(entt::entity thing) const = 0;
	/// A rock or a bonfire
	[[nodiscard]] virtual bool IsRock(entt::entity thing) const = 0;
	/// Any building of a town (a house, a field, a store, a workshop, a dispenser...): where a firefly hovers
	[[nodiscard]] virtual bool IsBuilding(entt::entity thing) const = 0;
	/// How tall a thing's model stands, at its size
	[[nodiscard]] virtual float HeightOf(entt::entity thing) const = 0;

	/// The magic type, by number, with an effect of this name (ignoring case), if any
	[[nodiscard]] virtual std::optional<size_t> MagicKindNamed(std::string_view name) const = 0;
	/// A one-shot miracle of a magic type, at full strength, floating at a spot; nothing for a magic type without a seed
	virtual void MakeReward(size_t kind, const map_coords::MapCoords& spot) = 0;

	/// Where the camera is, and whether a point is in its view
	[[nodiscard]] virtual glm::vec3 CameraPosition() const = 0;
	[[nodiscard]] virtual bool InView(glm::vec3 point) const = 0;
	/// The sprite a firefly is drawn with, none when it can't be drawn
	[[nodiscard]] virtual std::optional<ecs::components::Sprite> Look() const = 0;
};

} // namespace openblack::fireflies
