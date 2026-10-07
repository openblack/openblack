/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>

#include "AudioManagerInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp", use interface instead.
#endif

namespace openblack::audio
{

class AudioManagerNoOp final: public AudioManagerInterface
{
public:
	BufferId CreateBuffer([[maybe_unused]] ChannelLayout layout, [[maybe_unused]] const std::vector<int16_t>& buffer,
	                      [[maybe_unused]] int sampleRate) override
	{
		return 0;
	}
	void CreateBuffer([[maybe_unused]] Sound& sound) override {}
	void PlayEmitter([[maybe_unused]] entt::entity emitter) override {}
	void PauseEmitter([[maybe_unused]] entt::entity emitter) override {}
	void StopEmitter([[maybe_unused]] entt::entity emitter) override {}
	void DestroyEmitter([[maybe_unused]] entt::entity emitter) override {}
	entt::entity CreateEmitter([[maybe_unused]] entt::id_type id, [[maybe_unused]] std::optional<glm::vec3> worldPosition,
	                           [[maybe_unused]] PlayType playType) override
	{
		return entt::null;
	};
	[[nodiscard]] bool EmitterExists([[maybe_unused]] entt::entity emitter) override { return false; }
	[[nodiscard]] float GetProgress([[maybe_unused]] entt::entity emitter) override { return 1.0f; }
	[[nodiscard]] AudioStatus GetStatus([[maybe_unused]] entt::entity emitter) override { return {}; }
	void PlayMusic([[maybe_unused]] const std::string& packPath, [[maybe_unused]] PlayType type) override {}
	void StopMusic() override {}
	bool MusicPlay([[maybe_unused]] const std::string& bankPath, [[maybe_unused]] const MusicPlayOptions& options) override
	{
		return false;
	}
	void MusicStop([[maybe_unused]] bool fadeOut) override {}
	[[nodiscard]] bool MusicIsActive() const override { return false; }
	[[nodiscard]] std::optional<MusicBankInfo> GetMusicBankInfo([[maybe_unused]] const std::string& bankPath) override
	{
		return std::nullopt;
	}
	[[nodiscard]] const MusicPlayer* GetMusic() const override { return nullptr; }
	const Sound& GetSound([[maybe_unused]] entt::id_type id) override
	{
		static const Sound result {};
		return result;
	}
	void PlaySound([[maybe_unused]] entt::id_type id, [[maybe_unused]] PlayType type) override {}
	void AddAnimEffects([[maybe_unused]] const std::string& bankName, [[maybe_unused]] AnimEffectTable table) override {}
	AnimEffectPlay PlayAnimEffect([[maybe_unused]] const std::string& bankName, [[maybe_unused]] std::span<const int32_t> keys,
	                              [[maybe_unused]] entt::entity owner, [[maybe_unused]] const glm::vec3& position) override
	{
		return {};
	}
	void PlaySoundEffect([[maybe_unused]] entt::id_type id, [[maybe_unused]] std::optional<glm::vec3> worldPosition) override {}
	void StopSoundEffect([[maybe_unused]] entt::id_type id) override {}
	entt::entity StartSoundEffect([[maybe_unused]] entt::id_type id,
	                              [[maybe_unused]] const SoundEffectOptions& options) override
	{
		return entt::null;
	}
	void SetEmitterPosition([[maybe_unused]] entt::entity emitter, [[maybe_unused]] const glm::vec3& position) override {}
	void ReleaseEmitterLoop([[maybe_unused]] entt::entity emitter) override {}
	[[nodiscard]] bool IsEmitterLooping([[maybe_unused]] entt::entity emitter) override { return false; }
	void SetEmitterVolume([[maybe_unused]] entt::entity emitter, [[maybe_unused]] uint32_t volume) override {}
	[[nodiscard]] uint32_t GetEmitterVolume([[maybe_unused]] entt::entity emitter) override { return 0; }
	void StopOwnedSounds([[maybe_unused]] entt::entity owner) override {}
	void SetGlobalVolume([[maybe_unused]] float volume) override {}
	void SetSfxVolume([[maybe_unused]] float volume) override {}
	void SetMusicVolume([[maybe_unused]] float volume) override {}
	[[nodiscard]] float GetGlobalVolume() override { return 0.0f; }
	[[nodiscard]] float GetSfxVolume() override { return 0.0f; }
	[[nodiscard]] float GetMusicVolume() override { return 0.0f; }
	void Stop() override {}
	void Update() override {}
	void CreateSoundGroup([[maybe_unused]] const std::string& name) override {}
	void AddMusicEntry([[maybe_unused]] const std::string& name) override {}
	[[nodiscard]] const std::vector<std::string>& GetMusicTracks() const override
	{
		static const std::vector<std::string> result;
		return result;
	}
	void AddToSoundGroup([[maybe_unused]] const std::string& name, [[maybe_unused]] entt::id_type id) override {}
	const SoundGroup& GetSoundGroup([[maybe_unused]] const std::string& name) override
	{
		static const SoundGroup result;
		return result;
	}
	const std::map<std::string, SoundGroup>& GetSoundGroups() override
	{
		static const std::map<std::string, SoundGroup> result;
		return result;
	}
	uint32_t AtmosRegisterBank([[maybe_unused]] const std::string& bankName,
	                           [[maybe_unused]] const std::vector<pack::AudioBankSampleHeader>& headers,
	                           [[maybe_unused]] uint16_t atmosCount) override
	{
		return 0;
	}
	void AtmosReleaseBank([[maybe_unused]] uint32_t bank) override {}
	void AtmosSetBankVolume([[maybe_unused]] uint32_t bank, [[maybe_unused]] int32_t volume) override {}
	void AtmosSetGroup([[maybe_unused]] uint32_t bank, [[maybe_unused]] uint32_t group) override {}
	void AtmosProcess([[maybe_unused]] bool active) override {}
	[[nodiscard]] const AtmosPlayer* GetAtmos() const override { return nullptr; }
};

} // namespace openblack::audio
