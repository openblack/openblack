/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AtmosAudio.h"

#include <PackFile.h>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "AudioManagerInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"

namespace openblack::audio
{

namespace
{
// Ambience stays silent for the first turns of a level
constexpr uint32_t k_FirstAudibleTurn = 6;

// Banks switch to their evil samples at or below this alignment
constexpr double k_EvilAlignment = -0.6;

constexpr float k_FastFadeStep = 0.04f;
constexpr float k_SlowFadeStep = 0.02f;
constexpr float k_FastFadeMin = 0.1f;
constexpr float k_FastFadeMax = 0.8f;
constexpr float k_BankVolumeScale = 127.0f;

constexpr uint32_t k_GoodGroup = 1;
constexpr uint32_t k_EvilGroup = 2;

int32_t Ftol(double value)
{
	if (value <= -2147483649.0 || value >= 2147483648.0)
	{
		return INT32_MIN;
	}
	return static_cast<int32_t>(value);
}
} // namespace

void AtmosAudio::Init()
{
	if (_registered)
	{
		Release();
	}

	auto& audio = Locator::audio::value();
	auto& fileSystem = Locator::filesystem::value();
	for (size_t i = 0; i < k_AtmosTypeCount; ++i)
	{
		_banks.at(i) = 0;
		const auto& bank = k_AtmosTypeInfos.at(i).bank;
		if (bank.empty())
		{
			continue;
		}

		const auto relativePath = fileSystem.GetPath<filesystem::Path::Audio>() / bank;
		if (!fileSystem.Exists(relativePath))
		{
			SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Atmosphere bank {} not found", bank);
			continue;
		}
		const auto path = fileSystem.FindPath(relativePath);

		pack::PackFile soundPack;
		const auto result = soundPack.ReadFile(*fileSystem.GetData(path));
		if (result != pack::PackResult::Success)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("audio"), "Unable to load atmosphere bank {}: {}", bank, pack::ResultToStr(result));
			continue;
		}
		_banks.at(i) = audio.AtmosRegisterBank(path.filename().string(), soundPack.GetAudioSampleHeaders(),
		                                       soundPack.GetAudioBankAtmosCount());
	}

	_targets.fill(0.0f);
	_current.fill(0.0f);
	_registered = true;
}

void AtmosAudio::Release()
{
	if (!_registered)
	{
		return;
	}

	auto& audio = Locator::audio::value();
	for (auto& bank : _banks)
	{
		if (bank != 0)
		{
			audio.AtmosReleaseBank(bank);
			bank = 0;
		}
	}
	_registered = false;
}

// fn_005E2240, from the alignment that fn_0064AC30 maps to 0-1
void AtmosAudio::SetAlignment(float alignment)
{
	_alignment = CalculateAlignmentValue(alignment);
}

float AtmosAudio::CalculateAlignmentValue(float alignment)
{
	const auto goodness = static_cast<float>((static_cast<double>(alignment) + 1.0) * 0.5);
	auto clamped = 0.0f;
	if (goodness >= 0.0f)
	{
		clamped = goodness > 1.0f ? 1.0f : goodness;
	}
	const auto evilness = (1.0 - clamped) + (1.0 - clamped);
	return static_cast<float>((2.0 - evilness) - 1.0);
}

uint32_t AtmosAudio::GetGroup() const
{
	return static_cast<double>(_alignment) <= k_EvilAlignment ? k_EvilGroup : k_GoodGroup;
}

float AtmosAudio::CalculateSkyType(float time, const SkyInterface::DayNightTimes& times)
{
	// Mirrored around midday
	if (time > 12.0f)
	{
		time = static_cast<float>(24.0 - time);
	}

	if (time < times.nightFull)
	{
		return 2.0f;
	}
	if (time < times.duskStart)
	{
		return static_cast<float>(
		    2.0 - ((static_cast<double>(time) - times.nightFull) / (static_cast<double>(times.duskStart) - times.nightFull)));
	}
	if (time < times.duskEnd)
	{
		return 1.0f;
	}
	if (time < times.dayFull)
	{
		return static_cast<float>(
		    1.0 - ((static_cast<double>(time) - times.duskEnd) / (static_cast<double>(times.dayFull) - times.duskEnd)));
	}
	return 0.0f;
}

void AtmosAudio::EndTurn(const TurnInputs& inputs)
{
	const SoundMap::Inputs mapInputs {
	    .receiver = inputs.camera,
	    .weather = inputs.weather,
	    .skyType = CalculateSkyType(Locator::skySystem::value().GetTime(), Locator::skySystem::value().GetDayNightTimes()),
	};
	_soundMap.Update(Locator::terrainSystem::value(), Locator::infoConstants::value().sound, mapInputs);

	auto& audio = Locator::audio::value();
	if (inputs.paused || inputs.turn < k_FirstAudibleTurn)
	{
		audio.AtmosProcess(false);
		return;
	}

	// GAudio::ProcessAudioGameTurn
	CopyTargets(inputs.widescreen);
	ProcessAtmosBanks();
	if (!inputs.videoPlaying)
	{
		audio.AtmosProcess(true);
	}
}

// fn_00429100
void AtmosAudio::CopyTargets(bool widescreen)
{
	if (widescreen)
	{
		_targets.fill(0.0f);
		return;
	}
	_targets = _soundMap.GetVolumes();
}

void AtmosAudio::ProcessAtmosBanks()
{
	auto& audio = Locator::audio::value();
	const auto group = GetGroup();
	for (size_t i = 0; i < k_AtmosTypeCount; ++i)
	{
		audio.AtmosSetGroup(_banks.at(i), group);

		const auto [current, bankVolume] = StepBankVolume(_current.at(i), _targets.at(i));
		_current.at(i) = current;
		audio.AtmosSetBankVolume(_banks.at(i), bankVolume);
	}
}

std::pair<float, int32_t> AtmosAudio::StepBankVolume(float current, float target)
{
	const auto step = current > k_FastFadeMax || current <= k_FastFadeMin ? k_SlowFadeStep : k_FastFadeStep;

	double volume;
	if (target <= current)
	{
		volume = static_cast<double>(current) - step;
		if (!(static_cast<double>(target) <= volume))
		{
			volume = target;
		}
	}
	else
	{
		volume = static_cast<double>(current) + step;
		if (static_cast<double>(target) < volume)
		{
			volume = target;
		}
	}
	return {static_cast<float>(volume), Ftol(volume * k_BankVolumeScale)};
}

} // namespace openblack::audio
