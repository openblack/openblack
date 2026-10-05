/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GameMusic.h"

#include <algorithm>
#include <array>
#include <limits>
#include <string>

#include <glm/geometric.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "AudioManagerInterface.h"
#include "Locator.h"

using namespace openblack::audio;

namespace
{
struct MusicTypeInfo
{
	std::string_view bank;
	std::string_view name;
};

// The game's table of music types: the bank of each and its name
constexpr std::array<MusicTypeInfo, static_cast<size_t>(MusicType::_COUNT)> k_MusicTypes = {{
    {.bank = "", .name = "MUSIC_TYPE_NONE"},
    {.bank = "audio/music/align/evil.sad", .name = "MUSIC_TYPE_GENERIC_EVIL"},
    {.bank = "audio/music/align/neutral.sad", .name = "MUSIC_TYPE_GENERIC_NEUTRAL"},
    {.bank = "audio/music/align/good.sad", .name = "MUSIC_TYPE_GENERIC_GOOD"},
    {.bank = "audio/music/align/celt_evil.sad", .name = "MUSIC_TYPE_CELTIC_TOWN_EVIL"},
    {.bank = "audio/music/align/celt_neutral.sad", .name = "MUSIC_TYPE_CELTIC_TOWN_NEUTRAL"},
    {.bank = "audio/music/align/celt_good.sad", .name = "MUSIC_TYPE_CELTIC_TOWN_GOOD"},
    {.bank = "audio/music/align/aztc_evil.sad", .name = "MUSIC_TYPE_AZTEC_TOWN_EVIL"},
    {.bank = "audio/music/align/aztc_neutral.sad", .name = "MUSIC_TYPE_AZTEC_TOWN_NEUTRAL"},
    {.bank = "audio/music/align/aztc_good.sad", .name = "MUSIC_TYPE_AZTEC_TOWN_GOOD"},
    {.bank = "audio/music/align/japn_evil.sad", .name = "MUSIC_TYPE_JAPANESE_TOWN_EVIL"},
    {.bank = "audio/music/align/japn_neutral.sad", .name = "MUSIC_TYPE_JAPANESE_TOWN_NEUTRAL"},
    {.bank = "audio/music/align/japn_good.sad", .name = "MUSIC_TYPE_JAPANESE_TOWN_GOOD"},
    {.bank = "audio/music/align/indn_evil.sad", .name = "MUSIC_TYPE_INDIAN_TOWN_EVIL"},
    {.bank = "audio/music/align/indn_neutral.sad", .name = "MUSIC_TYPE_INDIAN_TOWN_NEUTRAL"},
    {.bank = "audio/music/align/indn_good.sad", .name = "MUSIC_TYPE_INDIAN_TOWN_GOOD"},
    {.bank = "audio/music/align/egpt_evil.sad", .name = "MUSIC_TYPE_EGYPTIAN_TOWN_EVIL"},
    {.bank = "audio/music/align/egpt_neutral.sad", .name = "MUSIC_TYPE_EGYPTIAN_TOWN_NEUTRAL"},
    {.bank = "audio/music/align/egpt_good.sad", .name = "MUSIC_TYPE_EGYPTIAN_TOWN_GOOD"},
    {.bank = "audio/music/align/grek_evil.sad", .name = "MUSIC_TYPE_GREEK_TOWN_EVIL"},
    {.bank = "audio/music/align/grek_neutral.sad", .name = "MUSIC_TYPE_GREEK_TOWN_NEUTRAL"},
    {.bank = "audio/music/align/grek_good.sad", .name = "MUSIC_TYPE_GREEK_TOWN_GOOD"},
    // The norse towns have no music of their own
    {.bank = "audio/music/align/celt_evil.sad", .name = "MUSIC_TYPE_NORSE_TOWN_EVIL"},
    {.bank = "audio/music/align/celt_neutral.sad", .name = "MUSIC_TYPE_NORSE_TOWN_NEUTRAL"},
    {.bank = "audio/music/align/celt_good.sad", .name = "MUSIC_TYPE_NORSE_TOWN_GOOD"},
    {.bank = "audio/music/align/tbtn_evil.sad", .name = "MUSIC_TYPE_TIBETAN_TOWN_EVIL"},
    {.bank = "audio/music/align/tbtn_neutral.sad", .name = "MUSIC_TYPE_TIBETAN_TOWN_NEUTRAL"},
    {.bank = "audio/music/align/tbtn_good.sad", .name = "MUSIC_TYPE_TIBETAN_TOWN_GOOD"},
    {.bank = "audio/music/chant/celt_chant.sad", .name = "MUSIC_TYPE_CELTIC_CHANT"},
    {.bank = "audio/music/chant/celt_chant_vox.sad", .name = "MUSIC_TYPE_CELTIC_CHANT_VOX"},
    {.bank = "audio/music/chant/aztc_chant.sad", .name = "MUSIC_TYPE_AZTEC_CHANT"},
    {.bank = "audio/music/chant/aztc_chant_vox.sad", .name = "MUSIC_TYPE_AZTEC_CHANT_VOX"},
    {.bank = "audio/music/chant/japn_chant.sad", .name = "MUSIC_TYPE_JAPANESE_CHANT"},
    {.bank = "audio/music/chant/japn_chant_vox.sad", .name = "MUSIC_TYPE_JAPANESE_CHANT_VOX"},
    {.bank = "audio/music/chant/indn_chant.sad", .name = "MUSIC_TYPE_INDIAN_CHANT"},
    {.bank = "audio/music/chant/indn_chant_vox.sad", .name = "MUSIC_TYPE_INDIAN_CHANT_VOX"},
    {.bank = "audio/music/chant/egpt_chant.sad", .name = "MUSIC_TYPE_EGYPTIAN_CHANT"},
    {.bank = "audio/music/chant/egpt_chant_vox.sad", .name = "MUSIC_TYPE_EGYPTIAN_CHANT_VOX"},
    {.bank = "audio/music/chant/grek_chant.sad", .name = "MUSIC_TYPE_GREEK_CHANT"},
    {.bank = "audio/music/chant/grek_chant_vox.sad", .name = "MUSIC_TYPE_GREEK_CHANT_VOX"},
    {.bank = "audio/music/chant/nrse_chant.sad", .name = "MUSIC_TYPE_NORSE_CHANT"},
    {.bank = "audio/music/chant/nrse_chant_vox.sad", .name = "MUSIC_TYPE_NORSE_CHANT_VOX"},
    {.bank = "audio/music/chant/tbtn_chant.sad", .name = "MUSIC_TYPE_TIBETAN_CHANT"},
    {.bank = "audio/music/chant/tbtn_chant_vox.sad", .name = "MUSIC_TYPE_TIBETAN_CHANT_VOX"},
    {.bank = "audio/music/citadel/citadel.sad", .name = "MUSIC_TYPE_CITADEL_EVIL"},
    {.bank = "audio/music/citadel/citadel.sad", .name = "MUSIC_TYPE_CITADEL_NEUTRAL"},
    {.bank = "audio/music/citadel/citadel.sad", .name = "MUSIC_TYPE_CITADEL_GOOD"},
    {.bank = "audio/music/script/pipertune_m.sad", .name = "MUSIC_TYPE_SCRIPT_PIPER_TUNE"},
    {.bank = "audio/music/script/pipercave_m.sad", .name = "MUSIC_TYPE_SCRIPT_PIPER_CAVE_TUNE"},
    {.bank = "audio/music/script/Hermit.sad", .name = "MUSIC_TYPE_SCRIPT_HERMIT"},
    {.bank = "audio/music/script/MissionariesBackground.sad", .name = "MUSIC_TYPE_SCRIPT_MISSIONARIES_BACKGROUND"},
    {.bank = "audio/dialogue/MissionariesVerse1.sad", .name = "MUSIC_TYPE_SCRIPT_MISSIONARIES_VERSE_1"},
    {.bank = "audio/dialogue/MissionariesVerse2.sad", .name = "MUSIC_TYPE_SCRIPT_MISSIONARIES_VERSE_2"},
    {.bank = "audio/dialogue/MissionariesVerse3.sad", .name = "MUSIC_TYPE_SCRIPT_MISSIONARIES_VERSE_3"},
    {.bank = "audio/music/intro/intro.sad", .name = "MUSIC_TYPE_SCRIPT_INTRO"},
    {.bank = "audio/music/script/singingstonesa.sad", .name = "MUSIC_TYPE_SCRIPT_SINGING_STONE_CIRCLE"},
    {.bank = "audio/music/script/FollowUsWelcome.sad", .name = "MUSIC_TYPE_SCRIPT_WELCOME_DANCE"},
    {.bank = "audio/music/script/Script01.sad", .name = "MUSIC_TYPE_SCRIPT_GENERIC_01"},
    {.bank = "audio/music/script/Script02.sad", .name = "MUSIC_TYPE_SCRIPT_GENERIC_02"},
    {.bank = "audio/music/script/Script03.sad", .name = "MUSIC_TYPE_SCRIPT_GENERIC_03"},
    {.bank = "audio/music/script/Script04.sad", .name = "MUSIC_TYPE_SCRIPT_GENERIC_04"},
    {.bank = "audio/music/script/Epic01.sad", .name = "MUSIC_TYPE_SCRIPT_EPIC_01"},
    {.bank = "audio/music/script/Epic02.sad", .name = "MUSIC_TYPE_SCRIPT_EPIC_02"},
    {.bank = "audio/music/script/Epic03.sad", .name = "MUSIC_TYPE_SCRIPT_EPIC_03"},
    {.bank = "audio/music/script/Epic04.sad", .name = "MUSIC_TYPE_SCRIPT_EPIC_04"},
    {.bank = "audio/music/script/CreatureChosen.sad", .name = "MUSIC_TYPE_SCRIPT_CREATURE_CHOSEN"},
    {.bank = "audio/music/script/Funeral.sad", .name = "MUSIC_TYPE_SCRIPT_FUNERAL"},
    {.bank = "audio/music/script/CreatureGuide.sad", .name = "MUSIC_TYPE_SCRIPT_CREATURE_GUIDE"},
    {.bank = "audio/music/script/Khazar.sad", .name = "MUSIC_TYPE_SCRIPT_KHAZAR"},
    {.bank = "audio/music/script/Nemesis.sad", .name = "MUSIC_TYPE_SCRIPT_NEMESIS"},
    {.bank = "audio/music/script/Twinkle.sad", .name = "MUSIC_TYPE_SCRIPT_TWINKLE"},
    {.bank = "audio/music/script/WhistleFuneral.sad", .name = "MUSIC_TYPE_SCRIPT_WHISTLE_FUNERAL"},
    {.bank = "audio/music/script/WhistleTwinkle.sad", .name = "MUSIC_TYPE_SCRIPT_WHISTLE_TWINKLE"},
    {.bank = "audio/music/script/Sleg.sad", .name = "MUSIC_TYPE_SCRIPT_SLEG"},
    {.bank = "audio/music/script/creaturefight.sad", .name = "MUSIC_TYPE_CREATURE_FIGHT"},
    {.bank = "audio/music/script/creatureBigfight.sad", .name = "MUSIC_TYPE_CREATURE_BIG_FIGHT"},
    {.bank = "audio/music/script/guardianstone.sad", .name = "MUSIC_TYPE_SCRIPT_GUARDIAN_STONE"},
    {.bank = "audio/music/outro/outro.sad", .name = "MUSIC_TYPE_OUTRO"},
    {.bank = "audio/music/script/failure.sad", .name = "MUSIC_TYPE_SCRIPT_FAILURE"},
    {.bank = "audio/music/script/gregorian.sad", .name = "MUSIC_TYPE_SCRIPT_GREGORIAN"},
    {.bank = "audio/music/script/christmas.sad", .name = "MUSIC_TYPE_SCRIPT_CHRISTMAS"},
    {.bank = "audio/music/script/gregorian3d.sad", .name = "MUSIC_TYPE_SCRIPT_GREGORIAN_3D"},
    {.bank = "audio/music/script/circus.sad", .name = "MUSIC_TYPE_SCRIPT_CIRCUS"},
    {.bank = "audio/music/script/circus3d.sad", .name = "MUSIC_TYPE_SCRIPT_CIRCUS_3D"},
    {.bank = "audio/music/script/creatureendsequence.sad", .name = "MUSIC_TYPE_SCRIPT_CREATURE_END_SEQUENCE"},
}};

// The alignment in seven steps, 0 (evil) to 6 (good), which the music tables group into evil, neutral
// and good
constexpr std::array<int32_t, 7> k_AlignmentIndex = {0, 0, 1, 1, 1, 2, 2};
constexpr float k_DiscreteAlignmentSteps = 6.9999995f;
constexpr int32_t k_MaxDiscreteAlignment = 6;
// The evil music of each tribe's towns. The african towns have the celtic music.
constexpr std::array<MusicType, 9> k_TribeTownMusic = {
    MusicType::CelticTownEvil,   MusicType::CelticTownEvil, MusicType::AztecTownEvil,
    MusicType::JapaneseTownEvil, MusicType::IndianTownEvil, MusicType::EgyptianTownEvil,
    MusicType::GreekTownEvil,    MusicType::NorseTownEvil,  MusicType::TibetanTownEvil,
};

// The town trigger distances in info.dat: a town's music starts within 300 of the camera and carries on to
// 400. Above 400 from the land there is no town music.
constexpr float k_TownTriggerDistance = 300.0f;
constexpr float k_TownTriggerOffDistance = 400.0f;
// The land's music only starts once the game has been going a little while
constexpr uint32_t k_LandMusicFirstTurn = 20;
// The land's music played to its end only comes back after this many turns away
constexpr uint32_t k_BlockedTurns = 3500;

constexpr int32_t k_FullVolume = 127;
constexpr int32_t k_LandVolume = 80;
// A music group carries on two chunks past where it was when it stopped, beyond the fade
constexpr uint32_t k_ResumeAhead = 2;

MusicType Offset(MusicType type, int32_t offset)
{
	return static_cast<MusicType>(static_cast<int32_t>(type) + offset);
}
} // namespace

