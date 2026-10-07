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

// What becomes of a particle's sound once the particle lets go of it, or goes. A sound the effect file doesn't
// soft-release is stopped at once, whether its sample loops or not. One it soft-releases plays on: a loop to the end of
// its pass, a sample that plays once to its end. Each turn from the one it is let go on, the sound gets quieter by its
// fade step, so a step of 20 is silent on the seventh. Pure rules, so they are tested without audio.

namespace openblack::particles
{

/// The loudest a sound plays, as the game's volumes go
inline constexpr uint32_t k_MaxSoundVolume = 127;

/// How a sound is let go
struct SoundLetGo
{
	/// The loop stops looping: the sample plays to the end of its pass and stops
	bool releaseLoop {false};
	/// How much quieter it gets each turn, out of k_MaxSoundVolume; 0 to keep its volume
	int fadeStep {0};
	/// Stop it once it is silent, rather than letting the sample end by itself
	bool stopWhenSilent {false};
	/// Stop it at once, as a sound the file doesn't soft-release
	bool stopNow {false};

	bool operator==(const SoundLetGo&) const = default;
};

/// How to let go of a sound: whether the effect file soft-releases it, whether its sample loops, and the fade step the
/// effect gave it
[[nodiscard]] SoundLetGo LetGoOf(bool softRelease, bool sampleLoops, int fadeStep);

/// A sound's volume a turn after it was let go
[[nodiscard]] uint32_t FadedVolume(uint32_t volume, int fadeStep);

} // namespace openblack::particles
