/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <PackFile.h>
#include <glm/vec3.hpp>

#include "AudioRandom.h"

namespace openblack::audio
{

/// A voice started on behalf of LHAudioDLL, in the library's units (the arguments LHSamplePlay hands to QMixer)
struct VoiceStart
{
	std::string bankName;
	int32_t sampleId;
	/// 0 to 127, before the master volume
	uint32_t volume;
	/// Playback rate in percent, after the random deviation
	uint32_t pitchPercent;
	/// Playback rate as a ratio of the sample rate
	float pitch;
	/// 0 plays once, n repeats n more times, -1 loops forever
	int32_t loopCount;
	/// Listener relative 3D position, otherwise centred
	bool positional;
	/// LHAudio listener frame: x right, y forward, z up
	glm::vec3 position;
	/// QSound distance mapping: full volume up to minDistance, then attenuated by minDistance / distance up to
	/// maxDistance, with distances multiplied by distanceScale
	float minDistance;
	float maxDistance;
	float distanceScale;
};

/// Where LHAudioDLL voices are actually played
class VoiceBackend
{
public:
	using Handle = uint32_t;
	static constexpr Handle k_InvalidHandle = 0;

	virtual ~VoiceBackend() = default;
	[[nodiscard]] virtual Handle Start(const VoiceStart& start) = 0;
	[[nodiscard]] virtual bool IsPlaying(Handle handle) const = 0;
	/// Volume from 0 to 127, before the master volume
	virtual void SetVolume(Handle handle, uint32_t volume) = 0;
	/// Stops the voice and releases it, the handle is invalid afterwards
	virtual void Stop(Handle handle) = 0;
};

/// Port of the atmosphere part of LHAudioDLL ver7.0 (LHAudioAtmos.cpp) together with the parts of LH_AudioSystem
/// sample playback that it depends on (LHSamplePlay, LHSampleSetVolume, LHSampleIsPlaying and LHSampleStop).
///
/// An atmosphere bank is a sound bank whose sample table is flagged as atmospheric. Each of its samples is either
/// a looping bed (atmosInterval == 0) that plays while the bank is audible, or a one-shot (atmosInterval > 0)
/// that is retriggered every 4 to 16 times atmosInterval game turns at a random position around the listener.
/// Samples carry an atmos group: 0 always plays, otherwise it only plays while the bank is set to that group.
///
/// Volumes are LHAudio units, 0 to 127. Times are game turns: LHAtmosProcess is called once per turn.
class AtmosPlayer
{
public:
	using BankId = uint32_t;
	static constexpr BankId k_InvalidBank = 0;
	static constexpr uint32_t k_MaxVolume = 127;

	/// Seconds since the epoch, seeds the random generator
	using Clock = std::function<uint32_t()>;

	explicit AtmosPlayer(VoiceBackend& backend, Clock clock = {});
	~AtmosPlayer();

	/// LHBankRegister for a bank whose samples are already loaded under bankName. The atmos scheduler only picks
	/// it up when atmosCount (high word of the sample table header) is non-zero.
	BankId RegisterBank(const std::string& bankName, const std::vector<pack::AudioBankSampleHeader>& headers,
	                    uint16_t atmosCount);
	/// LHBankRelease
	void ReleaseBank(BankId id);

	/// LHAtmosProcess: active is false while the game is paused or before turn 6, which silences every atmos voice
	void Process(bool active);
	/// LHAtmosSetBankVolume
	void SetBankVolume(BankId id, int32_t volume);
	/// LHAtmosGetBankVolume
	[[nodiscard]] uint32_t GetBankVolume(BankId id) const;
	/// LHAtmosSetGroup
	void SetGroup(BankId id, uint32_t group);

	/// LHSampleSetMasterVolume
	void SetMasterVolume(uint32_t volume) { _masterVolume = volume; }
	/// Linear gain QMixer plays a volume at
	[[nodiscard]] float VolumeToGain(uint32_t volume) const;

