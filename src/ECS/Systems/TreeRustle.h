/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cmath>
#include <cstdint>

#include <chrono>
#include <string_view>

#include <glm/vec3.hpp>

#include "Audio/AnimEffectKeys.h"

namespace openblack::ecs::systems
{

/// When Black & White's trees rustle (Tree::Draw).
///
/// A tree being bent crashes about, louder the further it is bent: a bend of more than k_BendLarge plays the large
/// collision of a tree, of more than k_BendMedium the medium one and less the small one, of which the game's bank has
/// none. Trees taller than k_MinIdleHeight also rustle and creak by themselves while the camera is among them, each
/// about once a second of game time. A tree plays one of each at a time, the bank plays them once (see
/// AudioManagerInterface::PlayAnimEffect).
struct TreeRustle
{
	/// The bank the sounds are in
	static constexpr std::string_view k_Bank = "editor.sad";

	static constexpr float k_BendLarge = 0.67f;
	static constexpr float k_BendMedium = 0.3f;

	/// How close to a tree, along the x and z axes, the camera is among it
	static constexpr float k_CameraReach = 10.0f;
	/// How close above or below the tree's base the camera is among it
	static constexpr float k_CameraHeight = 18.0f;
	/// How tall a tree is to rustle by itself
	static constexpr float k_MinIdleHeight = 10.0f;
	/// On average, a tree around the camera rustles once in this much game time
	static constexpr std::chrono::duration<float, std::milli> k_IdleTime {1000.0f};

	/// The sound of a tree bent by bend, from 0 standing to 1 at the most
	[[nodiscard]] static constexpr audio::AnimEffectKeys BendKeys(float bend) noexcept
	{
		using audio::SoundSize;
		auto size = SoundSize::Large;
		if (bend < k_BendMedium)
		{
			size = SoundSize::Small;
		}
		else if (bend < k_BendLarge)
		{
			size = SoundSize::Medium;
		}
		return {.size = size, .surface = audio::SoundSurface::Tree, .action = audio::SoundAction::Collide};
	}

	/// The sound of a tree rustling by itself
	[[nodiscard]] static constexpr audio::AnimEffectKeys IdleKeys() noexcept
	{
		return {.object = audio::SoundObject::Tree, .action = audio::SoundAction::Tree};
	}

	/// Whether a tree standing at position, height tall, rustles by itself with the camera at camera
	[[nodiscard]] static bool IsAmongTree(const glm::vec3& position, float height, const glm::vec3& camera) noexcept
	{
		return std::abs(position.x - camera.x) <= k_CameraReach && std::abs(position.z - camera.z) <= k_CameraReach &&
		       std::abs(camera.y - position.y) < k_CameraHeight && height > k_MinIdleHeight;
	}

	/// A tree among the camera rustles this frame when a roll from 0 to this, less one, comes up 1: with a chance of
	/// one in this, so that it does about once in k_IdleTime. 0 or 1 when it never does, as while the game is paused.
	[[nodiscard]] static uint32_t IdleChance(std::chrono::duration<float, std::milli> gameTime) noexcept
	{
		if (gameTime.count() <= 0.0f)
		{
			return 0;
		}
		const auto chance = k_IdleTime / gameTime;
		return chance >= static_cast<float>(UINT32_MAX) ? 0 : static_cast<uint32_t>(chance);
	}
};

} // namespace openblack::ecs::systems
