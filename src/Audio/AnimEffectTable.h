/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <optional>
#include <span>
#include <vector>

namespace openblack::audio
{

/// The animation effects of a sound bank: what the game plays for something that happens, such as a tree being
/// bent or a villager chopping wood, described by a few keys (LHAudioAnimArrayTable and LHAudioWaveNumTable).
///
/// Each row of the table holds a value for each key, or k_AnyKey to accept any value, and the samples the row picks
/// from. The banks of Black & White use five keys, which its editor names after its headers: the size, alignment,
/// object, surface and action of the effect.
class AnimEffectTable
{
public:
	/// A row with this value for a key accepts any value
	static constexpr int32_t k_AnyKey = 0x0FFF0000;

	/// Reads the two blocks of a sound bank. animArrayTable holds the number of rows, the number of columns, then the
	/// rows: a value for each key and, last, where the row's samples start in waveNumTable, counted in 32 bit words.
	/// There they are the number of samples followed by their ids. Null when the blocks don't fit together.
	static std::optional<AnimEffectTable> Parse(std::span<const uint8_t> animArrayTable, std::span<const uint8_t> waveNumTable);

	/// Number of keys a row matches on
	[[nodiscard]] size_t GetKeyCount() const noexcept { return _keyCount; }
	[[nodiscard]] size_t GetRowCount() const noexcept { return _samples.size(); }

	/// The sample ids of the row for keys (LHFindAttribRow). A row is for keys when each of its values is the key's or
	/// k_AnyKey. Of several such rows, the one with the most values equal to the keys wins, the later one if they
	/// tie. Empty when no row is for keys or keys are too few.
	[[nodiscard]] std::span<const int32_t> Find(std::span<const int32_t> keys) const;

private:
	size_t _keyCount {0};
	/// _keyCount values for each row
	std::vector<int32_t> _keys;
	std::vector<std::vector<int32_t>> _samples;
};

} // namespace openblack::audio
