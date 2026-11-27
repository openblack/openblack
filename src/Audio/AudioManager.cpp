/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "AudioManager.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <string_view>

#include <PackFile.h>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>

#include "AudioPlayerInterface.h"
#include "Camera/Camera.h"
#include "Common/StringUtils.h"
#include "ECS/Registry.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"
#include "MpegAudioDecoder.h"
#include "Resources/Resources.h"
#include "WavAudioDecoder.h"

using namespace openblack::ecs::components;

namespace openblack::audio
{

AudioManager::AudioManager()
    : _audioPlayer(new AudioPlayer())
{
	_audioPlayer->Initialize();
}

AudioManager::~AudioManager() = default;

void AudioManager::Stop()
{
	StopMusic();
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<Transform, AudioEmitter>(
	    [this](entt::entity entity, const Transform&, const AudioEmitter&) { DestroyEmitter(entity); });
	auto& sounds = Locator::resources::value().GetSounds();
	sounds.Each([&](entt::id_type id, const auto&) {
		auto sound = sounds.Handle(id);
		if (sound->bufferId != 0)
		{
			_audioPlayer->DeleteBuffer(sound->bufferId);
			sound->bufferId = 0;
		}
	});
}

void AudioManager::Update()
{
	auto& camera = Locator::camera::value();
	auto pos = camera.GetOrigin();
	auto vel = camera.GetOriginVelocity();
	auto forward = camera.GetForward();
	auto top = camera.GetUp();
	_audioPlayer->UpdateListener(pos, vel, forward, top);
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<Transform, AudioEmitter>(
	    [this](entt::entity entity, const Transform& transform, const AudioEmitter& emitter) {
		    auto volume = _globalVolume * emitter.volume;
		    if (entity == _musicEntity)
		    {
			    volume *= _musicVolume;
		    }
		    else
		    {
			    volume *= _sfxVolume;
		    }
		    _audioPlayer->UpdateSource(emitter.sourceId, transform.position, volume, emitter.loop == PlayType::Repeat);
		    auto audioStatus = _audioPlayer->GetStatus(emitter.sourceId);
		    if (audioStatus == AudioStatus::Stopped)
		    {
			    DestroyEmitter(entity);
		    }
	    });
}

BufferId AudioManager::CreateBuffer(ChannelLayout layout, const std::vector<int16_t>& buffer, int sampleRate)
{
	return _audioPlayer->CreateBuffer(layout, buffer, sampleRate);
}

void AudioManager::PlayEmitter(entt::entity emitter)
{
	auto& registry = Locator::entitiesRegistry::value();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& emitterComponent = registry.Get<AudioEmitter>(emitter);
	auto& transform = registry.Get<Transform>(emitter);
	const float volume = _globalVolume * (emitter == _musicEntity ? _musicVolume : _sfxVolume) * emitterComponent.volume;
	_audioPlayer->PlaySource(emitterComponent.sourceId, transform.position, volume, emitterComponent.loop == PlayType::Repeat);
}

void AudioManager::PauseEmitter(entt::entity emitter)
{
	auto& registry = Locator::entitiesRegistry::value();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	_audioPlayer->PauseSource(component.sourceId);
}

void AudioManager::StopEmitter(entt::entity emitter)
{
	auto& registry = Locator::entitiesRegistry::value();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	_audioPlayer->StopSource(component.sourceId);
}

void AudioManager::DestroyEmitter(entt::entity emitter)
{
	auto& registry = Locator::entitiesRegistry::value();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	_audioPlayer->DeleteSource(component.sourceId);
	registry.Destroy(emitter);
}

entt::entity AudioManager::CreateEmitter(entt::id_type id, PlayType playType, glm::vec3 position, glm::vec3 direction,
                                         glm::vec2 radius, float volume, AudioStatus status, bool relative)
{
	auto sound = Locator::resources::value().GetSounds().Handle(id);
	if (sound->bufferId == 0)
	{
		CreateBuffer(sound);
	}
	if (sound->bufferId == 0)
	{
		return entt::null;
	}
	const auto sourceId = _audioPlayer->CreateSource(static_cast<float>(sound->pitch) / 100.0f, relative);
	if (sourceId == 0)
	{
		return entt::null;
	}
	_audioPlayer->QueueBuffer(sourceId, sound->bufferId);
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	registry.Assign<AudioEmitter>(entity, sourceId, id, 0, position, direction, radius, volume, playType, status, relative);
	registry.Assign<Transform>(entity, position, glm::mat3(1.0f), glm::one<glm::vec3>());
	return entity;
}

void AudioManager::CreateBuffer(Sound& sound)
{
	std::vector<int16_t> decodeBuffer;
	for (auto& buffer : sound.buffer)
	{
		bool success;
		std::vector<int16_t> decoded;
		{
			auto decoder = audio::WavAudioDecoder();
			success = decoder.Open(buffer);
			if (success)
			{
				decoder.Read(decoded);
				sound.channelLayout = decoder.GetChannelLayout();
			}
		}
		if (!success)
		{
			auto decoder = audio::MpegAudioDecoder();
			success = decoder.Open(buffer);
			if (success)
			{
				decoder.Read(decoded);
				sound.channelLayout = decoder.GetChannelLayout();
			}
		}
		if (success)
		{
			decodeBuffer.insert(decodeBuffer.end(), decoded.begin(), decoded.end());
		}
		else
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("audio"), "Unable to decode sound");
		}
	}
	if (decodeBuffer.empty() || sound.sampleRate <= 0)
	{
		return;
	}
	sound.bufferId = CreateBuffer(sound.channelLayout, decodeBuffer, sound.sampleRate);
	if (sound.bufferId == 0)
	{
		return;
	}
	sound.duration = _audioPlayer->GetDuration(sound.bufferId);
	sound.sizeInBytes = decodeBuffer.size() * sizeof(decodeBuffer[0]);
}

