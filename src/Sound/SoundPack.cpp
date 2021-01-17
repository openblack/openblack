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

void SoundPack::LoadFromFile(const std::string& filename)
{
	spdlog::debug("Loading Sound Pack from file: {}", filename);

	try
	{
		_sadFile.Open(Game::instance()->GetFileSystem().FindPath(filename).u8string());
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open {}: {}", filename, err.what());
		return;
	}

	if (!_sadFile.GetName().empty())
		_name = _sadFile.GetName();
	else
		_name = (filename);

	// TODO: Discover sector flag
	bool isSector;
	auto combinedData = std::map<std::string, Sound>();

	for (auto& [sample, soundData] : _sadFile.GetSounds())
	{
		auto path = fs::path(sample.audioSampleName).filename();
		auto soundName = path.string();
		auto ext = path.extension().string();
		isSector = ext == ".mpg";
		auto volume = sample.volume > 0 ? ((float)sample.volume) / 127.f : 1.f;

		// We'll use the file name for the sound ID instead as each sector has different name
		if (isSector)
		{
			if (combinedData.find(filename) == combinedData.end())
				combinedData[filename] = {
					/*id=*/filename,
					/*name*/filename,
					/*priority=*/sample.priority,
					/*sampleRate=*/static_cast<int>(sample.sampleRate),
				    /*bitRate=*/0,
					/*volume=*/volume,
					/*pitch=*/sample.pitch,
					/*pitchDeviation=*/sample.pitchDeviation,
					/*bytes=*/soundData,
				    /*channelLayout=*/ChannelLayout::Stereo,
					/*loopType=*/static_cast<PlayType>(sample.loopType),
					/*loaded=*/false,
					/*sectorMerged=*/true,
				    /*sectorCount=*/1,
					/*lengthInSeconds=*/-1
				};
			else
			{
				auto& sound = combinedData[filename];
				sound.sectorCount++;
				sound.bytes.insert(sound.bytes.end(), soundData.begin(), soundData.end());
			}
		}
		else
		{
			Sound sound = {
				/*id=*/soundName,
			    /*name*/soundName,
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
				/*lengthInSeconds=*/-1
			};
			_sounds[sound.id] = std::make_unique<Sound>(sound);
		}
	}

	// TODO: Rely on SadFile isMusic
	_isMusic = combinedData.size() > 0;

	for (auto& [soundName, sound] : combinedData)
		_sounds[soundName] = std::make_unique<Sound>(sound);

	if (_sounds.empty())
		spdlog::warn("Soundpack {} has no sounds", _name);
}

} // namespace openblack
