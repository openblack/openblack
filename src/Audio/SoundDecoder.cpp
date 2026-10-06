/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SoundDecoder.h"

#include <cmath>
#include <cstring>

#include <algorithm>
#include <utility>

#include <fmt/format.h>

extern "C" {
#include <dr_mp3.h>
#include <dr_wav.h>
}

namespace openblack::audio
{

namespace
{
constexpr uint16_t k_WaveFormatPcm = 0x0001;
constexpr uint16_t k_WaveFormatAdpcm = 0x0002;
constexpr uint16_t k_WaveFormatMpeg = 0x0050;
constexpr uint16_t k_WaveFormatMpegLayer3 = 0x0055;
constexpr uint16_t k_WaveFormatExtensible = 0xFFFE;

/// An MPEG length is estimated from the bit rate, so it may differ from the decoded one by this much
constexpr float k_DurationTolerance = 0.05f;
constexpr float k_DurationToleranceRatio = 0.05f;
/// Bit rates outside these bounds mean an MPEG decoder locked onto data that is not MPEG audio
constexpr float k_MinMpegBitRate = 4'000.0f;
constexpr float k_MaxMpegBitRate = 450'000.0f;

struct WavInfo
{
	uint16_t formatTag;
	uint16_t channels;
	uint32_t sampleRate;
	uint32_t averageBytesPerSecond;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	size_t dataOffset;
	size_t dataSize;
	std::optional<uint32_t> factSamples;
};

template <typename T>
T Read(std::span<const uint8_t> data, size_t offset)
{
	T value;
	std::memcpy(&value, data.data() + offset, sizeof(T));
	return value;
}

std::optional<WavInfo> ParseWav(std::span<const uint8_t> data)
{
	if (data.size() < 12 || std::memcmp(data.data(), "RIFF", 4) != 0 || std::memcmp(data.data() + 8, "WAVE", 4) != 0)
	{
		return std::nullopt;
	}

	WavInfo info {};
	bool hasFormat = false;
	bool hasData = false;
	size_t offset = 12;
	while (offset + 8 <= data.size())
	{
		const auto* id = data.data() + offset;
		const auto size = Read<uint32_t>(data, offset + 4);
		const auto body = offset + 8;
		if (std::memcmp(id, "fmt ", 4) == 0 && body + 16 <= data.size())
		{
			info.formatTag = Read<uint16_t>(data, body);
			info.channels = Read<uint16_t>(data, body + 2);
			info.sampleRate = Read<uint32_t>(data, body + 4);
			info.averageBytesPerSecond = Read<uint32_t>(data, body + 8);
			info.blockAlign = Read<uint16_t>(data, body + 12);
			info.bitsPerSample = Read<uint16_t>(data, body + 14);
			if (info.formatTag == k_WaveFormatExtensible && size >= 26 && body + 26 <= data.size())
			{
				info.formatTag = Read<uint16_t>(data, body + 24);
			}
			hasFormat = true;
		}
		else if (std::memcmp(id, "data", 4) == 0)
		{
			info.dataOffset = body;
			info.dataSize = std::min<size_t>(size, data.size() - std::min(body, data.size()));
			hasData = true;
		}
		else if (std::memcmp(id, "fact", 4) == 0 && body + 4 <= data.size())
		{
			info.factSamples = Read<uint32_t>(data, body);
		}
		offset = body + size + (size & 1);
	}
	if (!hasFormat || !hasData)
	{
		return std::nullopt;
	}
	return info;
}

bool IsMpeg(std::span<const uint8_t> data)
{
	if (data.size() >= 3 && std::memcmp(data.data(), "ID3", 3) == 0)
	{
		return true;
	}
	return data.size() >= 2 && data[0] == 0xFF && (data[1] & 0xE0) == 0xE0;
}

std::optional<ChannelLayout> ToLayout(uint32_t channels)
{
	switch (channels)
	{
	case 1:
		return ChannelLayout::Mono;
	case 2:
		return ChannelLayout::Stereo;
	default:
		return std::nullopt;
	}
}

void DecodeWav(std::span<const uint8_t> data, DecodeResult& result)
{
	drwav wav;
	if (drwav_init_memory(&wav, data.data(), data.size(), nullptr) == DRWAV_FALSE)
	{
		result.error = "the WAV decoder rejected the data";
		return;
	}
	const auto layout = ToLayout(wav.channels);
	if (!layout)
	{
		result.error = fmt::format("{} channels are not supported", wav.channels);
		drwav_uninit(&wav);
		return;
	}

	DecodedSound sound {.samples = {},
	                    .channelLayout = *layout,
	                    .sampleRate = static_cast<int>(wav.sampleRate),
	                    .frames = wav.totalPCMFrameCount};
	sound.samples.resize(static_cast<size_t>(wav.totalPCMFrameCount * wav.channels));
	const auto read = drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, sound.samples.data());
	if (read != wav.totalPCMFrameCount)
	{
		result.warnings.push_back(fmt::format("only {} of {} frames could be decoded", read, wav.totalPCMFrameCount));
		sound.frames = read;
		sound.samples.resize(static_cast<size_t>(read * wav.channels));
	}
	drwav_uninit(&wav);
	result.sound = std::move(sound);
}

void DecodeMpeg(std::span<const uint8_t> data, DecodeResult& result)
{
	drmp3 mp3;
	if (drmp3_init_memory(&mp3, data.data(), data.size(), nullptr) == DRMP3_FALSE)
	{
		result.error = "the MPEG decoder rejected the data";
		return;
	}
	const auto layout = ToLayout(mp3.channels);
	if (!layout)
	{
		result.error = fmt::format("{} channels are not supported", mp3.channels);
		drmp3_uninit(&mp3);
		return;
	}

	const auto frames = drmp3_get_pcm_frame_count(&mp3);
	DecodedSound sound {
	    .samples = {}, .channelLayout = *layout, .sampleRate = static_cast<int>(mp3.sampleRate), .frames = frames};
	sound.samples.resize(static_cast<size_t>(frames * mp3.channels));
	const auto read = drmp3_read_pcm_frames_s16(&mp3, frames, sound.samples.data());
	if (read != frames)
	{
		result.warnings.push_back(fmt::format("only {} of {} frames could be decoded", read, frames));
		sound.frames = read;
		sound.samples.resize(static_cast<size_t>(read * mp3.channels));
	}
	drmp3_uninit(&mp3);

	// Locking onto stray sync bytes in data that is not MPEG audio gives a few frames of noise
	const auto duration = sound.Duration();
	if (duration > 0.0f)
	{
		const auto bitRate = static_cast<float>(data.size()) * 8.0f / duration;
		if (bitRate < k_MinMpegBitRate || bitRate > k_MaxMpegBitRate)
		{
			result.warnings.push_back(fmt::format("implausible MPEG bit rate {:.0f} bit/s for {} bytes over {:.3f}s", bitRate,
			                                      data.size(), duration));
		}
	}
	result.sound = std::move(sound);
}
} // namespace

