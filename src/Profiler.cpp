/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Profiler.h"

#include <cassert>

void openblack::Profiler::Begin(Stage stage)
{
	assert(_currentLevel < 255);
	auto& entry = _entries.at(_currentEntry).stages.at(static_cast<uint8_t>(stage));
	entry.level = _currentLevel;
	_currentLevel++;
	entry.start = std::chrono::system_clock::now();
	entry.finalized = false;
}

void openblack::Profiler::End(Stage stage)
{
	assert(_currentLevel > 0);
	auto& entry = _entries.at(_currentEntry).stages.at(static_cast<uint8_t>(stage));
	assert(!entry.finalized);
	_currentLevel--;
	assert(entry.level == _currentLevel);
	entry.end = std::chrono::system_clock::now();
	entry.finalized = true;
}

void openblack::Profiler::Frame()
{
	auto& prevEntry = _entries.at(_currentEntry);
	_currentEntry = (_currentEntry + 1) % k_BufferSize;
	prevEntry.frameEnd = _entries.at(_currentEntry).frameStart = std::chrono::system_clock::now();
}
