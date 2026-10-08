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
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <PackFile.h>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>

#include "AudioPlayerInterface.h"
#include "Camera/Camera.h"
#include "Common/RandomNumberManager.h"
#include "Common/StringUtils.h"
#include "ECS/Registry.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"
#include "Resources/Resources.h"
#include "SoundDecoder.h"

using namespace openblack::ecs::components;

namespace openblack::audio
{

namespace
{
// The game's default sample play options, which a bank header overrides where it says so
constexpr uint32_t k_MaxVolume = 127;
constexpr uint32_t k_DefaultPitchPercent = 100;
constexpr float k_DefaultMinDistance = 1.0f;
constexpr float k_DefaultMaxDistance = 9999.0f;
constexpr float k_DefaultDistanceScale = 0.3f;
} // namespace

AudioManager::AudioManager()
    : _audioPlayer(new AudioPlayer())
{
	_audioPlayer->Initialize();
	_atmos = std::make_unique<AtmosPlayer>(static_cast<VoiceBackend&>(*this));
	_musicStreams = std::make_unique<MusicStreamBackend>(*_audioPlayer);
	_musicPlayer = std::make_unique<MusicPlayer>(*_musicStreams);
}

AudioManager::~AudioManager()
{
	_musicPlayer.reset();
	_musicStreams.reset();
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<AudioEmitter>([this](entt::entity entity, const AudioEmitter&) { DestroyEmitter(entity); });

	// Stops the atmosphere voices before their buffers go
	_atmos.reset();
	for (const auto buffer : _decodedBuffers)
	{
		_audioPlayer->DeleteBuffer(buffer);
	}
}

namespace
{
/// The name of the sound an emitter plays, for the log
std::string EmitterSoundName(const AudioEmitter& emitter)
{
	auto& sounds = Locator::resources::value().GetSounds();
	return sounds.Contains(emitter.soundId) ? sounds.Handle(emitter.soundId)->name : fmt::format("#{}", emitter.soundId);
}

/// How an emitter is described in the log: its entity, its sound, 2D or where it is
// Only the debug and trace logging uses it, which release builds leave out
[[maybe_unused]] std::string DescribeEmitter(entt::entity entity, const AudioEmitter& emitter)
{
	return fmt::format("{} {} {}{}", static_cast<uint32_t>(entity), emitter.spatial ? "3D" : "2D", EmitterSoundName(emitter),
	                   emitter.spatial
	                       ? fmt::format(" at ({}, {}, {})", emitter.position.x, emitter.position.y, emitter.position.z)
	                       : std::string());
}

// The log lines below are left out of release builds, which would otherwise find these parameters unused
void LogEmitterStart([[maybe_unused]] entt::entity entity, [[maybe_unused]] const AudioEmitter& emitter)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Emitter {} starts: volume {} pitch {}%{}", DescribeEmitter(entity, emitter),
	                    emitter.volume, emitter.pitchPercent, emitter.loop == PlayType::Repeat ? ", looping" : "");
}

void LogNotStarted([[maybe_unused]] const Sound& sound, [[maybe_unused]] const glm::vec3& position,
                   [[maybe_unused]] std::string_view why)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Sound {} at ({}, {}, {}) not started: {}", sound.name, position.x, position.y,
	                    position.z, why);
}

std::string TooFar(const Sound& sound, const glm::vec3& position)
{
	return fmt::format("{:.1f} m from the camera, beyond its {} m",
	                   glm::distance(Locator::camera::value().GetOrigin(), position), sound.maxDistance);
}
} // namespace

void AudioManager::Stop()
{
	_musicPlayer->Stop(false);
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<AudioEmitter>([this](entt::entity entity, const AudioEmitter&) { DestroyEmitter(entity); });
	_atmos->Process(false);
}