const char* ToString(SoundContainer container)
{
	switch (container)
	{
	case SoundContainer::RawMpeg:
		return "MPEG";
	case SoundContainer::WavPcm:
		return "WAV PCM";
	case SoundContainer::WavAdpcm:
		return "WAV MS-ADPCM";
	case SoundContainer::WavMpeg:
		return "WAV MPEG";
	case SoundContainer::WavOther:
		return "WAV";
	case SoundContainer::Unknown:
	default:
		return "unknown";
	}
}

DecodeResult DecodeSound(std::span<const uint8_t> data, int expectedSampleRate)
{
	DecodeResult result;

	if (const auto wav = ParseWav(data))
	{
		switch (wav->formatTag)
		{
		case k_WaveFormatPcm:
			result.container = SoundContainer::WavPcm;
			if (wav->blockAlign != 0)
			{
				result.expectedFrames = wav->dataSize / wav->blockAlign;
			}
			DecodeWav(data, result);
			break;
		case k_WaveFormatAdpcm:
			// Compressed blocks hold a fixed number of frames, so the last one is padded out with silence. The fact
			// chunk gives the real length: without it a looping sample has a gap at its end.
			result.container = SoundContainer::WavAdpcm;
			result.expectedFrames = wav->factSamples;
			DecodeWav(data, result);
			break;
		case k_WaveFormatMpeg:
		case k_WaveFormatMpegLayer3:
			// The WAV decoder cannot read MPEG data, decode the data chunk as an MPEG stream
			result.container = SoundContainer::WavMpeg;
			if (wav->averageBytesPerSecond != 0)
			{
				result.expectedDuration = static_cast<float>(wav->dataSize) / static_cast<float>(wav->averageBytesPerSecond);
			}
			DecodeMpeg(data.subspan(wav->dataOffset, wav->dataSize), result);
			break;
		default:
			result.container = SoundContainer::WavOther;
			result.expectedFrames = wav->factSamples;
			DecodeWav(data, result);
			break;
		}
	}
	else if (IsMpeg(data))
	{
		result.container = SoundContainer::RawMpeg;
		DecodeMpeg(data, result);
	}
	else
	{
		const auto head = data.subspan(0, std::min<size_t>(data.size(), 4));
		std::string bytes;
		for (const auto byte : head)
		{
			bytes += fmt::format("{:02x}", byte);
		}
		result.error = fmt::format("unrecognised data starting {} ({} bytes)", bytes, data.size());
		return result;
	}

	if (!result.sound)
	{
		return result;
	}

	auto& sound = *result.sound;
	if (result.expectedFrames)
	{
		const auto channels = sound.channelLayout == ChannelLayout::Stereo ? 2u : 1u;
		if (sound.frames > *result.expectedFrames && result.container != SoundContainer::WavPcm)
		{
			result.trimmedFrames = sound.frames - *result.expectedFrames;
			sound.frames = *result.expectedFrames;
			sound.samples.resize(static_cast<size_t>(sound.frames * channels));
		}
		if (sound.sampleRate != 0)
		{
			result.expectedDuration = static_cast<float>(*result.expectedFrames) / static_cast<float>(sound.sampleRate);
		}
	}

	// Banks fill their unused ids with a placeholder WAV whose data chunk is empty, which correctly decodes to silence
	const auto declaredSilent = result.expectedFrames && *result.expectedFrames == 0;
	if (sound.frames == 0 && !declaredSilent)
	{
		result.error = "decoded to no audio";
		result.sound.reset();
		return result;
	}

	const auto duration = sound.Duration();
	if (result.expectedFrames)
	{
		if (sound.frames != *result.expectedFrames)
		{
			result.warnings.push_back(
			    fmt::format("decoded {} frames but the file declares {}", sound.frames, *result.expectedFrames));
		}
	}
	else if (result.expectedDuration)
	{
		const auto difference = std::abs(duration - *result.expectedDuration);
		if (difference > k_DurationTolerance + (*result.expectedDuration * k_DurationToleranceRatio))
		{
			result.warnings.push_back(
			    fmt::format("decoded {:.3f}s but the file declares {:.3f}s", duration, *result.expectedDuration));
		}
	}
	if (expectedSampleRate != 0 && sound.sampleRate != expectedSampleRate)
	{
		result.warnings.push_back(
		    fmt::format("decoded at {} Hz but the bank header says {} Hz", sound.sampleRate, expectedSampleRate));
	}
	return result;
}

} // namespace openblack::audio
