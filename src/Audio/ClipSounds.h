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

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <SASFile.h>
#include <entt/core/hashed_string.hpp>

#include "AnimEffectKeys.h"

/// The sounds placed on the frames of the people's, animals' and birds' clips, and which of them a clip passes as it
/// plays on
namespace openblack::audio::clip_sounds
{

/// Every clip's sounds, by the clip's name in the animation pack
class ClipSoundTable
{
public:
	ClipSoundTable() = default;
	explicit ClipSoundTable(const sas::SASFile& file);
	/// The sounds of a clip, none when the file places none on it
	[[nodiscard]] const sas::ClipSounds* Find(std::string_view clip) const;

private:
	std::unordered_map<std::string, sas::ClipSounds> _byClip;
};

/// Which of a clip's sounds play as it plays on from a place by so many milliseconds: those whose time is in the
/// stretch played. Playing past the clip's end plays to its end, then a looping clip plays on from its start to where
/// it comes round to, and a clip played once plays all its sounds again from the start (its callers stop playing a clip
/// that has finished). The sounds are given by their place in the list, once for each time they play.
[[nodiscard]] std::vector<size_t> Passed(std::span<const sas::FrameSound> sounds, uint32_t place, uint32_t played,
                                         uint32_t duration, bool looping);

/// The size a clip's sound is played for: the people's clips by the person (a man large, a woman medium, a child or
/// anything else small); any other kind of thing's clips medium
[[nodiscard]] SoundSize SizeOf(int32_t soundType, bool isVillager, bool child, bool woman);

/// The one table's id in the resource cache
inline constexpr entt::hashed_string k_TableId = "clip_sounds";

/// The kind of sound the people's clips carry
inline constexpr int32_t k_PeopleSounds = 1;
/// The sounds played from the villagers' banter bank rather than the editor's; the first is played from the villager's
/// home
inline constexpr int32_t k_HomeBanter = 0x92;
inline constexpr int32_t k_LastBanter = 0x94;
/// A person's thrown clip screams only so many turns into its flight, the thrown-in-a-vortex clip fewer
inline constexpr uint16_t k_ThrownSoundTurns = 15;
inline constexpr uint16_t k_ThrownVortexSoundTurns = 10;

} // namespace openblack::audio::clip_sounds
