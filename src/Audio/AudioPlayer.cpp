/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AudioPlayer.h"

#include <cstdlib>

#include <array>
#include <random>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "AlCheck.h"

using namespace openblack::audio;

void ALC_APIENTRY AudioPlayerAlLogger([[maybe_unused]] void* userptr, char level, const char* message,
                                      [[maybe_unused]] int length) noexcept
{
	switch (level)
	{
	case 'E':
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("audio"), "{}", message);
	}
	break;
	case 'W':
	{
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "{}", message);
	}
	break;
	case 'I':
	{
		SPDLOG_LOGGER_INFO(spdlog::get("audio"), "{}", message);
	}
	break;
	}
}

void AudioPlayer::SetupLogging()
{
	auto* alsoftSetLogCallback = reinterpret_cast<void (*)(void (*)(void*, char, const char*, int) noexcept, void*) noexcept>(
	    alcGetProcAddress(nullptr, "alsoft_set_log_callback"));
	if (alsoftSetLogCallback != nullptr)
	{
		alsoftSetLogCallback(AudioPlayerAlLogger, nullptr);
	}
	else
	{
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Could not set openal logging callback");
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Falling back to tracing directly in openal");
		enum class LogLevel
		{
			Disable,
			Error,
			Warning,
			Trace
		};
		LogLevel level;
		switch (spdlog::get("audio")->level())
		{
		case spdlog::level::trace:
		case spdlog::level::debug:
		case spdlog::level::info:
			level = LogLevel::Trace;
			break;
		case spdlog::level::warn:
			level = LogLevel::Warning;
			break;
		case spdlog::level::err:
		case spdlog::level::critical:
			level = LogLevel::Error;
			break;
		default:
			level = LogLevel::Disable;
			break;
		}
		std::array<char, 2> levelStr = {'0', '\0'};
		levelStr[0] += static_cast<char>(level);
#if defined(_MSC_VER)
		_putenv_s("ALSOFT_LOGLEVEL", levelStr.data());
#elif defined(__MINGW32__)
		std::array<char, 18> setstr = {"ALSOFT_LOGLEVEL="};
		strcpy(&setstr[16], levelStr.data());
		putenv(setstr.data());
#else
		setenv("ALSOFT_LOGLEVEL", levelStr.data(), 0);
#endif
	}
}

void AudioPlayer::Initialize()
{
	ALCint majorVersion;
	ALCint minorVersion;
	alcGetIntegerv(_device.get(), ALC_MAJOR_VERSION, 1, &majorVersion);
	alcGetIntegerv(_device.get(), ALC_MINOR_VERSION, 1, &minorVersion);
	SPDLOG_LOGGER_INFO(spdlog::get("audio"), "ALC Version {}.{}", majorVersion, minorVersion);
	alCheckCall(alcMakeContextCurrent(_context.get()));
}

void AudioPlayer::UpdateListener(glm::vec3 pos, glm::vec3 vel, glm::vec3 front, glm::vec3 up) const
{
	alCheckCall(alListener3f(AL_POSITION, pos.z, pos.y, pos.x));
	alCheckCall(alListener3f(AL_VELOCITY, vel.z, vel.y, vel.x));
	ALfloat listenerOri[] = {front.x, front.y, front.z, up.x, up.y, up.z}; // NOLINT(modernize-avoid-c-arrays)
	alCheckCall(alListenerfv(AL_ORIENTATION, listenerOri));
}

BufferId AudioPlayer::CreateBuffer(ChannelLayout layout, const std::vector<int16_t>& buffer, int sampleRate)
{
	int playerLayout;
	if (layout == ChannelLayout::Mono)
	{
		playerLayout = AL_FORMAT_MONO16;
	}
	else if (layout == ChannelLayout::Stereo)
	{
		playerLayout = AL_FORMAT_STEREO16;
	}
	else
	{
		throw std::runtime_error("Unknown channel layout");
	}
	BufferId id;
	alCheckCall(alGenBuffers(1, &id));
	auto bufferSize = static_cast<ALsizei>(buffer.size() * sizeof(buffer[0]));
	alCheckCall(alBufferData(id, playerLayout, buffer.data(), bufferSize, sampleRate));
	return id;
}

