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

#include <array>
#include <functional>
#include <optional>

#include <entt/core/fwd.hpp>
#include <glm/mat4x4.hpp>

#include "3D/TempleInteriorInterface.h"

namespace openblack
{

/// The doors of the temple's main room, which swing open as the camera walks through them (InnerRoom's door). Each
/// doorway has two leaves, submeshes of the main room whose joints are numbered in pairs: the first leaf of the doorway
/// and the one after it. The leaves of the open doorway turn about their hinges, away from the room.
class TempleDoors
{
public:
	/// LH3D's table of joint matrices which InnerRoom::SetDoorMatrices fills
	static constexpr size_t k_JointCount = 32;
	/// How far each leaf turns at most, twice this, in radians
	static constexpr float k_HalfSwing = 0.6981317f;

	using Joints = std::array<glm::mat4, k_JointCount>;
	/// Plays one of the doors' sounds
	using PlaySound = std::function<void(entt::id_type)>;

	explicit TempleDoors(PlaySound playSound);

	/// The first leaf of the doorway through to a room from the main room
	[[nodiscard]] static std::optional<uint32_t> LeafOf(TempleRoom room);

	/// Starts the leaves of a doorway on their way from a point of their swing at a rate per second, unless they're
	/// already on their way open. Between 0 and 1 the doorway opens, then closes up to 2. Before 0, it waits.
	void Open(std::optional<uint32_t> leaf, float from, float rate);
	/// Turns the open doorway round to close at a rate per second, from as far open as it is
	void Close(float rate);
	/// Shuts the open doorway at once
	void FastClose();
	/// Swings the doorway on, and lays out the joints
	void Update(float deltaSeconds);

	[[nodiscard]] std::optional<uint32_t> GetLeaf() const { return _leaf; }
	[[nodiscard]] float GetSwing() const { return _swing; }
	/// Each joint's turn: the leaves of the doorway turn about their hinges and the rest stay
	[[nodiscard]] const Joints& GetJoints() const { return _joints; }

private:
	void LayOutJoints();

	PlaySound _playSound;
	std::optional<uint32_t> _leaf;
	float _swing {0.0f};
	float _rate {0.0f};
	Joints _joints;
};

} // namespace openblack