std::string_view openblack::audio::GetMusicBankPath(MusicType type)
{
	const auto index = static_cast<size_t>(type);
	return index < k_MusicTypes.size() ? k_MusicTypes.at(index).bank : std::string_view {};
}

std::string_view openblack::audio::GetMusicTypeName(MusicType type)
{
	const auto index = static_cast<size_t>(type);
	return index < k_MusicTypes.size() ? k_MusicTypes.at(index).name : std::string_view {};
}

int32_t GameMusic::GetAlignmentIndex(float alignment)
{
	const auto discrete = static_cast<int32_t>(
	    std::min((alignment + 1.0f) / 2.0f * k_DiscreteAlignmentSteps, static_cast<float>(k_MaxDiscreteAlignment)));
	return k_AlignmentIndex.at(static_cast<size_t>(std::clamp(discrete, 0, k_MaxDiscreteAlignment)));
}

GameMusic::~GameMusic()
{
	if (Locator::audio::has_value())
	{
		Locator::audio::value().MusicStop(false);
	}
}

void GameMusic::Reset()
{
	Locator::audio::value().MusicStop(false);
	_playing = MusicType::None;
	_landType = MusicType::None;
	_blockedType = MusicType::None;
	_blockedTurns = 0;
	_scriptType = MusicType::None;
	_scriptStarted = false;
	_alignmentMusicEnabled = true;
	_rememberedTown.reset();
	_resumeChunks.clear();
}

