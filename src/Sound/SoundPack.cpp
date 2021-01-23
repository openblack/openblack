/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "SoundPack.h"

#include <Common/FileSystem.h>
#include <Game.h>
#include <spdlog/spdlog.h>

namespace openblack::audio
{

void SoundPack::LoadFromFile(const std::string& filePath)
{
	spdlog::debug("Loading Sound Pack from file: {}", filePath);

	try
	{
		_packFile.Open(Game::instance()->GetFileSystem().FindPath(filePath).u8string());
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open {}: {}", filePath, err.what());
		return;
	}

	if (!_packFile.GetDescription().empty())
		_name = _packFile.GetDescription();
	else
		_name = (filePath);

	// TODO: Discover sector flag
	bool isSector;
	bool isPrevSector = false;
	SoundId sectorSoundId;
	auto combinedData = std::map<SoundId, Sound>();
	auto& samples = _packFile.GetSounds();

	for (auto& [sample, soundData] : samples)
	{
		auto audioSampleName = std::string(sample.audioSampleName);
		std::replace(audioSampleName.begin(), audioSampleName.end(), '\\',  FileSystem::Separator);
		auto audioSampleFileName = fs::path(audioSampleName).filename();
		auto ext = audioSampleFileName.extension().string();
		isSector = ext == ".mpg";
		auto fileName = fs::path(filePath).filename().string();

		auto pitch = Math::MapTo(sample.pitch, 0.0f, 127.0f, 0, 100);
		auto volume = Math::MapTo(sample.volume == 0 ? 1 : sample.volume, 0.0f, 127.0f, 0, 100);

		// It's either all sectors (music) or individual sounds (dialogue, fx). Files do not contain a mix of the two
		if (isSector)
		{
			if (!isPrevSector)
				sectorSoundId = CreateId();

			auto emplacement = combinedData.emplace(sectorSoundId, Sound{});
			auto& sound = emplacement.first->second;
			if (emplacement.second) // Sound was not in the map
			{

				sound.id = sectorSoundId;
				sound.name = fileName;
				sound.priority = sample.priority;
				sound.sampleRate = static_cast<int>(sample.sampleRate);
				sound.bitRate = 0;
				sound.volume = volume;
				sound.pitch = pitch;
				sound.pitchDeviation = sample.pitchDeviation;
				sound.bytes = soundData;
				sound.channelLayout = ChannelLayout::Stereo;
				sound.playType = static_cast<PlayType>(sample.loopType);
				sound.loaded = false;
				sound.sectorMerged = true;
				sound.sectorCount = 1;
				sound.lengthInSeconds = -1;
			}
			else // Sound was already in the map
			{
				sound.sectorCount++;
				sound.bytes.insert(sound.bytes.end(), soundData.begin(), soundData.end());
			}
			isPrevSector = isSector;
		}
		else
		{
			Sound sound;
			auto name = audioSampleFileName.string();
			sound.id = CreateId();
			sound.name = name;
			sound.priority = sample.priority;
			sound.sampleRate = static_cast<int>(sample.sampleRate);
			sound.bitRate = 0;
			sound.volume = volume;
			sound.pitch = pitch;
			sound.pitchDeviation = sample.pitchDeviation;
			sound.bytes = soundData;
			sound.channelLayout = ChannelLayout::Stereo;
			sound.playType = static_cast<PlayType>(sample.loopType);
			sound.loaded = false;
			sound.sectorMerged = true;
			sound.sectorCount = 1;
			sound.lengthInSeconds = -1;
			_sounds[sound.id] = std::make_unique<Sound>(sound);
			isPrevSector = isSector;
		}
	}

	// TODO: Rely on SadFile isMusic
	_isMusic = combinedData.size() > 0;

	for (auto& [id, sound] : combinedData)
	{
		_sounds[id] = std::make_unique<Sound>(sound);
	}

	if (_sounds.empty())
		spdlog::warn("Soundpack {} has no sounds", _name);
}

} // namespace openblack::audio
