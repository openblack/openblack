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
#include <map>
#include <optional>
#include <string_view>
#include <vector>

#include <glm/vec3.hpp>

namespace openblack::audio
{

/// MUSIC_TYPE
enum class MusicType : int32_t
{
	None = 0,
	GenericEvil,
	GenericNeutral,
	GenericGood,
	CelticTownEvil,
	CelticTownNeutral,
	CelticTownGood,
	AztecTownEvil,
	AztecTownNeutral,
	AztecTownGood,
	JapaneseTownEvil,
	JapaneseTownNeutral,
	JapaneseTownGood,
	IndianTownEvil,
	IndianTownNeutral,
	IndianTownGood,
	EgyptianTownEvil,
	EgyptianTownNeutral,
	EgyptianTownGood,
	GreekTownEvil,
	GreekTownNeutral,
	GreekTownGood,
	NorseTownEvil,
	NorseTownNeutral,
	NorseTownGood,
	TibetanTownEvil,
	TibetanTownNeutral,
	TibetanTownGood,
	CelticChant,
	CelticChantVox,
	AztecChant,
	AztecChantVox,
	JapaneseChant,
	JapaneseChantVox,
	IndianChant,
	IndianChantVox,
	EgyptianChant,
	EgyptianChantVox,
	GreekChant,
	GreekChantVox,
	NorseChant,
	NorseChantVox,
	TibetanChant,
	TibetanChantVox,
	CitadelEvil,
	CitadelNeutral,
	CitadelGood,
	ScriptPiperTune,
	ScriptPiperCaveTune,
	ScriptHermit,
	ScriptMissionariesBackground,
	ScriptMissionariesVerse1,
	ScriptMissionariesVerse2,
	ScriptMissionariesVerse3,
	ScriptIntro,
	ScriptSingingStoneCircle,
	ScriptWelcomeDance,
	ScriptGeneric01,
	ScriptGeneric02,
	ScriptGeneric03,
	ScriptGeneric04,
	ScriptEpic01,
	ScriptEpic02,
	ScriptEpic03,
	ScriptEpic04,
	ScriptCreatureChosen,
	ScriptFuneral,
	ScriptCreatureGuide,
	ScriptKhazar,
	ScriptNemesis,
	ScriptTwinkle,
	ScriptWhistleFuneral,
	ScriptWhistleTwinkle,
	ScriptSleg,
	CreatureFight,
	CreatureBigFight,
	ScriptGuardianStone,
	Outro,
	ScriptFailure,
	ScriptGregorian,
	ScriptChristmas,
	ScriptGregorian3D,
	ScriptCircus,
	ScriptCircus3D,
	ScriptCreatureEndSequence,

	_COUNT
};

/// The bank GAudio loads for a music type, relative to the game's directory. Empty for MusicType::None.
[[nodiscard]] std::string_view GetMusicBankPath(MusicType type);
[[nodiscard]] std::string_view GetMusicTypeName(MusicType type);

/// The music half of GAudio: once a game turn, picks what music plays and hands it to LHAudioDLL's music player.
///
/// In order, the first that wants to play wins: the citadel's music while inside the citadel, music a script has
/// started, and the music of the land under the camera. Over land the music follows the player's alignment, and near a
/// town that of its tribe too. Each music group remembers where it got to, so the land's music carries on in time when
/// it changes and picks up where it left off when it comes back.
// TODO(raffclar): GAudio also plays creature fight, chant, creature dance and object music before the land's music
class GameMusic
{
public:
	struct Town
	{
		glm::vec3 position;
		/// Tribe, from 0 (celtic) to 8 (tibetan)
		int32_t tribe;
		uint32_t id;
	};

	struct TurnInputs
	{
		uint32_t turn;
		glm::vec3 camera;
		/// Height of the land under the camera
		float groundHeight;
		/// Inside the citadel
		bool inCitadel;
		/// Alignment of the player, -1 (evil) to 1 (good)
		float alignment;
		std::vector<Town> towns;
	};

	GameMusic() = default;
	/// Stops the music, which would otherwise call back into it when it ends
	~GameMusic();
	GameMusic(const GameMusic&) = delete;
	GameMusic& operator=(const GameMusic&) = delete;

	/// GAudio::ProcessMusic
	void ProcessTurn(const TurnInputs& inputs);
	/// Forgets what was playing, when a land is loaded
	void Reset();

	/// GAudio::StartScriptMusic: MusicType::None stops it
	void StartScriptMusic(MusicType type);
	/// ENABLE_DISABLE_ALIGNMENT_MUSIC
	void SetAlignmentMusicEnabled(bool enabled) { _alignmentMusicEnabled = enabled; }

	// Debug introspection
	[[nodiscard]] MusicType GetPlaying() const { return _playing; }
	[[nodiscard]] MusicType GetLandType() const { return _landType; }
	[[nodiscard]] MusicType GetBlockedType() const { return _blockedType; }
	[[nodiscard]] uint32_t GetBlockedTurns() const { return _blockedTurns; }
	[[nodiscard]] MusicType GetScriptType() const { return _scriptType; }
	[[nodiscard]] bool IsAlignmentMusicEnabled() const { return _alignmentMusicEnabled; }
	[[nodiscard]] const std::map<int32_t, uint32_t>& GetResumeChunks() const { return _resumeChunks; }

	/// GAlignment::GetDiscreteAlignmentValue followed by the evil, neutral or good index of the music tables
	[[nodiscard]] static int32_t GetAlignmentIndex(float alignment);
	/// fn_00427460: the land's music at the camera
	[[nodiscard]] MusicType SelectLandType(const TurnInputs& inputs);

private:
	void ProcessMusic(const TurnInputs& inputs);
	bool ProcessCitadel(const TurnInputs& inputs);
	bool ProcessScript();
	bool ProcessLand(const TurnInputs& inputs);
	/// fn_004281C0: remembers where each playing music group has got to
	void SaveResumeChunks();
	[[nodiscard]] uint32_t GetResumeChunk(int32_t groupId) const;

	/// The music type heard
	MusicType _playing {MusicType::None};
	/// The land's music playing, None when other music has taken over
	MusicType _landType {MusicType::None};
	/// The land's music that played to its end: it is not played again for a while
	MusicType _blockedType {MusicType::None};
	uint32_t _blockedTurns {0};
	MusicType _scriptType {MusicType::None};
	bool _scriptStarted {false};
	bool _alignmentMusicEnabled {true};
	std::optional<uint32_t> _rememberedTown;
	/// 1-based chunk each music group carries on from
	std::map<int32_t, uint32_t> _resumeChunks;
};

} // namespace openblack::audio