void GameMusic::StartScriptMusic(MusicType type)
{
	_scriptType = type;
}

void GameMusic::ProcessTurn(const TurnInputs& inputs)
{
	const auto before = _playing;
	ProcessMusic(inputs);
	if (_playing != before)
	{
		SPDLOG_LOGGER_INFO(spdlog::get("audio"), "Music Playing {}", GetMusicTypeName(_playing));
	}
}

void GameMusic::ProcessMusic(const TurnInputs& inputs)
{
	if (ProcessCitadel(inputs) || ProcessScript())
	{
		_landType = MusicType::None;
		return;
	}
	if (ProcessLand(inputs))
	{
		return;
	}
	SaveResumeChunks();
	Locator::audio::value().MusicStop(true);
	_playing = MusicType::None;
	_landType = MusicType::None;
}

// Inside the citadel its music plays, of the player's alignment, carrying on from where
// it was
bool GameMusic::ProcessCitadel(const TurnInputs& inputs)
{
	if (!inputs.inCitadel)
	{
		return false;
	}
	const auto type = Offset(MusicType::CitadelEvil, GetAlignmentIndex(inputs.alignment));
	const auto path = std::string(GetMusicBankPath(type));
	auto& audio = Locator::audio::value();
	const auto info = audio.GetMusicBankInfo(path);
	if (!info)
	{
		return false;
	}
	SaveResumeChunks();
	audio.MusicPlay(path, MusicPlayOptions {
	                          .volume = k_FullVolume,
	                          .startChunk = GetResumeChunk(info->groupId),
	                          .sync = true,
	                          .fadeIn = true,
	                      });
	_playing = type;
	return true;
}