void AudioManager::Update()
{
	auto& camera = Locator::camera::value();
	_audioPlayer->UpdateListener(camera.GetOrigin(), camera.GetForward(), camera.GetUp());

	// 3D emitters follow the listener, finished emitters are released
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<AudioEmitter>([this](entt::entity entity, AudioEmitter& emitter) {
		emitter.state = _audioPlayer->GetStatus(emitter.sourceId);
		if (emitter.state == AudioStatus::Stopped)
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Emitter {} has finished", DescribeEmitter(entity, emitter));
			DestroyEmitter(entity);
			return;
		}
		if (emitter.spatial)
		{
			PositionSource(emitter.sourceId, ToListenerFrame(emitter.position), emitter.distanceScale);
		}
		_audioPlayer->SetVolume(emitter.sourceId, emitter.gain * _globalVolume * (emitter.music ? _musicVolume : _sfxVolume));
	});

	// The music thread's work: streams are fed and fade every tick
	const auto now = std::chrono::steady_clock::now();
	_musicStreams->SetOutputVolume(_globalVolume * _musicVolume);
	_musicPlayer->Update(std::chrono::duration_cast<std::chrono::microseconds>(now - _lastMusicUpdate));
	_lastMusicUpdate = now;

	// Atmosphere voices are positioned relative to the listener and only follow the volume settings here
	for (const auto& [handle, voice] : _atmosVoices)
	{
		_audioPlayer->SetVolume(voice.source, voice.gain * _globalVolume * _sfxVolume);
	}
}

BufferId AudioManager::CreateBuffer(ChannelLayout layout, const std::vector<int16_t>& buffer, int sampleRate)
{
	return _audioPlayer->CreateBuffer(layout, buffer, sampleRate);
}

void AudioManager::PlayEmitter(entt::entity emitter)
{
	auto& registry = Locator::entitiesRegistry::value();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	_audioPlayer->SetVolume(component.sourceId, component.gain * _globalVolume * (component.music ? _musicVolume : _sfxVolume));
	_audioPlayer->StartSource(component.sourceId);
	component.state = AudioStatus::Playing;
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
	SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Emitter {} stopped", DescribeEmitter(emitter, component));
	_audioPlayer->StopSource(component.sourceId);
}

void AudioManager::DestroyEmitter(entt::entity emitter)
{
	auto& registry = Locator::entitiesRegistry::value();
	assert(registry.AnyOf<AudioEmitter>(emitter));
	auto& component = registry.Get<AudioEmitter>(emitter);
	if (component.state != AudioStatus::Stopped)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Emitter {} stopped and removed", DescribeEmitter(emitter, component));
	}
	_audioPlayer->StopSource(component.sourceId);
	_audioPlayer->DeleteSource(component.sourceId);
	registry.Destroy(emitter);
}

VoiceStart AudioManager::MakeVoiceStart(const Sound& sound, std::optional<glm::vec3> worldPosition, PlayType playType)
{
	const auto overrides = [&sound](pack::AudioBankOverride flag) {
		return (sound.overrideFlags & static_cast<uint32_t>(flag)) != 0;
	};

	// Playback rate in percent with a random deviation of up to pitchDeviation percent either way
	auto pitch = overrides(pack::AudioBankOverride::Pitch) && sound.pitch != 0 ? static_cast<uint32_t>(sound.pitch)
	                                                                           : k_DefaultPitchPercent;
	const auto deviation = (pitch * static_cast<uint32_t>(sound.pitchDeviation)) / 100;
	if (deviation != 0)
	{
		pitch = pitch - deviation + Locator::rng::value().NextValue(0u, deviation * 2);
	}

	int32_t loopCount = overrides(pack::AudioBankOverride::Loop) ? sound.loop : 0;
	if (playType == PlayType::Repeat)
	{
		loopCount = -1;
	}

	return VoiceStart {
	    .bankName = {},
	    .sampleId = sound.id,
	    .volume =
	        overrides(pack::AudioBankOverride::Volume) ? std::min<uint32_t>(sound.headerVolume, k_MaxVolume) : k_MaxVolume,
	    .pitchPercent = pitch,
	    .pitch = static_cast<float>(pitch) / 100.0f,
	    .loopCount = loopCount,
	    .positional = worldPosition.has_value(),
	    .position = worldPosition ? ToListenerFrame(*worldPosition) : glm::zero<glm::vec3>(),
	    .minDistance = overrides(pack::AudioBankOverride::MinDist) ? sound.minDistance : k_DefaultMinDistance,
	    .maxDistance = overrides(pack::AudioBankOverride::MaxDist) ? sound.maxDistance : k_DefaultMaxDistance,
	    .distanceScale = overrides(pack::AudioBankOverride::Scale) ? sound.distanceScale : k_DefaultDistanceScale,
	};
}

