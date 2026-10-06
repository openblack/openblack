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
#include <optional>
#include <unordered_map>
#include <vector>

#include <entt/core/hashed_string.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "3D/SkeletalAnimation.h"
#include "Common/Zoomer.h"
#include "Creature/CreatureEyes.h"
#include "Creature/CreatureMorph.h"

namespace openblack::ecs::components
{

/// How a creature's body is drawn: its base mesh pulled towards its evil or good, thin or fat and weak or strong
/// meshes, as the creature has become
struct CreatureMorph
{
	/// The fatness the body shows, which follows the creature's own a little each turn
	float shownFatness {0.5f};
	/// How far along each axis the body is drawn
	creature_morph::Morph drawn {};
	/// Goes up each time the animations and rest pose have to follow the body anew
	uint32_t revision {0};
};

/// How a creature's body is posed. The animations follow the body's evil to good and thin to fat axes, built again
/// when they change.
struct CreatureAnimation
{
	/// What the creature's body is doing, which picks its animation
	enum class State : uint8_t
	{
		/// Standing, breathing
		Idle,
	};
	State state {State::Idle};

	/// How far through a breath the creature is, 0 to 1, and the seconds a breath takes now
	float breathPhase {0.0f};
	float breathPeriod {0.0f};

	/// The CreatureMorph revision the animations below were built for
	std::optional<uint32_t> builtRevision;
	/// The rest pose, blended as the body is, and the animations blended so far, by their index in the creature spec
	skeletal_animation::Skeleton skeleton;
	std::unordered_map<size_t, skeletal_animation::Animation> animations;

	/// The bones' matrices in the mesh's space as posed this frame, which the body is drawn with
	std::vector<glm::mat4> boneMatrices;
};

/// A creature's eyes: where they look and blink, and where each eyeball and eyelid is drawn this frame
struct CreatureEyes
{
	/// The eyeball and eyelid every creature's eyes are drawn with, Data/Eyeball.l3d and Data/Eyelid.l3d
	static constexpr entt::id_type k_EyeballMeshId = entt::hashed_string("creature/eyeball");
	static constexpr entt::id_type k_EyelidMeshId = entt::hashed_string("creature/eyelid");

	creature_eyes::Mode mode {creature_eyes::Mode::Calm};
	/// How open the eyes are, -1 to about 0.75: sleepy eyes turn slowly and droop
	float openness {0.0f};
	creature_eyes::Blink blink {};
	/// Where the creature looks, when it looks at something rather than ahead
	std::optional<glm::vec3> lookAt;
	/// Which way the back of each eye faces as it eases round to where it looks
	std::array<Zoomer3, 2> look;
	std::array<bool, 2> lookStarted {};

	struct Eye
	{
		std::optional<glm::mat4> eyeball;
		std::optional<glm::mat4> eyelid;
	};
	std::array<Eye, 2> drawn {};
	/// The skin's colour under the eyelids, which they are drawn in
	glm::vec3 lidColour {1.0f};
};

} // namespace openblack::ecs::components
