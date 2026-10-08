/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <entt/entity/entity.hpp>

#include "ECS/Systems/MiracleFxSystemInterface.h"
#include "Magic/TribalPowerSpin.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class MiracleFxSystem final: public MiracleFxSystemInterface
{
public:
	void Update(float seconds, float gameSeconds) override;
	void SeedInHand(int powerUp, int previousPowerUp) override;
	void SeedLeftHand() override;
	void SeedShakenOff() override;
	void Reset() override;
	void SetInterface(const gui::GameInterface* interface) override { _interface = interface; }
	void StartTribalPowerRing(Tribe tribe) override;
	void StopTribalPowerRing() override;
	void ReleaseTribalPowerRing(Tribe tribe, glm::vec3 handPosition) override;
	void TribalPowerColumn(Tribe tribe, glm::vec3 position, PlayerNames player) override;
	void UpdateTribalPower(float gameSeconds) override;
	[[nodiscard]] std::vector<OrientedTextVertex> GetTribalPowerText() const override;
	[[nodiscard]] const graphics::Texture2D* GetTextTexture() const override;
	[[nodiscard]] entt::id_type BubbleMesh() override;
	[[nodiscard]] entt::id_type BandMesh() override;

private:
	/// The globes: their glint and rings run on, they face the camera, and the miracle in each spins or plays
	void UpdateGlobes(float seconds);
	/// The hand: its bands fly and spin, and it glows while it holds a miracle
	void UpdateHand(float seconds);
	/// The piles: each eases to how far it is raised for what it holds, from under the ground where it was made
	static void UpdatePiles(float gameSeconds);
	/// A model of the game's data by its path, loaded once under an id
	entt::id_type LoadMesh(entt::id_type id, std::string_view file);
	void LoadMeshes();

	/// A tribe's name, as the interface's text has it
	[[nodiscard]] std::u16string TribeName(Tribe tribe) const;

	/// The effect playing in each globe
	std::unordered_map<entt::entity, uint32_t> _holderEffects;
	const gui::GameInterface* _interface {nullptr};
	/// The tribes' names spinning in the world, and the one ringing this computer's hand among them
	std::vector<std::unique_ptr<magic::tribal_spin::Runner>> _runners;
	magic::tribal_spin::Runner* _ring {nullptr};
};

} // namespace openblack::ecs::systems