entt::entity AudioManager::CreateEmitter(entt::id_type id, std::optional<glm::vec3> worldPosition, PlayType playType)
{
	auto& sounds = Locator::resources::value().GetSounds();
	if (!sounds.Contains(id))
	{
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Sound {} is not loaded", id);
		return entt::null;
	}
	auto sound = sounds.Handle(id);
	const auto start = MakeVoiceStart(*sound, worldPosition, playType);
	const auto source = CreateSource(*sound, start);

	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	registry.Assign<AudioEmitter>(entity, AudioEmitter {
	                                          .sourceId = source,
	                                          .soundId = id,
	                                          .priority = sound->priority,
	                                          .spatial = worldPosition.has_value(),
	                                          .position = worldPosition.value_or(glm::zero<glm::vec3>()),
	                                          .gain = _atmos->VolumeToGain(start.volume),
	                                          .volume = start.volume,
	                                          .pitchPercent = start.pitchPercent,
	                                          .minDistance = start.minDistance,
	                                          .maxDistance = start.maxDistance,
	                                          .distanceScale = start.distanceScale,
	                                          .loop = start.loopCount < 0 ? PlayType::Repeat : PlayType::Once,
	                                          .state = AudioStatus::Initial,
	                                          .music = false,
	                                      });
	LogEmitterStart(entity, registry.Get<AudioEmitter>(entity));
	return entity;
}

void AudioManager::CreateBuffer(Sound& sound)
{
	std::vector<int16_t> decodeBuffer;
	auto sampleRate = sound.sampleRate;
	for (size_t i = 0; i < sound.buffer.size(); ++i)
	{
		const auto result = DecodeSound(sound.buffer[i], sound.sampleRate);
		const auto part = sound.buffer.size() > 1 ? fmt::format(" part {}", i) : std::string();
		if (!result.sound)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("audio"), "Unable to decode sound {}{} ({}): {}", sound.name, part,
			                    ToString(result.container), result.error);
			continue;
		}
		for (const auto& warning : result.warnings)
		{
			SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Sound {}{} ({}): {}", sound.name, part, ToString(result.container),
			                   warning);
		}

		const auto& decoded = *result.sound;
		SPDLOG_LOGGER_TRACE(spdlog::get("audio"), "Decoded sound {}{}: {}, {} Hz, {}, {:.3f}s, {} padding frames removed",
		                    sound.name, part, ToString(result.container), decoded.sampleRate,
		                    decoded.channelLayout == ChannelLayout::Mono ? "mono" : "stereo", decoded.Duration(),
		                    result.trimmedFrames);
		sound.channelLayout = decoded.channelLayout;
		// Play at the rate the data was recorded at, the bank header can disagree
		sampleRate = decoded.sampleRate;
		decodeBuffer.insert(decodeBuffer.end(), decoded.samples.begin(), decoded.samples.end());
	}
	sound.bufferId = CreateBuffer(sound.channelLayout, decodeBuffer, sampleRate);
	// A loop over part of the sample, as the game's mixer plays it: from the start, round its loop while looping, and on
	// to the end once let go
	const auto frames = static_cast<int32_t>(decodeBuffer.size() / (sound.channelLayout == ChannelLayout::Stereo ? 2 : 1));
	if (sound.loopStart >= 0 && sound.loopEnd > sound.loopStart && sound.loopStart < frames)
	{
		_audioPlayer->SetLoopPoints(sound.bufferId, sound.loopStart, std::min(sound.loopEnd + 1, frames));
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
	const auto entity = CreateEmitter(id, std::nullopt, playType);
	if (entity != entt::null)
	{
		PlayEmitter(entity);
	}
}

