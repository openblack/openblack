/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <chrono>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::systems
{
class VegetationInterface
{
public:
	/// Number of sways that trees and fields share
	static constexpr uint8_t k_SwayCount = 16;

	virtual ~VegetationInterface() = default;
	/// Tree::PreDraw: moves the trees' sway on by the game time that has passed, which stops while the game is paused
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;
	/// GLandscape::Draw: notes where the hand is, for the trees it bends this frame
	virtual void UpdateBendPoints() = 0;
	/// Tree::Draw: the trees being bent crash about and the trees around the camera rustle now and then (TreeRustle),
	/// after gameTime has passed
	virtual void Rustle(std::chrono::duration<float, std::milli> gameTime) = 0;
	/// Tree::Draw: the matrix to draw a tree with, bent away from the hand when it is close, otherwise swaying.
	/// model places the tree, which stands at position, scale times its mesh's size. height is the height of its
	/// mesh's bounding box and swaySlot which of the shared sways it follows.
	[[nodiscard]] virtual glm::mat4 GetTreeMatrix(const glm::mat4& model, const glm::vec3& position, float scale, float height,
	                                              uint8_t swaySlot) const = 0;
	/// Field::Draw: the matrix to draw a fully grown field with, its crop swaying further than trees do. model places
	/// the field, which is scale times its mesh's size, and swaySlot is which of the shared sways it follows.
	[[nodiscard]] virtual glm::mat4 GetFieldMatrix(const glm::mat4& model, float scale, uint8_t swaySlot) const = 0;
};
} // namespace openblack::ecs::systems
