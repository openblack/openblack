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
#include <optional>

#include "ECS/Systems/VegetationInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

/// How Black & White's trees move (Tree::PreDraw and Tree::Draw).
///
/// Trees share 16 sways, each a slow back and forth whose speed changes at random every two seconds of game time. A
/// tree follows the sway its facing picks, so trees facing the same way move together. The sway leans the tree along
/// the world's z axis by up to 3% of its height, whatever the weather.
///
/// Things passing through the trees bend them instead: a tree within reach of the hand leans away from it by up to 27
/// degrees, less the further it is, and stands straight again as soon as the hand has gone. Bending them makes them
/// crash about, and they rustle by themselves around the camera (TreeRustle).
///
/// Fully grown fields follow the same sways, leaning their crops 1.75 times as far.
// TODO(raffclar): creatures and thrown objects bend the trees they pass in vanilla too
class VegetationSystem final: public VegetationInterface
{
public:
	void Update(std::chrono::duration<float, std::milli> gameTime) override;
	void UpdateBendPoints() override;
	void Rustle(std::chrono::duration<float, std::milli> gameTime) override;
	[[nodiscard]] glm::mat4 GetTreeMatrix(const glm::mat4& model, const glm::vec3& position, float scale, float height,
	                                      uint8_t swaySlot) const override;
	[[nodiscard]] glm::mat4 GetFieldMatrix(const glm::mat4& model, float scale, uint8_t swaySlot) const override;

private:
	struct BendPoint
	{
		glm::vec3 position;
		float radius;
	};

	struct Bend
	{
		/// Horizontal, from the bend point to the tree
		glm::vec3 direction;
		/// 0 standing to 1 at the most
		float amount;
	};

	/// How the bend points bend a tree standing at position, height tall, null when they don't
	[[nodiscard]] std::optional<Bend> GetBend(const glm::vec3& position, float height) const;

	/// Lean of each sway along the z axis, per unit of a tree's height
	std::array<float, k_SwayCount> _leans {};
	std::array<float, k_SwayCount> _phases {};
	/// Phase speed of each sway, 1 to 2
	std::array<float, k_SwayCount> _speeds {};
	/// Game time since the speeds last changed
	std::chrono::duration<float, std::milli> _speedTime {0.0f};
	std::optional<BendPoint> _hand;
};
} // namespace openblack::ecs::systems