void AudioManager::PlaySoundEffect(entt::id_type id, std::optional<glm::vec3> worldPosition)
{
	auto& sounds = Locator::resources::value().GetSounds();
	if (sounds.Contains(id))
	{
		const auto sound = sounds.Handle(id);
		// A sound with a place can't be heard from further than its maximum distance
		if (worldPosition.has_value() &&
		    glm::distance(Locator::camera::value().GetOrigin(), *worldPosition) > sound->maxDistance)
		{
			LogNotStarted(*sound, *worldPosition, TooFar(*sound, *worldPosition));
			return;
		}
		// Played with the play type of the bank header: a sound played once isn't played again while it plays,
		// which lets the game ask for a sound every frame and hear it go on, and one that restarts starts over
		if ((sound->overrideFlags & static_cast<uint32_t>(pack::AudioBankOverride::LoopType)) != 0)
		{
			const auto playing = FindPlaying(entt::null, 0, id, 0);
			if (sound->loopType == pack::AudioBankLoop::Once && playing != entt::null)
			{
				LogNotStarted(*sound, worldPosition.value_or(glm::vec3(0.0f)), "it plays once and is playing");
				return;
			}
			if (sound->loopType == pack::AudioBankLoop::Restart && playing != entt::null)
			{
				DestroyEmitter(playing);
			}
		}
	}
	const auto entity = CreateEmitter(id, worldPosition, PlayType::Once);
	if (entity != entt::null)
	{
		PlayEmitter(entity);
	}
}

void AudioManager::StopSoundEffect(entt::id_type id)
{
	for (auto playing = FindPlaying(entt::null, 0, id, 0); playing != entt::null; playing = FindPlaying(entt::null, 0, id, 0))
	{
		DestroyEmitter(playing);
	}
}

entt::entity AudioManager::StartSoundEffect(entt::id_type id, const SoundEffectOptions& options)
{
	auto& sounds = Locator::resources::value().GetSounds();
	if (!sounds.Contains(id))
	{
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Sound {} is not loaded", id);
		return entt::null;
	}
	auto sound = sounds.Handle(id);
	if (options.position.has_value() &&
	    glm::distance(Locator::camera::value().GetOrigin(), *options.position) > sound->maxDistance)
	{
		LogNotStarted(*sound, *options.position, TooFar(*sound, *options.position));
		return entt::null;
	}
	auto start = MakeVoiceStart(*sound, options.position, options.playType);
	if (options.pitchPercent.has_value())
	{
		start.pitchPercent = *options.pitchPercent;
		start.pitch = static_cast<float>(start.pitchPercent) / 100.0f;
	}
	if (options.volume.has_value())
	{
		start.volume = std::min<uint32_t>(*options.volume, k_MaxVolume);
	}
	start.minDistance = options.minDistance.value_or(start.minDistance);
	start.maxDistance = options.maxDistance.value_or(start.maxDistance);
	const auto source = CreateSource(*sound, start);
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	registry.Assign<AudioEmitter>(entity, AudioEmitter {
	                                          .sourceId = source,
	                                          .soundId = id,
	                                          .priority = sound->priority,
	                                          .spatial = options.position.has_value(),
	                                          .position = options.position.value_or(glm::zero<glm::vec3>()),
	                                          .gain = _atmos->VolumeToGain(start.volume),
	                                          .volume = start.volume,
	                                          .pitchPercent = start.pitchPercent,
	                                          .minDistance = start.minDistance,
	                                          .maxDistance = start.maxDistance,
	                                          .distanceScale = start.distanceScale,
	                                          .loop = start.loopCount < 0 ? PlayType::Repeat : PlayType::Once,
	                                          .state = AudioStatus::Initial,
	                                          .music = false,
	                                          .owner = options.owner,
	                                      });
	LogEmitterStart(entity, registry.Get<AudioEmitter>(entity));
	PlayEmitter(entity);
	return entity;
}

