/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "OpenAlPlayer.h"
#include "Math.h"

#include <random>
#include <string>

void alErrorCheck(std::string message, std::string file, uint64_t line)
{
	std::string errorMessage;
	ALenum error = alGetError();
	switch (error)
	{
	case AL_NO_ERROR:
		return;
	case AL_INVALID_NAME:
		errorMessage = "AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function";
		break;
	case AL_INVALID_ENUM:
		errorMessage = "AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function";
		break;
	case AL_INVALID_VALUE:
		errorMessage = "AL_INVALID_VALUE: an invalid value was passed to an OpenAL function";
		break;
	case AL_INVALID_OPERATION:
		errorMessage = "AL_INVALID_OPERATION: the requested operation is not valid";
		break;
	case AL_OUT_OF_MEMORY:
		errorMessage = "AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out of memory";
		break;
	default:
		errorMessage = "UNKNOWN AL ERROR: " + std::to_string(error);
	}

	spdlog::error(R"(OpenAL error: {} with call "{}" at file "{}" on line {})", errorMessage, message, file, line);
}

#define alCheckCall(FUNCTION_CALL) \
	FUNCTION_CALL;                 \
	alErrorCheck(#FUNCTION_CALL, __FILE__, __LINE__)

using namespace openblack::audio;

OpenAlPlayer::OpenAlPlayer()
    : _device(alcOpenDevice(nullptr), DeleteAlDevice)
    , _context(alcCreateContext(_device.get(), nullptr), DeleteAlContext)
    , _rand(std::random_device {}())
{}

void OpenAlPlayer::Activate()
{
	alCheckCall(alcMakeContextCurrent(_context.get()));
}

void OpenAlPlayer::SetVolume(float volume)
{
	_volume = volume;
}

void OpenAlPlayer::SetVolume(AudioSourceId id, float volume)
{
	alCheckCall(alSourcef(id, AL_GAIN, volume));
}

float OpenAlPlayer::GetVolume(AudioSourceId id) const
{
	return _volume;
}

int GetSoundFormat(Sound& sound)
{
	if (sound.channelLayout == ChannelLayout::Mono)
		return AL_FORMAT_MONO16;
	else if (sound.channelLayout == ChannelLayout::Stereo)
		return AL_FORMAT_STEREO16;
	else
		throw std::runtime_error("Unknown channel layout");
}

void OpenAlPlayer::SetupEmitter(AudioEmitter& emitter, Sound& sound)
{
	auto& soundData = sound.bytes;
	auto soundFormat = GetSoundFormat(sound);
	alCheckCall(alGenSources(1, &emitter.audioSourceId));
	alCheckCall(alGenBuffers(1, &emitter.audioBufferId));
	alCheckCall(alBufferData(emitter.audioBufferId, soundFormat, soundData.data(), soundData.size(), sound.sampleRate));
	alCheckCall(alSourcei(emitter.audioSourceId, AL_BUFFER, emitter.audioBufferId));
	std::uniform_int_distribution<> dist(-sound.pitchDeviation, sound.pitchDeviation);
	auto pitch = Math::MapTo((float)(sound.pitch + dist(_rand)) * 0.001, .0f, 1.0, .5f, 2.f);
	alCheckCall(alSourcef(emitter.audioSourceId, AL_PITCH, pitch));

	if (!emitter.world)
	{
		alCheckCall(alSourcei(emitter.audioSourceId, AL_SOURCE_RELATIVE, true));
	}
}

void OpenAlPlayer::PlayEmitter(AudioEmitter& emitter) const
{
	auto soundSource = emitter.audioSourceId;
	UpdateEmitterState(emitter);
	alCheckCall(alSourcef(soundSource, AL_PITCH, 1.f));
	alCheckCall(alSourcei(soundSource, AL_LOOPING, AL_FALSE));
	alCheckCall(alSourcePlay(soundSource));
}

void OpenAlPlayer::UpdateEmitterState(AudioEmitter& emitter) const
{
	if (emitter.world)
	{
		glm::vec3& pos = emitter.position;
		glm::vec3& vel = emitter.velocity;
		alCheckCall(alSource3f(emitter.audioSourceId, AL_POSITION, pos.z, pos.y, pos.x));
		alCheckCall(alSource3f(emitter.audioSourceId, AL_VELOCITY, vel.z, vel.y, vel.x));
		alCheckCall(alSource3f(emitter.audioSourceId, AL_DIRECTION, 0, 0, 0));
	}

	alCheckCall(alSourcef(emitter.audioSourceId, AL_GAIN, emitter.volume * _volume));
}

void OpenAlPlayer::CleanUpResources(std::map<AudioEmitterId, AudioEmitter>& emitters)
{
	for (auto iterator = emitters.begin(); iterator != emitters.end(); iterator++)
	{
		auto& [id, emitter] = *iterator;
		CleanUpResource(emitter);
	}
}

void OpenAlPlayer::CleanUpResource(AudioEmitter& emitter) const
{
	Destroy(emitter.audioSourceId, emitter.audioBufferId);
	emitter.audioSourceId = 0;
	emitter.audioBufferId = 0;
}

void OpenAlPlayer::PauseEmitter(AudioEmitter& emitter) const
{
	alCheckCall(alSourcePause(emitter.audioSourceId));
}

void OpenAlPlayer::StopEmitter(AudioEmitter& emitter) const
{
	alCheckCall(alSourceStop(emitter.audioSourceId));
}

void OpenAlPlayer::Stop(AudioSourceId sourceId) const
{
	ALint status;
	alCheckCall(alGetSourcei(sourceId, AL_SOURCE_STATE, &status));
	if (status != AL_STOPPED)
	{
		alCheckCall(alSourceStop(sourceId));
	}
}

void OpenAlPlayer::Destroy(AudioSourceId sourceId, AudioBufferId bufferId) const
{
	ALint status;
	alCheckCall(alGetSourcei(sourceId, AL_SOURCE_STATE, &status));
	if (status != AL_STOPPED)
	{
		alCheckCall(alSourceStop(sourceId));
	}
	alCheckCall(alDeleteSources(1, &sourceId));
	alCheckCall(alDeleteBuffers(1, &bufferId));
}

void OpenAlPlayer::DeleteAlDevice(ALCdevice* device)
{
	alCheckCall(alcCloseDevice(device));
}

void OpenAlPlayer::DeleteAlContext(ALCcontext* context)
{
	alCheckCall(alcDestroyContext(context));
}

AudioStatus OpenAlPlayer::GetAudioStatus(AudioSourceId id) const
{
	ALint status;
	alCheckCall(alGetSourcei(id, AL_SOURCE_STATE, &status));

	switch (status)
	{
	case AL_STOPPED:
		return AudioStatus::Stopped;
	case AL_PAUSED:
		return AudioStatus::Paused;
	case AL_PLAYING:
		return AudioStatus::Playing;
	case AL_INITIAL:
		return AudioStatus::Initial;
	default:
		throw std::runtime_error("Unknown audio status");
	}
}

float OpenAlPlayer::GetAudioProgress(AudioEmitter& emitter) const
{
	ALfloat offset;
	ALint size;
	alCheckCall(alGetSourcef(emitter.audioSourceId, AL_BYTE_OFFSET, &offset));
	alCheckCall(alGetBufferi(emitter.audioBufferId, AL_SIZE, &size));
	return offset / (float)size;
}

void OpenAlPlayer::UpdateListenerState(glm::vec3 pos, glm::vec3 vel, glm::vec3 front, glm::vec3 up) const
{
	alCheckCall(alListener3f(AL_POSITION, pos.z, pos.y, pos.x));
	alCheckCall(alListener3f(AL_VELOCITY, vel.z, vel.y, vel.x));
	ALfloat listenerOri[] = {front.x, front.y, front.z, up.x, up.y, up.z};
	alCheckCall(alListenerfv(AL_ORIENTATION, listenerOri));
}