bool AudioManager::EmitterExists(entt::entity emitter)
{
	auto& registry = Locator::entitiesRegistry::value();
	return registry.Valid(emitter) && registry.AnyOf<AudioEmitter>(emitter);
}

float AudioManager::GetProgress(entt::entity entity)
{
	auto& registry = Locator::entitiesRegistry::value();
	assert(registry.AnyOf<AudioEmitter>(entity));
	auto& emitter = registry.Get<AudioEmitter>(entity);
	auto sizeInBytes = Locator::resources::value().GetSounds().Handle(emitter.soundId)->sizeInBytes;
	return _audioPlayer->GetProgress(sizeInBytes, emitter.sourceId);
}

AudioStatus AudioManager::GetStatus(entt::entity emitter)
{
	auto& registry = Locator::entitiesRegistry::value();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	return _audioPlayer->GetStatus(component.sourceId);
}

const Sound& AudioManager::GetSound(entt::id_type id)
{
	return Locator::resources::value().GetSounds().Handle(id);
}

void AudioManager::PlaySound(entt::id_type id, PlayType playType)
{
	auto position = glm::zero<glm::vec3>();
	auto direction = glm::zero<glm::vec3>();
	auto radius = glm::zero<glm::vec3>();
	auto sound = Locator::resources::value().GetSounds().Handle(id);
	auto entity = CreateEmitter(id, playType, position, direction, radius, sound->volume, AudioStatus::Playing, true);
	if (entity != entt::null)
	{
		PlayEmitter(entity);
	}
}