void AudioManager::SetEmitterPosition(entt::entity emitter, const glm::vec3& position)
{
	if (!EmitterExists(emitter))
	{
		return;
	}
	Locator::entitiesRegistry::value().Get<AudioEmitter>(emitter).position = position;
}

void AudioManager::ReleaseEmitterLoop(entt::entity emitter)
{
	if (!EmitterExists(emitter))
	{
		return;
	}
	auto& component = Locator::entitiesRegistry::value().Get<AudioEmitter>(emitter);
	SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Emitter {} let go: plays to the end of its pass",
	                    DescribeEmitter(emitter, component));
	_audioPlayer->SetLooping(component.sourceId, false);
	component.loop = PlayType::Once;
}

bool AudioManager::IsEmitterLooping(entt::entity emitter)
{
	return EmitterExists(emitter) && Locator::entitiesRegistry::value().Get<AudioEmitter>(emitter).loop == PlayType::Repeat;
}

void AudioManager::SetEmitterVolume(entt::entity emitter, uint32_t volume)
{
	if (!EmitterExists(emitter))
	{
		return;
	}
	auto& component = Locator::entitiesRegistry::value().Get<AudioEmitter>(emitter);
	SPDLOG_LOGGER_TRACE(spdlog::get("audio"), "Emitter {} volume {} -> {}", DescribeEmitter(emitter, component),
	                    component.volume, std::min<uint32_t>(volume, k_MaxVolume));
	component.volume = std::min<uint32_t>(volume, k_MaxVolume);
	component.gain = _atmos->VolumeToGain(component.volume);
	_audioPlayer->SetVolume(component.sourceId, component.gain * _globalVolume * (component.music ? _musicVolume : _sfxVolume));
}

uint32_t AudioManager::GetEmitterVolume(entt::entity emitter)
{
	return EmitterExists(emitter) ? Locator::entitiesRegistry::value().Get<AudioEmitter>(emitter).volume : 0;
}

void AudioManager::StopOwnedSounds(entt::entity owner)
{
	if (owner == entt::null)
	{
		return;
	}
	std::vector<entt::entity> owned;
	Locator::entitiesRegistry::value().Each<const AudioEmitter>([&](entt::entity entity, const AudioEmitter& emitter) {
		if (emitter.owner == owner)
		{
			owned.push_back(entity);
		}
	});
	for (const auto entity : owned)
	{
		DestroyEmitter(entity);
	}
}

void AudioManager::AddAnimEffects(const std::string& bankName, AnimEffectTable table)
{
	_animEffects.insert_or_assign(bankName, std::move(table));
}

