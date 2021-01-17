/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "AudioManager.h"

#include <fstream>

#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>

#include "3D/Camera.h"
#include "AudioPlayerInterface.h"
#include "Common/FileSystem.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"
#include "MpegAudioDecoder.h"
#include "Resources/Resources.h"
#include "WavAudioDecoder.h"

using namespace openblack::ecs::components;

namespace openblack::audio
{

AudioManager::AudioManager()
    : _audioPlayer(new AudioPlayer())
    , _musicVolume(1.f)
    , _sfxVolume(1.f)
    , _globalVolume(1.f)
    , _musicEntity(entt::null)
{
	_audioPlayer->Initialize();
}

AudioManager::~AudioManager()
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	registry.Each<Transform, AudioEmitter>([this](entt::entity entity, const Transform&, const AudioEmitter& emitter) {
		DestroyEmitter(entity);
		auto sound = Locator::resources::value().GetSounds().Handle(emitter.soundId);
		_audioPlayer->DeleteBuffer(sound->bufferId);
	});

	if (registry.Valid(_musicEntity))
	{
		DestroyEmitter(_musicEntity);
	}
}

void AudioManager::Stop()
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	registry.Each<Transform, AudioEmitter>(
	    [this](entt::entity entity, const Transform&, const AudioEmitter&) { DestroyEmitter(entity); });
	StopMusic();
}

void AudioManager::Update(Game& game)
{
	auto& cam = game.GetCamera();
	auto pos = cam.GetPosition();
	auto vel = cam.GetVelocity();
	auto forward = cam.GetForward();
	auto top = cam.GetUp();
	_audioPlayer->UpdateListener(pos, vel, forward, top);
	auto& registry = Game::Instance()->GetEntityRegistry();
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
	auto& registry = Game::Instance()->GetEntityRegistry();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& emitterComponent = registry.Get<AudioEmitter>(emitter);
	auto& transform = registry.Get<Transform>(emitter);
	_audioPlayer->PlaySource(emitterComponent.sourceId, transform.position, 1.f, emitterComponent.loop == PlayType::Repeat);
}

void AudioManager::PauseEmitter(entt::entity emitter)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	_audioPlayer->PauseSource(component.sourceId);
}

void AudioManager::StopEmitter(entt::entity emitter)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	_audioPlayer->StopSource(component.sourceId);
}

void AudioManager::DestroyEmitter(entt::entity emitter)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	_audioPlayer->DeleteSource(component.sourceId);
	registry.Destroy(emitter);
}

entt::entity AudioManager::CreateEmitter(entt::id_type id, PlayType playType, glm::vec3 position, glm::vec3 direction,
                                         glm::vec2 radius, float volume, AudioStatus status, bool relative)
{
	auto sound = Locator::resources::value().GetSounds().Handle(id);
	auto& registry = Game::Instance()->GetEntityRegistry();
	auto entity = registry.Create();
	auto sourceId = _audioPlayer->CreateSource(sound->pitch, relative);
	if (!sound->buffer.empty())
	{
		CreateBuffer(sound);
	}
	_audioPlayer->QueueBuffer(sourceId, sound->bufferId);
	registry.Assign<AudioEmitter>(entity, sourceId, id, 0, position, direction, radius, volume, playType, status, relative);
	registry.Assign<Transform>(entity, glm::zero<glm::vec3>(), glm::one<glm::mat4>(), glm::one<glm::vec3>());
	return entity;
}

