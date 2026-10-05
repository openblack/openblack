/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AtmosPlayer.h"

#include <cmath>
#include <ctime>

#include <algorithm>
#include <utility>

namespace openblack::audio
{

using pack::AudioBankOverride;

namespace
{
// The game's default sample play options
constexpr int32_t k_DefaultPitchPercent = 100;
constexpr float k_DefaultMinDistance = 1.0f;
constexpr float k_DefaultMaxDistance = 9999.0f;
constexpr float k_DefaultDistanceScale = 0.3f;

// Volumes are handed to the QMixer library, whose volume is linear from 0 to 32767
constexpr uint32_t k_QMixerVolumePerStep = 258;
constexpr float k_QMixerMaxVolume = 32767.0f;

// Volume steps of the looping beds and of one-shots fading out of the current group, per turn
constexpr int32_t k_FadeStep = 5;

// A new bank's earliest one-shot plays this many turns after registration
constexpr uint32_t k_FirstTriggerLead = 20;

bool HasOverride(const pack::AudioBankSampleHeader& header, AudioBankOverride flag)
{
	return (header.overrideFlags & static_cast<uint32_t>(flag)) != 0;
}

/// Group a sample is tagged with: high word of the dword at 0x118 of the sample header
uint32_t AtmosGroup(const pack::AudioBankSampleHeader& header)
{
	return static_cast<uint16_t>(header.atmosGroup);
}

/// Volume a scheduled sample plays at relative to its bank, before the bank volume is applied
uint32_t AtmosSampleVolume(const pack::AudioBankSampleHeader& header)
{
	return HasOverride(header, AudioBankOverride::Volume) ? header.volume : AtmosPlayer::k_MaxVolume;
}

uint32_t ScaleVolume(uint32_t volume, uint32_t scale)
{
	return (volume * scale) / AtmosPlayer::k_MaxVolume;
}

int32_t RandomOffset(AudioRandom& random)
{
	// 2 - rand() * 4 / RAND_MAX in integer arithmetic: one of -2, -1, 0, 1 and, for rand() == RAND_MAX only, 2
	return 2 - static_cast<int32_t>((random.Next() * 4) / AudioRandom::k_RandMax);
}
} // namespace

AtmosPlayer::AtmosPlayer(VoiceBackend& backend, Clock clock)
    : _backend(backend)
    , _clock(clock ? std::move(clock) : [] { return static_cast<uint32_t>(std::time(nullptr)); })
{
	// The game's audio system seeds the CRT generator once at start up
	Reseed();
}

void AtmosPlayer::Reseed()
{
	_random.Seed(_clock());
}

AtmosPlayer::~AtmosPlayer()
{
	for (const auto& voice : _voices)
	{
		_backend.Stop(voice.handle);
	}
}

AtmosPlayer::Bank* AtmosPlayer::FindBank(BankId bank) const
{
	const auto iter = _banks.find(bank);
	return iter != _banks.end() ? iter->second.get() : nullptr;
}

AtmosPlayer::BankId AtmosPlayer::RegisterBank(const std::string& bankName,
                                              const std::vector<pack::AudioBankSampleHeader>& headers, uint16_t atmosCount)
{
	auto bank = std::make_unique<Bank>();
	bank->name = bankName;
	bank->headers = headers;

	const auto id = _nextBankId++;
	auto& registered = *_banks.emplace(id, std::move(bank)).first->second;
	if (atmosCount != 0)
	{
		RegisterAtmos(registered);
	}
	return id;
}

void AtmosPlayer::RegisterAtmos(Bank& bank)
{
	Reseed();

	uint32_t firstTrigger = UINT32_MAX;
	for (const auto& header : bank.headers)
	{
		if (header.atmosInterval < 0 || header.id == 0)
		{
			continue;
		}

		auto info = std::make_unique<AtmosInfo>();
		info->bank = &bank;
		info->header = &header;
		info->nextTime = 0;
		info->group = AtmosGroup(header);
		info->volume = AtmosSampleVolume(header);
		if (header.atmosInterval != 0)
		{
			info->nextTime = ScheduleTime(header.atmosInterval);
			firstTrigger = std::min(firstTrigger, info->nextTime);
		}
		Insert(std::move(info));
	}

	if (firstTrigger == UINT32_MAX)
	{
		return;
	}

	// The clock is shared by every bank: it jumps to just before this bank's first one-shot and anything already
	// queued that is now in the past is rescheduled from there
	_tick = firstTrigger - k_FirstTriggerLead;
	const auto queued = _queue.size();
	for (size_t i = 0; i < queued; ++i)
	{
		if (_queue.front()->nextTime >= _tick)
		{
			break;
		}
		auto info = std::move(_queue.front());
		_queue.pop_front();
		info->nextTime = ScheduleTime(info->header->atmosInterval);
		Insert(std::move(info));
	}
}

uint32_t AtmosPlayer::ScheduleTime(int32_t interval)
{
	// Uniform in [4, 16) * interval turns from now, computed as the library does
	const auto sixTimes = (interval * 600) / 100;
	const auto fourTimes = (interval * 10) - sixTimes;
	const auto offset = (_random.Next() * static_cast<uint32_t>(sixTimes) * 2u) / AudioRandom::k_RandMax;
	return _tick + offset + static_cast<uint32_t>(fourTimes);
}

void AtmosPlayer::Insert(std::unique_ptr<AtmosInfo> info)
{
	if (info->header->atmosInterval == 0)
	{
		InsertLoop(*info);
		return;
	}

	// Ahead of every one-shot due at the same time or later
	const auto position =
	    std::ranges::find_if(_queue, [time = info->nextTime](const auto& queued) { return queued->nextTime >= time; });
	_queue.insert(position, std::move(info));
}

void AtmosPlayer::InsertLoop(const AtmosInfo& info)
{
	_loops.push_back(Loop {
	    .bank = info.bank,
	    .header = info.header,
	    .voice = VoiceBackend::k_InvalidHandle,
	    .target = AtmosSampleVolume(*info.header),
	    .current = 0,
	    .playing = false,
	    .group = AtmosGroup(*info.header),
	});
}

void AtmosPlayer::ReleaseBank(BankId id)
{
	auto* bank = FindBank(id);
	if (bank == nullptr)
	{
		return;
	}

	_loops.remove_if([bank](const Loop& loop) { return loop.bank == bank; });
	_queue.remove_if([bank](const auto& info) { return info->bank == bank; });

	for (auto iter = _voices.begin(); iter != _voices.end();)
	{
		if (iter->bank == bank)
		{
			_backend.Stop(iter->handle);
			iter = _voices.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	_banks.erase(id);
}

void AtmosPlayer::SetBankVolume(BankId id, int32_t volume)
{
	auto* bank = FindBank(id);
	if (bank == nullptr)
	{
		return;
	}

	const auto clamped = static_cast<uint32_t>(std::clamp(volume, 0, static_cast<int32_t>(k_MaxVolume)));
	if (clamped == bank->volume)
	{
		return;
	}
	bank->volume = clamped;

	// Playing one-shots follow the bank straight away, the looping beds catch up in Process
	for (auto& voice : _voices)
	{
		const auto* info = voice.atmosInfo;
		if (info == nullptr || voice.bank != bank)
		{
			continue;
		}
		if (info->group != 0 && info->group != bank->group)
		{
			continue;
		}
		SetVoiceVolume(voice, static_cast<int32_t>(ScaleVolume(info->volume, clamped)));
	}
}

uint32_t AtmosPlayer::GetBankVolume(BankId id) const
{
	const auto* bank = FindBank(id);
	return bank != nullptr ? bank->volume : 0;
}

void AtmosPlayer::SetGroup(BankId id, uint32_t group)
{
	if (auto* bank = FindBank(id))
	{
		bank->group = group;
	}
}

void AtmosPlayer::Process(bool active)
{
	PruneVoices();

	if (!active)
	{
		for (auto& loop : _loops)
		{
			if (loop.voice != VoiceBackend::k_InvalidHandle)
			{
				StopVoice(loop.voice);
			}
			loop.playing = false;
		}
		for (auto iter = _voices.begin(); iter != _voices.end();)
		{
			if (iter->atmosInfo != nullptr || iter->isAtmosLoop)
			{
				_backend.Stop(iter->handle);
				iter = _voices.erase(iter);
			}
			else
			{
				++iter;
			}
		}
		return;
	}

	// One-shots tagged with another group fade out
	for (auto& voice : _voices)
	{
		const auto* info = voice.atmosInfo;
		if (voice.bank == nullptr || info == nullptr || info->group == 0 || info->group == voice.bank->group)
		{
			continue;
		}
		const auto volume = static_cast<int32_t>(voice.volume) - k_FadeStep;
		if (volume >= 0)
		{
			SetVoiceVolume(voice, volume);
		}
	}

	// Start and stop the looping beds
	for (auto& loop : _loops)
	{
		if (loop.voice != VoiceBackend::k_InvalidHandle && !IsVoicePlaying(loop.voice))
		{
			loop.playing = false;
		}
		if (loop.playing)
		{
			if (loop.bank->volume == 0)
			{
				StopVoice(loop.voice);
				loop.playing = false;
			}
			if (loop.playing && !IsVoicePlaying(loop.voice))
			{
				loop.playing = false;
			}
		}
		if (!loop.playing && loop.bank->volume != 0 && (loop.group == 0 || loop.group == loop.bank->group))
		{
			PlayOptions options;
			options.playType = PlayType::Once;
			options.isAtmosLoop = true;
			options.volume = 0;
			options.loopCount = -1;
			loop.voice = PlaySample(*loop.bank, *loop.header, options);
			if (loop.voice != VoiceBackend::k_InvalidHandle)
			{
				loop.playing = true;
				loop.current = 0;
			}
		}
	}

	// Fade the looping beds towards their volume, or out when they belong to another group
	for (auto& loop : _loops)
	{
		if (!loop.playing)
		{
			continue;
		}

		uint32_t level;
		if (loop.group != 0 && loop.group != loop.bank->group)
		{
			if (loop.current <= 0)
			{
				StopVoice(loop.voice);
				loop.playing = false;
				loop.current = 0;
				continue;
			}
			loop.current = std::max(loop.current - k_FadeStep, 0);
			level = static_cast<uint32_t>(loop.current);
		}
		else if (std::cmp_less(loop.current, loop.target))
		{
			// May overshoot the target by up to 4 for one turn, the voice volume clamps at 127
			loop.current += k_FadeStep;
			level = static_cast<uint32_t>(loop.current);
		}
		else
		{
			loop.current = static_cast<int32_t>(loop.target);
			level = loop.target;
		}

		if (auto* voice = FindVoice(loop.voice))
		{
			SetVoiceVolume(*voice, static_cast<int32_t>(ScaleVolume(loop.bank->volume, level)));
		}
	}

	ProcessOneShot();
}

void AtmosPlayer::ProcessOneShot()
{
	if (_queue.empty())
	{
		return;
	}

	++_tick;

	// At most one one-shot per turn
	auto& info = *_queue.front();
	if (info.nextTime > _tick)
	{
		return;
	}

	if (info.bank->volume != 0 && (info.group == 0 || info.group == info.bank->group))
	{
		PlayOneShot(info);
	}

	auto rescheduled = std::move(_queue.front());
	_queue.pop_front();
	rescheduled->nextTime = ScheduleTime(rescheduled->header->atmosInterval);
	Insert(std::move(rescheduled));
}

void AtmosPlayer::PlayOneShot(const AtmosInfo& info)
{
	const auto volume = ScaleVolume(info.bank->volume, info.volume);

	auto x = static_cast<float>(RandomOffset(_random));
	auto y = static_cast<float>(RandomOffset(_random));
	if (std::fabs(x) + std::fabs(y) <= 1.0f)
	{
		// Too close to the listener: push it out, and use a diagonal when it would sit right on top of them
		x *= 4.0f;
		y *= 4.0f;
		if (x == 0.0f && y == 0.0f)
		{
			x = static_cast<float>(_cornerX * 5);
			y = static_cast<float>(_cornerY * 5);
			_cornerX = -_cornerX;
			_cornerY *= _cornerX;
		}
	}

	PlayOptions options;
	options.atmosInfo = &info;
	options.volume = volume;
	options.loopCount = 0;
	options.positional = true;
	options.position = glm::vec3(x, y, 0.0f);
	PlaySample(*info.bank, *info.header, options);
}

VoiceBackend::Handle AtmosPlayer::PlaySample(Bank& bank, const pack::AudioBankSampleHeader& header, const PlayOptions& options)
{
	if (!_playSeeded)
	{
		Reseed();
		_playSeeded = true;
	}

	// What to do about instances already playing
	const auto playType = HasOverride(header, AudioBankOverride::LoopType)
	                          ? static_cast<PlayType>(static_cast<uint32_t>(header.loopType))
	                          : options.playType;
	const auto voiceGroup = static_cast<uint32_t>(static_cast<uint16_t>(header.group));
	const auto findPlaying = [this, &bank, &header, voiceGroup](bool sameSample, bool sameGroup) -> Voice* {
		for (auto& voice : _voices)
		{
			if (voice.bank != &bank || !_backend.IsPlaying(voice.handle))
			{
				continue;
			}
			if ((sameSample && voice.sampleId == header.id) || (sameGroup && voiceGroup != 0 && voice.voiceGroup == voiceGroup))
			{
				return &voice;
			}
		}
		return nullptr;
	};
	if (playType == PlayType::Once)
	{
		if (const auto* playing = findPlaying(true, true))
		{
			return playing->handle;
		}
	}
	else if (playType == PlayType::Restart)
	{
		auto* playing = findPlaying(true, false);
		if (playing == nullptr)
		{
			playing = findPlaying(false, true);
		}
		if (playing != nullptr)
		{
			StopVoice(playing->handle);
		}
	}

	// The atmosphere always passes its own volume, every other header override applies
	const auto volume = std::min(options.volume, k_MaxVolume);
	const auto loopCount = HasOverride(header, AudioBankOverride::Loop) ? header.loop : options.loopCount;
	const auto minDistance = HasOverride(header, AudioBankOverride::MinDist) ? header.minDist : k_DefaultMinDistance;
	const auto maxDistance = HasOverride(header, AudioBankOverride::MaxDist) ? header.maxDist : k_DefaultMaxDistance;
	const auto scale = HasOverride(header, AudioBankOverride::Scale) ? header.scale : k_DefaultDistanceScale;

	auto position = options.position;
	if (HasOverride(header, AudioBankOverride::PosX))
	{
		position.x = header.pos[0];
	}
	if (HasOverride(header, AudioBankOverride::PosY))
	{
		position.y = header.pos[1];
	}
	if (HasOverride(header, AudioBankOverride::PosZ))
	{
		position.z = header.pos[2];
	}
	const bool positional =
	    options.positional || HasOverride(header, AudioBankOverride::PosX) || HasOverride(header, AudioBankOverride::PosY);

	// Playback rate in percent with a random deviation, consuming one random number on every play
	uint32_t pitch = HasOverride(header, AudioBankOverride::Pitch) ? header.pitch : k_DefaultPitchPercent;
	if (pitch == 0)
	{
		pitch = k_DefaultPitchPercent;
	}
	const auto deviation = (pitch * header.pitchDeviation) / 100;
	pitch = pitch - deviation + ((_random.Next() * deviation * 2) / AudioRandom::k_RandMax);
	if (pitch == 0)
	{
		pitch = k_DefaultPitchPercent;
	}
	const auto baseFrequency = header.sampleRate;
	const auto frequency = (baseFrequency * pitch) / 100;
	const auto rate = baseFrequency != 0 ? static_cast<float>(frequency) / static_cast<float>(baseFrequency)
	                                     : static_cast<float>(pitch) / 100.0f;

	const VoiceStart start {
	    .bankName = bank.name,
	    .sampleId = header.id,
	    .volume = volume,
	    .pitchPercent = pitch,
	    .pitch = rate,
	    .loopCount = loopCount,
	    .positional = positional,
	    .position = position,
	    .minDistance = minDistance,
	    .maxDistance = maxDistance,
	    .distanceScale = scale,
	};
	const auto handle = _backend.Start(start);
	if (handle != VoiceBackend::k_InvalidHandle)
	{
		_voices.push_back(Voice {
		    .handle = handle,
		    .bank = &bank,
		    .sampleId = header.id,
		    .voiceGroup = voiceGroup,
		    .atmosInfo = options.atmosInfo,
		    .isAtmosLoop = options.isAtmosLoop,
		    .volume = volume,
		});
	}
	return handle;
}

void AtmosPlayer::SetVoiceVolume(Voice& voice, int32_t volume)
{
	const auto clamped = static_cast<uint32_t>(std::clamp(volume, 0, static_cast<int32_t>(k_MaxVolume)));
	if (!_backend.IsPlaying(voice.handle) || voice.volume == clamped)
	{
		return;
	}
	_backend.SetVolume(voice.handle, clamped);
	voice.volume = clamped;
}

float AtmosPlayer::VolumeToGain(uint32_t volume) const
{
	const auto mixed = ScaleVolume(volume, _masterVolume) * k_QMixerVolumePerStep;
	return static_cast<float>(mixed) / k_QMixerMaxVolume;
}

AtmosPlayer::Voice* AtmosPlayer::FindVoice(VoiceBackend::Handle handle)
{
	const auto iter = std::ranges::find_if(_voices, [handle](const Voice& voice) { return voice.handle == handle; });
	return iter != _voices.end() ? &*iter : nullptr;
}

bool AtmosPlayer::IsVoicePlaying(VoiceBackend::Handle handle) const
{
	const auto known = std::ranges::any_of(_voices, [handle](const Voice& voice) { return voice.handle == handle; });
	return known && _backend.IsPlaying(handle);
}

void AtmosPlayer::StopVoice(VoiceBackend::Handle handle)
{
	const auto iter = std::ranges::find_if(_voices, [handle](const Voice& voice) { return voice.handle == handle; });
	if (iter != _voices.end())
	{
		_backend.Stop(iter->handle);
		_voices.erase(iter);
	}
}

void AtmosPlayer::PruneVoices()
{
	for (auto iter = _voices.begin(); iter != _voices.end();)
	{
		if (!_backend.IsPlaying(iter->handle))
		{
			_backend.Stop(iter->handle);
			iter = _voices.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

std::vector<AtmosPlayer::LoopState> AtmosPlayer::GetLoops() const
{
	std::vector<LoopState> result;
	result.reserve(_loops.size());
	for (const auto& loop : _loops)
	{
		result.push_back({.bankName = loop.bank->name,
		                  .sampleId = loop.header->id,
		                  .group = loop.group,
		                  .current = loop.current,
		                  .target = loop.target,
		                  .playing = loop.playing});
	}
	return result;
}

std::vector<AtmosPlayer::VoiceState> AtmosPlayer::GetVoices() const
{
	std::vector<VoiceState> result;
	for (const auto& voice : _voices)
	{
		if (_backend.IsPlaying(voice.handle))
		{
			result.push_back(
			    {.bankName = voice.bank->name, .sampleId = voice.sampleId, .volume = voice.volume, .loop = voice.isAtmosLoop});
		}
	}
	return result;
}

std::vector<AtmosPlayer::OneShotState> AtmosPlayer::GetQueue() const
{
	std::vector<OneShotState> result;
	result.reserve(_queue.size());
	for (const auto& info : _queue)
	{
		result.push_back(
		    {.bankName = info->bank->name, .sampleId = info->header->id, .group = info->group, .nextTime = info->nextTime});
	}
	return result;
}

} // namespace openblack::audio