AnimEffectPlay AudioManager::PlayAnimEffect(const std::string& bankName, std::span<const int32_t> keys, entt::entity owner,
                                            const glm::vec3& position)
{
	const auto effects = _animEffects.find(bankName);
	if (effects == _animEffects.end())
	{
		return {};
	}
	// Any of the samples of the effect
	const auto samples = effects->second.Find(keys);
	if (samples.empty())
	{
		return {};
	}
	const auto sample = samples[Locator::rng::value().NextValue<size_t>(0, samples.size() - 1)];
	const auto id = entt::hashed_string(fmt::format("{}/{}", bankName, sample).c_str()).value();
	auto& sounds = Locator::resources::value().GetSounds();
	if (!sounds.Contains(id))
	{
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Sound {}/{} of an animation effect is not loaded", bankName, sample);
		return {.outcome = AnimEffectPlay::Outcome::NotLoaded, .emitter = entt::null, .sample = sample};
	}
	const auto sound = sounds.Handle(id);

	// The sample can't be heard from further than its maximum distance, overridden or not
	if (glm::distance(Locator::camera::value().GetOrigin(), position) > sound->maxDistance)
	{
		LogNotStarted(*sound, position, TooFar(*sound, position));
		return {.outcome = AnimEffectPlay::Outcome::TooFar, .emitter = entt::null, .sample = sample};
	}

	// Played with the play type of the bank header
	const auto bank = entt::hashed_string(bankName.c_str()).value();
	if ((sound->overrideFlags & static_cast<uint32_t>(pack::AudioBankOverride::LoopType)) != 0)
	{
		if (sound->loopType == pack::AudioBankLoop::Once && FindPlaying(owner, bank, id, sound->group) != entt::null)
		{
			LogNotStarted(*sound, position, "it plays once and is playing");
			return {.outcome = AnimEffectPlay::Outcome::AlreadyPlaying, .emitter = entt::null, .sample = sample};
		}
		if (sound->loopType == pack::AudioBankLoop::Restart)
		{
			auto playing = FindPlaying(owner, bank, id, 0);
			if (playing == entt::null)
			{
				playing = FindPlaying(owner, bank, 0, sound->group);
			}
			if (playing != entt::null)
			{
				DestroyEmitter(playing);
			}
		}
	}

	const auto entity = CreateEmitter(id, position, PlayType::Once);
	if (entity == entt::null)
	{
		return {.outcome = AnimEffectPlay::Outcome::NotLoaded, .emitter = entt::null, .sample = sample};
	}
	auto& emitter = Locator::entitiesRegistry::value().Get<AudioEmitter>(entity);
	emitter.owner = owner;
	emitter.bank = bank;
	emitter.group = sound->group;
	PlayEmitter(entity);
	return {.outcome = AnimEffectPlay::Outcome::Played, .emitter = entity, .sample = sample};
}

entt::entity AudioManager::FindPlaying(entt::entity owner, entt::id_type bank, entt::id_type id, uint16_t group) const
{
	auto found = entt::entity {entt::null};
	Locator::entitiesRegistry::value().Each<const AudioEmitter>([&](entt::entity entity, const AudioEmitter& emitter) {
		if (found != entt::null || emitter.owner != owner || emitter.bank != bank || emitter.state == AudioStatus::Stopped)
		{
			return;
		}
		if (emitter.soundId == id || (group != 0 && emitter.group == group))
		{
			found = entity;
		}
	});
	return found;
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
	_musicPlayer->Stop(false);
	MusicPlay(packPath, MusicPlayOptions {.loops = type == PlayType::Repeat ? -1 : 0});
}

void AudioManager::StopMusic()
{
	_musicPlayer->Stop(false);
}

namespace
{
/// The game names its music in lower case, which the files on a case sensitive file system may not be
std::optional<std::filesystem::path> FindIgnoringCase(const std::filesystem::path& path)
{
	if (path.is_absolute())
	{
		return std::filesystem::exists(path) ? std::optional(path) : std::nullopt;
	}
	auto found = Locator::filesystem::value().GetGamePath();
	for (const auto& part : path.relative_path())
	{
		auto match = found / part;
		if (!std::filesystem::exists(match))
		{
			const auto wanted = string_utils::LowerCase(part.string());
			std::error_code error;
			for (const auto& entry : std::filesystem::directory_iterator(found, error))
			{
				if (string_utils::LowerCase(entry.path().filename().string()) == wanted)
				{
					match = entry.path();
					break;
				}
			}
		}
		if (!std::filesystem::exists(match))
		{
			return std::nullopt;
		}
		found = match;
	}
	return found;
}
} // namespace

