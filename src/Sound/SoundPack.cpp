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
		_sadFile.Open(Game::instance()->GetFileSystem().FindPath(filePath).u8string());
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open {}: {}", filePath, err.what());
		return;
	}

	if (!_sadFile.GetName().empty())
		_name = _sadFile.GetName();
	else
		_name = (filePath);

	// TODO: Discover sector flag
	bool isSector;
	auto combinedData = std::map<std::string, Sound>();

	for (auto& [sample, soundData] : _sadFile.GetSounds())
	{
		auto audioSampleName = std::string(sample.audioSampleName);
		std::replace(audioSampleName.begin(), audioSampleName.end(), '\\',  FileSystem::Separator);
		auto audioSampleFileName = fs::path(audioSampleName).filename();
		auto ext = audioSampleFileName.extension().string();
		isSector = ext == ".mpg";
		auto volume = sample.volume > 0 ? ((float)sample.volume) / 127.f : 1.f;
		auto fileName = fs::path(filePath).filename().string();

		// We'll use the file name for the sound ID instead as each sector has different name
		if (isSector)
		{
			auto emplacement = combinedData.emplace(fileName, Sound{});
			auto& sound = emplacement.first->second;
			if (emplacement.second) // Sound was not in the map
			{
				sound.id = fileName;
				sound.name = fileName;
				sound.priority = sample.priority;
				sound.sampleRate = static_cast<int>(sample.sampleRate);
				sound.bitRate = 0;
				sound.volume = volume;
				sound.pitch = sample.pitch;
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
		}
		else
		{
			auto name = audioSampleFileName.string();
			Sound sound = {/*id=*/name,
			               /*name*/name,
			               /*priority=*/sample.priority,
			               /*sampleRate=*/static_cast<int>(sample.sampleRate),
			               /*bitRate=*/0,
			               /*volume=*/volume,
			               /*pitch=*/sample.pitch,
			               /*pitchDeviation=*/sample.pitchDeviation,
			               /*bytes=*/soundData,
			               /*format=*/ChannelLayout::Mono,
			               /*loopType=*/static_cast<PlayType>(sample.loopType),
			               /*loaded=*/false,
			               /*sectorMerged=*/false,
			               /*sectorCount=*/0,
			               /*lengthInSeconds=*/-1};
			_sounds[sound.id] = std::make_unique<Sound>(sound);
		}
	}

	// TODO: Rely on SadFile isMusic
	_isMusic = combinedData.size() > 0;

	for (auto& [soundName, sound] : combinedData) _sounds[soundName] = std::make_unique<Sound>(sound);

	if (_sounds.empty())
		spdlog::warn("Soundpack {} has no sounds", _name);
}

} // namespace openblack::audio
