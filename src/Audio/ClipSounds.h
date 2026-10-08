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
	/// The sounds placed under a clip's name, none when the file places none on it
	[[nodiscard]] const sas::ClipSounds* Find(std::string_view clip) const;
	/// Gives each name's sounds to the first clip of the animation pack that bears the name, as the game does when it
	/// loads them: a later clip of the same name has none
	void Attach(std::span<const std::string_view> packNames);
	/// The sounds of the pack's clip at an index, none when no sounds were given to it
	[[nodiscard]] const sas::ClipSounds* OfClip(uint32_t packIndex) const;

private:
	std::unordered_map<std::string, sas::ClipSounds> _byClip;
	std::unordered_map<uint32_t, const sas::ClipSounds*> _byPackIndex;
};

/// The bank a clip's sound plays from
enum class Bank : uint8_t
{
	Editor,
	Banter,
};

/// What becomes of one of a clip's sounds as the clip passes it
enum class Outcome : uint8_t
{
	/// It plays
	Play,
	/// It stays quiet, and the clip's later sounds are still heard
	Skip,
	/// It and every later sound of the clip this time stay quiet
	Stop,
};

/// Who and where a clip's sound is played by
struct SoundRoute
{
	Outcome outcome {Outcome::Skip};
	Bank bank {Bank::Editor};
	/// The sound belongs to the villager's home (a home it may not have) rather than to the villager
	bool fromHome {false};
};

/// What is known of the thing whose clip plays one of its sounds
struct SoundSource
{
	int32_t soundType {0};
	/// The sound's action and how it is played: only sounds played the ordinary way (mode 0) are kept quiet by where
	/// the player is
	int32_t action {0};
	int32_t mode {0};
	/// The pack index of the clip playing
	uint32_t clip {0};
	bool isVillager {false};
	bool alive {true};
	/// The turns since the thing's state last changed
	uint16_t turnsInState {0};
	/// The player's view is inside the temple
	bool insideTemple {false};
};

/// How one of a clip's sounds is played: a person's sounds only while it is alive; the banter from the villagers' bank,
/// the first of it from a villager's home; a thrown person's scream only early in its flight; anything else from the
/// editor's bank; and sounds played the ordinary way not while the player is inside the temple
[[nodiscard]] SoundRoute RouteOf(const SoundSource& source);

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
/// The pack indexes of a person's thrown clips
inline constexpr uint32_t k_ThrownClip = 399;
inline constexpr uint32_t k_ThrownVortexClip = 401;

} // namespace openblack::audio::clip_sounds