// Music a script has started plays from its start until the script stops it
bool GameMusic::ProcessScript()
{
	auto& audio = Locator::audio::value();
	if (_scriptType == MusicType::None)
	{
		if (_scriptStarted)
		{
			_scriptStarted = false;
			audio.MusicStop(true);
			_landType = MusicType::None;
		}
		return false;
	}
	if (_scriptStarted)
	{
		return true;
	}
	const auto path = std::string(GetMusicBankPath(_scriptType));
	if (path.empty() || !audio.GetMusicBankInfo(path))
	{
		return false;
	}
	SaveResumeChunks();
	audio.MusicPlay(path, MusicPlayOptions {.volume = k_FullVolume, .startChunk = 1});
	_scriptStarted = true;
	_playing = _scriptType;
	return true;
}

// The alignment music: the music of the land under the camera.
// Changing to another piece of the same music group carries on in time. A piece that plays to its end is left alone
// until the camera has been elsewhere for a while.
bool GameMusic::ProcessLand(const TurnInputs& inputs)
{
	if (!_alignmentMusicEnabled || inputs.turn <= k_LandMusicFirstTurn)
	{
		return false;
	}
	const auto type = SelectLandType(inputs);
	if (type == MusicType::None)
	{
		return false;
	}
	if (type == _blockedType && ++_blockedTurns <= k_BlockedTurns)
	{
		return false;
	}
	_blockedType = MusicType::None;

	auto& audio = Locator::audio::value();
	const auto path = std::string(GetMusicBankPath(type));
	const auto info = audio.GetMusicBankInfo(path);
	if (!info)
	{
		return false;
	}
	if (_landType != type)
	{
		SaveResumeChunks();
		const auto groupId = info->groupId;
		const auto start = GetResumeChunk(groupId);
		audio.MusicPlay(path, MusicPlayOptions {
		                          .volume = k_LandVolume,
		                          .startChunk = start,
		                          // Starting past half way, the piece plays again from its start
		                          .loops = start >= info->chunkCount / 2 ? 1 : 0,
		                          .sync = true,
		                          .fadeIn = true,
		                          .onFinished =
		                              [this, groupId]() {
			                              // Played to its end: it starts from its beginning next time, but not for a while
			                              _resumeChunks[groupId] = 1;
			                              _blockedType = _landType;
			                              _blockedTurns = 0;
			                              _landType = MusicType::None;
		                              },
		                      });
		_landType = type;
	}
	_playing = type;
	return true;
}

