/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AnimEffectTable.h"

#include <cstring>

using namespace openblack::audio;

namespace
{
std::vector<int32_t> ReadWords(std::span<const uint8_t> block)
{
	std::vector<int32_t> words(block.size() / sizeof(int32_t));
	std::memcpy(words.data(), block.data(), words.size() * sizeof(int32_t));
	return words;
}
} // namespace

std::optional<AnimEffectTable> AnimEffectTable::Parse(std::span<const uint8_t> animArrayTable,
                                                      std::span<const uint8_t> waveNumTable)
{
	const auto table = ReadWords(animArrayTable);
	const auto waves = ReadWords(waveNumTable);
	if (table.size() < 2 || table[0] < 0 || table[1] < 1)
	{
		return std::nullopt;
	}
	const auto rows = static_cast<size_t>(table[0]);
	const auto columns = static_cast<size_t>(table[1]);
	if (table.size() - 2 < rows * columns)
	{
		return std::nullopt;
	}

	AnimEffectTable result;
	result._keyCount = columns - 1;
	result._keys.reserve(rows * result._keyCount);
	result._samples.reserve(rows);
	for (size_t row = 0; row < rows; ++row)
	{
		const auto* values = &table[2 + (row * columns)];
		result._keys.insert(result._keys.end(), values, values + result._keyCount);

		const auto start = values[result._keyCount];
		if (start < 0 || static_cast<size_t>(start) >= waves.size() || waves[start] < 0 ||
		    waves.size() - start - 1 < static_cast<size_t>(waves[start]))
		{
			return std::nullopt;
		}
		const auto first = waves.begin() + start + 1;
		result._samples.emplace_back(first, first + waves[start]);
	}
	return result;
}

std::span<const int32_t> AnimEffectTable::Find(std::span<const int32_t> keys) const
{
	if (keys.size() < _keyCount)
	{
		return {};
	}

	const std::vector<int32_t>* best = nullptr;
	size_t bestExact = 0;
	for (size_t row = 0; row < _samples.size(); ++row)
	{
		const auto values = std::span(_keys).subspan(row * _keyCount, _keyCount);
		size_t exact = 0;
		bool matches = true;
		for (size_t i = 0; i < _keyCount && matches; ++i)
		{
			if (values[i] == keys[i])
			{
				++exact;
			}
			else
			{
				matches = values[i] == k_AnyKey;
			}
		}
		if (matches && (best == nullptr || exact >= bestExact))
		{
			best = &_samples[row];
			bestExact = exact;
		}
	}
	return best != nullptr ? std::span<const int32_t>(*best) : std::span<const int32_t>();
}