std::shared_ptr<const MusicBank> AudioManager::LoadMusicBank(const std::string& bankPath)
{
	if (auto loaded = _musicBanks[bankPath].lock())
	{
		return loaded;
	}
	const auto path = FindIgnoringCase(bankPath);
	if (!path)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Music bank {} not found", bankPath);
		return nullptr;
	}
	pack::PackFile pack;
	if (pack.Open(*path) != pack::PackResult::Success || pack.GetAudioSampleHeaders().empty())
	{
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Music bank {} could not be read", path->string());
		return nullptr;
	}
	auto bank = std::make_shared<MusicBank>();
	bank->path = bankPath;
	const auto& headers = pack.GetAudioSampleHeaders();
	const auto& first = headers.front();
	const auto overrides = [&first](pack::AudioBankOverride flag) {
		return (first.overrideFlags & static_cast<uint32_t>(flag)) != 0;
	};
	bank->groupId = first.group;
	if (overrides(pack::AudioBankOverride::Volume))
	{
		bank->bankVolume = std::min<uint32_t>(first.volume, MusicPlayer::k_MaxVolume);
	}
	if (overrides(pack::AudioBankOverride::Loop))
	{
		bank->loopOverride = first.loop;
	}
	bank->chunks = pack.GetAudioSamplesData();
	bank->chunkSampleRates.reserve(headers.size());
	for (const auto& header : headers)
	{
		bank->chunkSampleRates.push_back(header.sampleRate);
	}
	_musicBankInfo[bankPath] = MusicBankInfo {.groupId = bank->groupId, .chunkCount = bank->GetChunkCount()};
	_musicBanks[bankPath] = bank;
	// Kept until another bank is read, so asking about a bank and then playing it reads it once
	_recentMusicBank = bank;
	return bank;
}

bool AudioManager::MusicPlay(const std::string& bankPath, const MusicPlayOptions& options)
{
	auto withBank = options;
	withBank.bank = LoadMusicBank(bankPath);
	return _musicPlayer->Play(withBank);
}

void AudioManager::MusicStop(bool fadeOut)
{
	_musicPlayer->Stop(fadeOut);
}

bool AudioManager::MusicIsActive() const
{
	return _musicPlayer->IsActive();
}

std::optional<MusicBankInfo> AudioManager::GetMusicBankInfo(const std::string& bankPath)
{
	if (const auto found = _musicBankInfo.find(bankPath); found != _musicBankInfo.end())
	{
		return found->second;
	}
	// Reading the bank tells its group and length
	if (!LoadMusicBank(bankPath))
	{
		_musicBankInfo[bankPath] = std::nullopt;
	}
	return _musicBankInfo[bankPath];
}

uint32_t AudioManager::AtmosRegisterBank(const std::string& bankName, const std::vector<pack::AudioBankSampleHeader>& headers,
                                         uint16_t atmosCount)
{
	return _atmos->RegisterBank(bankName, headers, atmosCount);
}

void AudioManager::AtmosReleaseBank(uint32_t bank)
{
	_atmos->ReleaseBank(bank);
}

void AudioManager::AtmosSetBankVolume(uint32_t bank, int32_t volume)
{
	_atmos->SetBankVolume(bank, volume);
}

void AudioManager::AtmosSetGroup(uint32_t bank, uint32_t group)
{
	_atmos->SetGroup(bank, group);
}

void AudioManager::AtmosProcess(bool active)
{
	_atmos->Process(active);
}

