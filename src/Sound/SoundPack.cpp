/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "SoundPack.h"

#include <SADFile.h>
#include <spdlog/spdlog.h>

#include <Common/FileSystem.h>
#include <Game.h>

#include "Math.h"

namespace openblack::audio
{

bool SoundPack::LoadFromFile(const fs::path& path)
{
	spdlog::debug("Loading Sound Pack from file: {}", path.generic_string());

	_filename = Game::instance()->GetFileSystem().FindPath(path).u8string();

	sad::SADFile sad;
	try
	{
		pack::PackFile pack;
		pack.Open(_filename);

		const std::vector<uint8_t>& infoBuffer = pack.GetBlock("LHFileSegmentBankInfo");
		const std::vector<uint8_t>& tableBuffer = pack.GetBlock("LHAudioBankSampleTable");
		const std::vector<uint8_t>& dataBuffer = pack.GetBlock("LHAudioWaveData");

		sad.Open(infoBuffer, tableBuffer, dataBuffer);
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open {}: {}", path.generic_string(), err.what());
		return false;
	}

	if (sad.GetInfo().description[0] != '\0')
	{
		_name = sad.GetInfo().description.data();
	}
	else
	{
		spdlog::warn("File has no description {}", path.generic_string());
		_name = path;
	}

	// TODO: Discover sector flag
	bool isSector;
	bool isPrevSector = false;
	SoundId sectorSoundId;
	auto combinedData = std::map<SoundId, Sound>();

	const auto& table = sad.GetTable();
	for (size_t i = 0; i < table.size(); ++i)
	{
		const auto& sample = table[i];
		const auto& soundData = sad.GetSoundData(i);
		auto audioSampleName = std::string(sample.name.data());
		std::replace(audioSampleName.begin(), audioSampleName.end(), '\\', FileSystem::Separator);
		auto audioSampleFileName = fs::path(audioSampleName).filename();
		auto ext = audioSampleFileName.extension().string();
		isSector = ext == ".mpg";
		auto fileName = path.generic_string();

		auto pitch = Math::MapTo(sample.pitch, 0.0f, 127.0f, 0, 100);
		auto volume = Math::MapTo(sample.volume == 0 ? 1 : sample.volume, 0.0f, 127.0f, 0, 100);

		// It's either all sectors (music) or individual sounds (dialogue, fx). Files do not contain a mix
		if (isSector)
		{
			if (!isPrevSector)
				sectorSoundId = CreateId();
			auto emplacement = combinedData.emplace(sectorSoundId, Sound {});
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
				sound.bytes = std::vector<uint8_t>(soundData.begin(), soundData.end());
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
			sound.bytes = std::vector<uint8_t>(soundData.begin(), soundData.end());
			sound.channelLayout = ChannelLayout::Stereo;
			sound.playType = static_cast<PlayType>(sample.loopType);
			sound.loaded = false;
			sound.sectorMerged = true;
			sound.sectorCount = 1;
			sound.lengthInSeconds = -1;
			_sounds[sound.id] = std::make_unique<Sound>(sound);
			isPrevSector = isSector;
		}

		// TODO: Rely on SadFile isMusic
		_isMusic = combinedData.size() > 0;
		for (auto& [id, sound] : combinedData)
		{
			_sounds[id] = std::make_unique<Sound>(sound);
		}
		if (_sounds.empty())
		{
			spdlog::warn("Soundpack {} has no sounds", _name);
		}
	}

	return true;
}

} // namespace openblack::audio