void AudioPlayer::QueueBuffer(SourceId sourceId, BufferId bufferId)
{
	alCheckCall(alSourceQueueBuffers(sourceId, 1, &bufferId));
}

void AudioPlayer::DeleteBuffer(BufferId id)
{
	alCheckCall(alDeleteBuffers(1, &id));
}

SourceId AudioPlayer::CreateSource(float pitch, bool relative)
{
	unsigned int id;
	alCheckCall(alGenSources(1, &id));
	alCheckCall(alSourcef(id, AL_PITCH, pitch));
	alCheckCall(alSourcei(id, AL_SOURCE_RELATIVE, relative));
	return id;
}

void AudioPlayer::DeleteSource(SourceId id)
{
	alCheckCall(alDeleteSources(1, &id));
}

void AudioPlayer::UpdateSource(SourceId id, glm::vec3 pos, float volume, bool loop)
{
	alCheckCall(alSource3f(id, AL_POSITION, pos.z, pos.y, pos.x));
	alCheckCall(alSourcef(id, AL_GAIN, volume * _volume));
	alCheckCall(alSourcef(id, AL_LOOPING, loop));
	alCheckCall(alSourcef(id, AL_PITCH, 1.f));
}

void AudioPlayer::UpdateSource(SourceId id, float volume, bool loop)
{
	alCheckCall(alSourcef(id, AL_GAIN, volume * _volume));
	alCheckCall(alSourcef(id, AL_LOOPING, loop));
	alCheckCall(alSourcef(id, AL_PITCH, 1.f));
}

float AudioPlayer::GetDuration(BufferId id)
{
	ALint sizeInBytes;
	ALint channels;
	ALint bits;
	ALint frequency;
	alGetBufferi(id, AL_SIZE, &sizeInBytes);
	alGetBufferi(id, AL_CHANNELS, &channels);
	alGetBufferi(id, AL_BITS, &bits);
	auto lengthInSamples = sizeInBytes * 8 / (channels * bits);
	alGetBufferi(id, AL_FREQUENCY, &frequency);
	return static_cast<float>(lengthInSamples) / static_cast<float>(frequency);
}

void AudioPlayer::PlaySource(SourceId id, glm::vec3 pos, float volume, bool loop)
{
	UpdateSource(id, pos, volume, loop);
	alCheckCall(alSourcePlay(id));
}

void AudioPlayer::PlaySource(SourceId id, float volume, bool loop)
{
	UpdateSource(id, volume, loop);
	alCheckCall(alSourcePlay(id));
}

void AudioPlayer::PauseSource(SourceId id) const
{
	alCheckCall(alSourcePause(id));
}

void AudioPlayer::StopSource(SourceId id) const
{
	alCheckCall(alSourceStop(id));
}

float AudioPlayer::GetVolume() const
{
	return 0;
}

AudioPlayer::AudioPlayer()
    : _device(nullptr, DeleteDevice)
    , _context(nullptr, DeleteContext)
{
	// Register audio logging before doing any other calls to openal
	SetupLogging();

	_device = decltype(_device)(alcOpenDevice(nullptr), DeleteDevice);
	_context = decltype(_context)(alcCreateContext(_device.get(), nullptr), DeleteContext);

	if (!_device)
	{
		throw std::runtime_error(fmt::format("Error creating audio device {}", alcGetString(nullptr, alcGetError(nullptr))));
	}
	if (!_context)
	{
		throw std::runtime_error(
		    fmt::format("Error creating audio context {}", alcGetString(_device.get(), alcGetError(_device.get()))));
	}
}

AudioPlayer::~AudioPlayer() = default;

void AudioPlayer::SetVolume(SourceId id, float volume)
{
	alCheckCall(alSourcef(id, AL_GAIN, volume));
}

void AudioPlayer::DeleteDevice(ALCdevice* device)
{
	alcCloseDevice(device);
}

void AudioPlayer::DeleteContext(ALCcontext* context)
{
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
}

AudioStatus AudioPlayer::GetStatus(SourceId id) const
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

float AudioPlayer::GetProgress(size_t sizeInBytes, SourceId sourceId) const
{
	ALfloat offset;
	alCheckCall(alGetSourcef(sourceId, AL_BYTE_OFFSET, &offset));
	return offset / static_cast<float>(sizeInBytes);
}