// The music of the nearest town's tribe within 300 of the camera, or of the town last heard while still
// within 400, otherwise the player's alignment music. Too high above the land there are no towns to hear.
MusicType GameMusic::SelectLandType(const TurnInputs& inputs)
{
	const auto alignmentIndex = GetAlignmentIndex(inputs.alignment);
	const auto distance = [&inputs](const Town& town) { return glm::distance(glm::xz(inputs.camera), glm::xz(town.position)); };
	const auto townMusic = [alignmentIndex](const Town& town) {
		if (town.tribe < 0 || static_cast<size_t>(town.tribe) >= k_TribeTownMusic.size())
		{
			return Offset(MusicType::GenericEvil, alignmentIndex);
		}
		return Offset(k_TribeTownMusic.at(static_cast<size_t>(town.tribe)), alignmentIndex);
	};

	const Town* nearest = nullptr;
	auto nearestDistance = std::numeric_limits<float>::max();
	const Town* remembered = nullptr;
	for (const auto& town : inputs.towns)
	{
		const auto d = distance(town);
		if (d <= k_TownTriggerOffDistance && d < nearestDistance)
		{
			nearest = &town;
			nearestDistance = d;
		}
		if (_rememberedTown && town.id == *_rememberedTown)
		{
			remembered = &town;
		}
	}
	if (remembered == nullptr)
	{
		_rememberedTown.reset();
	}

	if (nearest != nullptr && inputs.camera.y - inputs.groundHeight < k_TownTriggerOffDistance)
	{
		if (nearestDistance <= k_TownTriggerDistance)
		{
			_rememberedTown = nearest->id;
			return townMusic(*nearest);
		}
		if (remembered != nullptr && remembered != nearest && distance(*remembered) < k_TownTriggerOffDistance)
		{
			return townMusic(*remembered);
		}
	}
	_rememberedTown.reset();
	return Offset(MusicType::GenericEvil, alignmentIndex);
}

void GameMusic::SaveResumeChunks()
{
	const auto* music = Locator::audio::value().GetMusic();
	if (music == nullptr)
	{
		return;
	}
	for (const auto& channel : music->GetChannels())
	{
		if (channel.active && channel.groupId > 0)
		{
			_resumeChunks[channel.groupId] = channel.playingChunk + k_ResumeAhead;
		}
	}
}

uint32_t GameMusic::GetResumeChunk(int32_t groupId) const
{
	const auto found = _resumeChunks.find(groupId);
	return found != _resumeChunks.end() ? found->second : 1;
}
