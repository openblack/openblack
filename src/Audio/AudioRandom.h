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

namespace openblack::audio
{

/// The C runtime random generator statically linked into LHAudioDLL (MSVC 6 LIBCMT rand/srand).
///
/// The audio library owns its own copy of the CRT state, so it is not shared with the game executable. It is
/// reseeded with time(0) whenever an atmosphere bank is registered and on the first sample played, and every
/// LHSamplePlay consumes one value for the pitch deviation, so the atmosphere scheduler and sample playback
/// draw from one stream.
class AudioRandom
{
public:
	static constexpr uint32_t k_RandMax = 0x7FFF;

	void Seed(uint32_t seed) noexcept { _state = seed; }

	uint32_t Next() noexcept
	{
		_state = (_state * 214013u) + 2531011u;
		return (_state >> 16) & k_RandMax;
	}

private:
	uint32_t _state {1};
};

} // namespace openblack::audio
