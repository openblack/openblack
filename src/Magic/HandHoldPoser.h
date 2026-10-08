/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Magic/HandHoldPose.h"

namespace openblack
{
class HandAnimation;
namespace ecs::components
{
struct Transform;
} // namespace ecs::components
} // namespace openblack

namespace openblack::magic
{

/// Poses the player's hand around the miracle seed it holds, each frame (see HandHoldPose.h): the hand rises for the
/// seed, takes the still frame of the hold, sways with the seed as the cursor moves and tips with a pour, and the seed's
/// model hangs in it. Taking or losing a seed cross-fades the drawn hand.
class HandHoldPoser
{
public:
	/// The seed in the hand, as the hand holds it
	struct HeldSeed
	{
		entt::entity entity {entt::null};
		HoldType hold {HoldType::Magic};
		/// How far below the hand point its model hangs
		float hang {0.0f};
		/// Its scale times its hold radius
		float reach {1.0f};
		float yRotate {0.0f};
		/// Its in-hand effect sits in the fingers rather than at the hand point
		bool effectInFingers {true};
	};

	/// The seed in the player's hand, if any
	[[nodiscard]] static std::optional<HeldSeed> Find();
	/// How far the hand rises for the seed, with the land under the cursor at a distance from the camera
	[[nodiscard]] static float Lift(const HeldSeed& seed, float landDistance);

	/// What the pose needs of the frame
	struct Frame
	{
		std::chrono::microseconds dt {0};
		/// The cursor in window pixels
		glm::ivec2 cursor {0};
		glm::vec3 camera {0.0f};
		/// The hand's turn about the vertical, the way it faces
		glm::mat3 levelTurn {1.0f};
		/// The hand model's turn upright
		glm::mat3 modelCorrection {1.0f};
		/// How far a pour tips the hand
		float tilt {0.0f};
		bool rightHanded {false};
	};
	/// Poses the hand holding the seed, if it holds one: its animation, its turn and the seed. Whether it holds one; the
	/// hand isn't animated otherwise.
	bool Pose(const std::optional<HeldSeed>& seed, const Frame& frame, HandAnimation* animation,
	          ecs::components::Transform& hand);
	/// The drawn hand, faded from where it last was drawn while it takes or lets go of a seed
	void Fade(const std::optional<HeldSeed>& seed, std::chrono::microseconds dt, ecs::components::Transform& hand);
	/// Once the hand is drawn at its size: the held seed's in-hand effect is placed in its fingers
	void PlaceHandEffect(const std::optional<HeldSeed>& seed, const HandAnimation* animation,
	                     const ecs::components::Transform& hand, float handSize) const;

private:
	hand_hold::HandFade _fade;
	entt::entity _lastSeed {entt::null};
	std::optional<glm::vec3> _drawnPosition;
	glm::mat3 _drawnRotation {1.0f};
};

} // namespace openblack::magic
