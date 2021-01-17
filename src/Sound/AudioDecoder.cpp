/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "AudioDecoder.h"
#include "Common/FileStream.h"
#include "Common/FileSystem.h"

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM

#include <cstring>

using namespace openblack::audio;

void MockAudioLoader::ToPCM16(Sound& sound)
{
	sound.loaded = true;
}

void AudioDecoder::DebugEmitWavFile(Sound& sound)
{
	auto fileName = FileSystem::WorkingDirectory() / fs::path(sound.name);
	fileName = fileName.filename().replace_extension("wav");
	auto channels = sound.channelLayout == ChannelLayout::Mono ? 1 : 2;
	// TODO: Add working directory
	auto file = FileStream(fileName, FileMode::Write);

	// Write the file headers
	auto header = "RIFF----WAVEfmt ";
	file.Write(header, strlen(header));
	file.WriteValue(16, 4);
	file.WriteValue(1, 2);
	file.WriteValue(channels, 2);
	file.WriteValue(sound.sampleRate, 4);
	file.WriteValue((sound.sampleRate * sound.bitRate * channels) / 8, 4);
	file.WriteValue(4, 2);
	file.WriteValue(16, 2);

	size_t data_chunk_pos = file.Position();
	header = "data----";
	file.Write(header, strlen(header));
	file.Write(sound.bytes.data(), sound.bytes.size());
	size_t file_length = file.Position();
	file.Seek(data_chunk_pos + 4, SeekMode::Begin);
	file.WriteValue(file_length - data_chunk_pos + 8);
	file.Seek(4, SeekMode::Begin);
	file.WriteValue(file_length - 8, 4);
}
