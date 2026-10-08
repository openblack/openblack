/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <entt/entity/registry.hpp>
#include <entt/signal/sigh.hpp>

#include "ECS/Systems/FireflySystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

#include "Nature/Fireflies.h"
#include "Nature/FireflyWorld.h"

namespace openblack::ecs::systems
{

class FireflySystem final: public FireflySystemInterface
{
public:
	/// The fireflies of the game's land
	FireflySystem();
	/// The fireflies of a world of their own, as tests give it
	explicit FireflySystem(std::unique_ptr<fireflies::FireflyWorldInterface> world);
	~FireflySystem() override;

	entt::entity Create(const map_coords::MapCoords& spot) override;
	void ProcessTurn() override;
	void Update(float milliseconds, float turnFraction) override;
	bool Catch(const map_coords::MapCoords& spot) override;
	void SetRewardWeight(std::string_view magicName, float weight) override;
	[[nodiscard]] const fireflies::RewardTable& GetRewards() const override { return _rewards; }
	[[nodiscard]] std::span<const entt::entity> GetFireflies() const override { return _order; }
	void Reset() override;

private:
	/// At nightfall: new fireflies hidden in random trees and rocks until the land holds its most
	void TopUp();
	/// The first in line comes out, if it is hiding where a tree or rock still stands and alone there
	void SendOutOnce();
	/// The first in line goes home, if it is out
	void SendHomeOnce();
	/// A turn of a firefly's flight
	void Fly(entt::entity firefly);
	/// The first in line goes to the back
	void ToTheBack();
	/// A building near a place to hover by, or a tree or rock to hide in, roughly the nearest
	[[nodiscard]] std::optional<fireflies::Found> Search(const map_coords::MapCoords& from, bool building);
	/// The spot a firefly hovers at, out from a hiding place; and the spot it hides at, from where it hovered
	[[nodiscard]] map_coords::MapCoords PlaceToHover(const map_coords::MapCoords& from);
	[[nodiscard]] map_coords::MapCoords PlaceToHide(const map_coords::MapCoords& from);

	void OnTreeMade(entt::registry& registry, entt::entity entity);
	void OnTreeGone(entt::registry& registry, entt::entity entity);
	void OnFixedMade(entt::registry& registry, entt::entity entity);
	void OnFixedGone(entt::registry& registry, entt::entity entity);
	void OnFireflyGone(entt::registry& registry, entt::entity entity);

	std::unique_ptr<fireflies::FireflyWorldInterface> _world;
	/// The fireflies, the first in line at the front: new ones join at the front, and those sent go to the back
	std::vector<entt::entity> _order;
	/// The land's trees, and its things that stand over several cells, oldest first
	std::vector<entt::entity> _trees;
	std::vector<entt::entity> _fixed;
	/// The most a land holds; the same on every land
	size_t _most {fireflies::k_DefaultMaximum};
	/// The next nightfall tops the fireflies up
	bool _topUpDue {true};
	fireflies::RewardTable _rewards;
	std::vector<entt::scoped_connection> _connections;
};

} // namespace openblack::ecs::systems