void AudioManager::PlayScriptSound(uint32_t bank, uint32_t sample, std::optional<glm::vec3> position)
{
	static constexpr std::array<std::string_view, 11> k_Banks = {"",
	                                                             "ingame.sad",
	                                                             "editor.sad",
	                                                             "spells.sad",
	                                                             "creature.sad",
	                                                             "scriptsfx.sad",
	                                                             "helpsprites.sad",
	                                                             "villagers.sad",
	                                                             "villagersbanter.sad",
	                                                             "spelldialogue.sad",
	                                                             "guidance.sad"};
	if (bank == 0 || bank >= k_Banks.size() || sample == 0)
	{
		return;
	}
	const auto bankName = k_Banks.at(bank);
	const auto group = std::ranges::find_if(
	    _soundGroups, [bankName](const auto& entry) { return string_utils::LowerCase(entry.first) == bankName; });
	if (group == _soundGroups.end())
	{
		return;
	}
	const auto id = resources::HashIdentifier(fmt::format("{}/{}", group->first, sample));
	const auto& sounds = Locator::resources::value().GetSounds();
	if (!sounds.Contains(id))
	{
		return;
	}
	const auto sound = sounds.Handle(id);
	auto& registry = Locator::entitiesRegistry::value();
	if (!sound->allowOverlap)
	{
		entt::entity previous = entt::null;
		registry.Each<AudioEmitter>([&](entt::entity entity, const AudioEmitter& emitter) {
			if (emitter.scriptEffect && emitter.soundId == id)
			{
				previous = entity;
			}
		});
		if (previous != entt::null)
		{
			if (!sound->restartOnPlay && GetStatus(previous) == AudioStatus::Playing)
			{
				return;
			}
			DestroyEmitter(previous);
		}
	}
	const auto entity = CreateEmitter(id, PlayType::Once, position.value_or(glm::vec3(0)), {}, {}, sound->volume,
	                                  AudioStatus::Playing, !position);
	if (entity != entt::null)
	{
		registry.Get<AudioEmitter>(entity).scriptEffect = true;
		PlayEmitter(entity);
	}
}

void AudioManager::CreateSoundGroup(const std::string& name)
{
	_soundGroups[name] = SoundGroup();
}

void AudioManager::AddMusicEntry(const std::string& name)
{
	_music.emplace_back(name);
}

void AudioManager::AddToSoundGroup(const std::string& name, entt::id_type id)
{
	_soundGroups[name].sounds.emplace_back(id);
}

const SoundGroup& AudioManager::GetSoundGroup(const std::string& name)
{
	return _soundGroups[name];
}

const std::map<std::string, SoundGroup>& AudioManager::GetSoundGroups()
{
	return _soundGroups;
}

void AudioManager::PlayMusic(const std::string& packPath, PlayType type)
{
	StopMusic();
	const entt::id_type id = entt::hashed_string(fmt::format("{}", packPath).c_str());
	if (!Locator::resources::value().GetSounds().Contains(packPath))
	{
		pack::PackFile soundPack;
		soundPack.Open(packPath);
		const auto& audioHeaders = soundPack.GetAudioSampleHeaders();
		const auto& audioData = soundPack.GetAudioSamplesData();
		Locator::resources::value().GetSounds().Load(id, resources::SoundLoader::FromBufferTag {}, audioHeaders[0], audioData);
	}
	auto sound = Locator::resources::value().GetSounds().Handle(id);
	auto position = glm::zero<glm::vec3>();
	auto direction = glm::zero<glm::vec3>();
	auto radius = glm::zero<glm::vec3>();
	_musicEntity = CreateEmitter(id, type, position, direction, radius, sound->volume, AudioStatus::Playing, true);
	if (_musicEntity != entt::null)
	{
		PlayEmitter(_musicEntity);
	}
}

void AudioManager::StopMusic()
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!EmitterExists(_musicEntity))
	{
		return;
	}
	auto& emitter = registry.Get<AudioEmitter>(_musicEntity);
	// Clean up the audio player's music resources
	_audioPlayer->StopSource(emitter.sourceId);
	_audioPlayer->DeleteSource(emitter.sourceId);
	auto music = Locator::resources::value().GetSounds().Handle(emitter.soundId);
	_audioPlayer->DeleteBuffer(music->bufferId);
	//	Erase the music resource as it is no longer being played
	Locator::resources::value().GetSounds().Erase(emitter.soundId);
	//	Remove the entity
	registry.Destroy(_musicEntity);
	_musicEntity = entt::null;
}
} // namespace openblack::audio