VoiceBackend::Handle AudioManager::Start(const VoiceStart& start)
{
	const auto stringId = fmt::format("{}/{}", start.bankName, start.sampleId);
	const entt::id_type id = entt::hashed_string(stringId.c_str());
	auto& sounds = Locator::resources::value().GetSounds();
	if (!sounds.Contains(id))
	{
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Atmosphere sample {} is not loaded", stringId);
		return k_InvalidHandle;
	}
	auto sound = sounds.Handle(id);
	const auto source = CreateSource(*sound, start);
	const auto gain = _atmos->VolumeToGain(start.volume);
	_audioPlayer->SetVolume(source, gain * _globalVolume * _sfxVolume);
	_audioPlayer->StartSource(source);

	const auto handle = _nextAtmosVoice++;
	_atmosVoices.emplace(handle, AtmosVoice {.source = source, .gain = gain});
	SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Atmosphere voice {}: {} volume {} pitch {}% at ({}, {})", handle, stringId,
	                    start.volume, start.pitchPercent, start.position.x, start.position.y);
	return handle;
}

SourceId AudioManager::CreateSource(Sound& sound, const VoiceStart& start)
{
	if (sound.bufferId == 0)
	{
		CreateBuffer(sound);
		_decodedBuffers.push_back(sound.bufferId);
	}

	// Sources are positioned relative to the listener, in the game's listener frame
	const auto source = _audioPlayer->CreateSource(start.pitch, true);
	// QMixer plays a sample loopCount + 1 times
	const auto loop = start.loopCount < 0;
	const auto playCount = loop ? 1 : start.loopCount + 1;
	for (int32_t i = 0; i < playCount; ++i)
	{
		_audioPlayer->QueueBuffer(source, sound.bufferId);
	}
	_audioPlayer->SetLooping(source, loop);
	if (start.positional)
	{
		// QSound attenuates by minDistance / (distance * scale) beyond minDistance, up to maxDistance, which is
		// OpenAL's inverse clamped model on scaled positions
		_audioPlayer->SetDistanceAttenuation(source, start.minDistance, start.maxDistance, 1.0f);
		PositionSource(source, start.position, start.distanceScale);
	}
	else
	{
		_audioPlayer->SetDistanceAttenuation(source, 1.0f, 1.0f, 0.0f);
		PositionSource(source, std::nullopt, start.distanceScale);
	}
	return source;
}

void AudioManager::PositionSource(SourceId source, std::optional<glm::vec3> listenerPosition, float distanceScale)
{
	if (!listenerPosition)
	{
		_audioPlayer->SetPosition(source, glm::zero<glm::vec3>());
		return;
	}
	// The game's listener frame is x right, y forward, z up; OpenAL's is x right, y up, -z forward
	const auto& position = *listenerPosition;
	_audioPlayer->SetPosition(source, glm::vec3(position.x, position.z, -position.y) * distanceScale);
}

glm::vec3 AudioManager::ToListenerFrame(glm::vec3 worldPosition)
{
	const auto& camera = Locator::camera::value();
	const auto forward = glm::normalize(camera.GetForward());
	const auto up = glm::normalize(camera.GetUp());
	const auto right = glm::normalize(glm::cross(forward, up));
	const auto offset = worldPosition - camera.GetOrigin();
	return {glm::dot(offset, right), glm::dot(offset, forward), glm::dot(offset, up)};
}

bool AudioManager::IsPlaying(Handle handle) const
{
	const auto iter = _atmosVoices.find(handle);
	return iter != _atmosVoices.end() && _audioPlayer->GetStatus(iter->second.source) == AudioStatus::Playing;
}

void AudioManager::SetVolume(Handle handle, uint32_t volume)
{
	const auto iter = _atmosVoices.find(handle);
	if (iter == _atmosVoices.end())
	{
		return;
	}
	iter->second.gain = _atmos->VolumeToGain(volume);
	_audioPlayer->SetVolume(iter->second.source, iter->second.gain * _globalVolume * _sfxVolume);
}

void AudioManager::Stop(Handle handle)
{
	const auto iter = _atmosVoices.find(handle);
	if (iter == _atmosVoices.end())
	{
		return;
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Atmosphere voice {} stopped", handle);
	_audioPlayer->StopSource(iter->second.source);
	_audioPlayer->DeleteSource(iter->second.source);
	_atmosVoices.erase(iter);
}

} // namespace openblack::audio
