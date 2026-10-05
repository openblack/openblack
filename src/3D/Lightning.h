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

#include <glm/vec3.hpp>

/// A storm's lightning: now and then it flashes, brightly with thunder or dimly with a forked bolt. The flash lights the
/// land around the camera while the camera is inside the storm.
namespace openblack::lightning
{

/// How long a flash lasts, in seconds
inline constexpr float k_FlashSeconds = 0.8f;
/// The strengths of a flash with thunder and of one with a bolt
inline constexpr float k_ThunderStrength = 1.0f;
inline constexpr float k_BoltStrength = 0.5f;

/// A flash of lightning where it struck
struct Flash
{
	glm::vec3 position {0.0f};
	float radius {0.0f};
	float strength {0.0f};
	/// Seconds since it struck
	float time {0.0f};
	bool active {false};
};

/// A new flash
[[nodiscard]] Flash Strike(const glm::vec3& position, float radius, float strength);

/// A flash after `seconds` more; it goes out once it is older than k_FlashSeconds
[[nodiscard]] Flash Advance(Flash flash, float seconds);

/// How bright a flash is now, 0 to its strength: it fades from full as it ages, but flickers down to a tenth between a
/// tenth and half of a second
[[nodiscard]] float Brightness(const Flash& flash);

/// The flash the land's light takes, 0 to 255, from a brightness
[[nodiscard]] uint8_t LandLightFlash(float brightness);

} // namespace openblack::lightning
