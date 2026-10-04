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

#include <array>
#include <utility>

#include <glm/vec3.hpp>

#include "3D/SkyInterface.h"
#include "SoundMap.h"

namespace openblack::audio
{

/// Port of the ambience part of GAudio: one atmosphere bank per AtmosType whose volume follows the SoundMap.
///
/// Each game turn the SoundMap's volumes become the banks' targets and every bank moves towards its target by
/// 0.02 (0.04 between 0.1 and 0.8), so a full fade takes 30 to 50 turns. Banks switch to their second sample
/// group on land owned by an evil god.
class AtmosAudio
{
public:
	struct TurnInputs
	{
		/// Listener position
		glm::vec3 camera;
		/// Weather at the camera
		AtmosWeather weather;
		bool paused;
		uint32_t turn;
		/// Cinematic letterbox, silences the ambience
		bool widescreen;
		bool videoPlaying;
	};

	/// GAudio::InitAtmos, when a level has finished loading
	void Init();
	/// GAudio::ReleaseAtmosSoundBanks
	void Release();

	/// Alignment of the most influential player at the camera, -1 (evil) to 1 (good). Updated by the game every
	/// turn before EndTurn.
	void SetAlignment(float alignment);

	/// The ambience part of GGame::EndTurn
	void EndTurn(const TurnInputs& inputs);

	[[nodiscard]] const SoundMap& GetSoundMap() const { return _soundMap; }
	[[nodiscard]] const std::array<float, k_AtmosTypeCount>& GetTargets() const { return _targets; }
	[[nodiscard]] const std::array<float, k_AtmosTypeCount>& GetVolumes() const { return _current; }
	[[nodiscard]] float GetAlignmentValue() const { return _alignment; }
	[[nodiscard]] uint32_t GetGroup() const;

	/// LH3DSky::Time2SkyType: 0 day, 1 dusk, 2 night
	[[nodiscard]] static float CalculateSkyType(float time, const SkyInterface::DayNightTimes& times);
	/// fn_005E2240: the value GAudio keeps for an alignment, -1 (evil) to 1 (good)
	[[nodiscard]] static float CalculateAlignmentValue(float alignment);
	/// One turn of a bank's fade towards its target: the new volume and what is sent to the bank (0-127)
	[[nodiscard]] static std::pair<float, int32_t> StepBankVolume(float current, float target);

private:
	void CopyTargets(bool widescreen);
	void ProcessAtmosBanks();

	SoundMap _soundMap;
	std::array<uint32_t, k_AtmosTypeCount> _banks {};
	std::array<float, k_AtmosTypeCount> _targets {};
	std::array<float, k_AtmosTypeCount> _current {};
	float _alignment {0.0f};
	bool _registered {false};
};

} // namespace openblack::audio