void AudioManager::CreateBuffer(Sound& sound)
{
	std::vector<int16_t> decodeBuffer;
	for (auto& buffer : sound.buffer)
	{
		bool success = false;
		std::vector<int16_t> decoded;
		{
			auto decoder = audio::MpegAudioDecoder();
			success = decoder.Open(buffer);
			if (success)
			{
				decoder.Read(decoded);
				sound.channelLayout = decoder.GetChannelLayout();
			}
		}
		if (!success)
		{
			auto decoder = audio::WavAudioDecoder();
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
	sound.bufferId = CreateBuffer(sound.channelLayout, decodeBuffer, sound.sampleRate);
	sound.duration = _audioPlayer->GetDuration(sound.bufferId);
	sound.sizeInBytes = decodeBuffer.size() * sizeof(decodeBuffer[0]);
}

bool AudioManager::EmitterExists(entt::entity emitter)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	return registry.Valid(emitter) && registry.AnyOf<AudioEmitter>(emitter);
}

float AudioManager::GetProgress(entt::entity entity)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	assert(registry.AnyOf<AudioEmitter>(entity));
	auto& emitter = registry.Get<AudioEmitter>(entity);
	auto sizeInBytes = Locator::resources::value().GetSounds().Handle(emitter.soundId)->sizeInBytes;
	return _audioPlayer->GetProgress(sizeInBytes, emitter.sourceId);
}

AudioStatus AudioManager::GetStatus(entt::entity emitter)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	return _audioPlayer->GetStatus(component.sourceId);
}

Sound& AudioManager::GetSound(entt::id_type id)
{
	return Locator::resources::value().GetSounds().Handle(id);
}

void AudioManager::PlaySound(entt::id_type id, PlayType playType)
{
	auto position = glm::one<glm::vec3>();
	auto direction = glm::zero<glm::vec3>();
	auto radius = glm::zero<glm::vec3>();
	auto sound = Locator::resources::value().GetSounds().Handle(id);
	auto entity = CreateEmitter(id, playType, position, direction, radius, sound->volume, AudioStatus::Playing, true);
	PlayEmitter(entity);
}

void AudioManager::CreateSoundGroup(std::string& name)
{
	_soundGroups[name] = SoundGroup();
}

void AudioManager::AddMusicEntry(std::string& name)
{
	_music.emplace_back(name);
}

void AudioManager::AddToSoundGroup(std::string& name, entt::id_type id)
{
	_soundGroups[name].sounds.emplace_back(id);
}

const SoundGroup& AudioManager::GetSoundGroup(std::string& name)
{
	return _soundGroups[name];
}

const std::map<std::string, SoundGroup>& AudioManager::GetSoundGroups()
{
	return _soundGroups;
}

void AudioManager::PlayMusic(std::string& packPath, PlayType type)
{
	StopMusic();
	entt::id_type id = entt::hashed_string(fmt::format("{}", packPath).c_str());
	if (!Locator::resources::value().GetSounds().Contains(packPath))
	{
		pack::PackFile soundPack;
		soundPack.Open(packPath);
		auto& sounds = soundPack.GetSounds();
		auto combined = std::vector<std::vector<uint8_t>>();
		for (auto& sound : sounds)
		{
			combined.insert(combined.end(), sound.buffer);
		}
		Locator::resources::value().GetSounds().Load(id, resources::SoundLoader::FromBufferTag {}, sounds[0].sample, combined);
	}
	auto sound = Locator::resources::value().GetSounds().Handle(id);
	auto position = glm::one<glm::vec3>();
	auto direction = glm::zero<glm::vec3>();
	auto radius = glm::zero<glm::vec3>();
	_musicEntity = CreateEmitter(id, type, position, direction, radius, sound->volume, AudioStatus::Playing, true);
	PlayEmitter(_musicEntity);
}

void AudioManager::StopMusic()
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	if (!EmitterExists(_musicEntity))
	{
		return;
	}
	auto& emitter = registry.Get<AudioEmitter>(_musicEntity);
	// Clean up the audio player's music resources
	_audioPlayer->StopSource(emitter.sourceId);
	_audioPlayer->DeleteSource(emitter.sourceId);
	auto music = Locator::resources::value().GetSounds().Handle(emitter.soundId);
	//	Erase the music resource as it is no longer being played
	Locator::resources::value().GetSounds().Erase(emitter.soundId);
	//	Remove the entity
	registry.Destroy(_musicEntity);
	_musicEntity = entt::null;
}
} // namespace openblack::audio