	// Introspection for debugging tools
	struct LoopState
	{
		std::string bankName;
		int32_t sampleId;
		uint32_t group;
		int32_t current;
		uint32_t target;
		bool playing;
	};
	struct OneShotState
	{
		std::string bankName;
		int32_t sampleId;
		uint32_t group;
		uint32_t nextTime;
	};
	struct VoiceState
	{
		std::string bankName;
		int32_t sampleId;
		uint32_t volume;
		bool loop;
	};
	[[nodiscard]] uint32_t GetTick() const { return _tick; }
	[[nodiscard]] std::vector<VoiceState> GetVoices() const;
	[[nodiscard]] std::vector<LoopState> GetLoops() const;
	[[nodiscard]] std::vector<OneShotState> GetQueue() const;

private:
	struct Bank
	{
		std::string name;
		std::vector<pack::AudioBankSampleHeader> headers;
		uint32_t volume {0};
		uint32_t group {0};
	};

	/// One-shot sample scheduled by the atmosphere (LH_AtmosInfo)
	struct AtmosInfo
	{
		Bank* bank;
		const pack::AudioBankSampleHeader* header;
		uint32_t nextTime;
		uint32_t volume;
		uint32_t group;
	};

	/// Looping bed of a bank
	struct Loop
	{
		Bank* bank;
		const pack::AudioBankSampleHeader* header;
		VoiceBackend::Handle voice;
		uint32_t target;
		int32_t current;
		bool playing;
		uint32_t group;
	};

	/// Sample info slot of a playing voice
	struct Voice
	{
		VoiceBackend::Handle handle;
		Bank* bank;
		int32_t sampleId;
		/// Voice group of the sample within its bank, 0 for none
		uint32_t voiceGroup;
		/// The scheduled one-shot this voice was started for, null for loops
		const AtmosInfo* atmosInfo;
		bool isAtmosLoop;
		uint32_t volume;
	};

	/// How LHSamplePlay treats a sample that is already playing
	enum class PlayType : uint32_t
	{
		/// Always start another instance
		Overlap = 1,
		/// Do nothing while the sample, or another of its voice group, plays and hand back that voice
		Once = 2,
		/// Cut off a playing instance of the sample, or else another of its voice group, and start again
		Restart = 3,
	};

	struct PlayOptions
	{
		PlayType playType {PlayType::Restart};
		const AtmosInfo* atmosInfo {nullptr};
		bool isAtmosLoop {false};
		uint32_t volume {0};
		int32_t loopCount {0};
		bool positional {false};
		/// LHAudio listener frame: x right, y forward, z up
		glm::vec3 position {0.0f, 0.0f, 0.0f};
	};

	[[nodiscard]] Bank* FindBank(BankId bank) const;
	void RegisterAtmos(Bank& bank);
	[[nodiscard]] uint32_t ScheduleTime(int32_t interval);
	void Insert(std::unique_ptr<AtmosInfo> info);
	void InsertLoop(const AtmosInfo& info);
	void ProcessOneShot();
	void PlayOneShot(const AtmosInfo& info);

	// LH_AudioSystem sample functions
	VoiceBackend::Handle PlaySample(Bank& bank, const pack::AudioBankSampleHeader& header, const PlayOptions& options);
	void SetVoiceVolume(Voice& voice, int32_t volume);
	[[nodiscard]] Voice* FindVoice(VoiceBackend::Handle handle);
	[[nodiscard]] bool IsVoicePlaying(VoiceBackend::Handle handle) const;
	void StopVoice(VoiceBackend::Handle handle);
	void PruneVoices();
	void Reseed();

	VoiceBackend& _backend;
	Clock _clock;
	AudioRandom _random;
	bool _playSeeded {false};
	uint32_t _masterVolume {k_MaxVolume};

	std::map<BankId, std::unique_ptr<Bank>> _banks;
	BankId _nextBankId {1};

	std::vector<Voice> _voices;
	std::list<Loop> _loops;
	/// One-shots ordered by next trigger time
	std::list<std::unique_ptr<AtmosInfo>> _queue;
	uint32_t _tick {0};
	/// Corner used when a random one-shot position lands on the listener, alternates between the four diagonals
	int32_t _cornerX {1};
	int32_t _cornerY {1};
};

} // namespace openblack::audio
