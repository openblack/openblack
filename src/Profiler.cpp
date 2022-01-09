/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Profiler.h"

#include <cassert>

void openblack::Profiler::Begin(Stage stage)
{
	assert(_currentLevel < 255);
	auto& entry = _entries[_currentEntry]._stages[static_cast<uint8_t>(stage)];
	entry._level = _currentLevel;
	_currentLevel++;
	entry._start = std::chrono::system_clock::now();
	entry._finalized = false;
}

void openblack::Profiler::End(Stage stage)
{
	assert(_currentLevel > 0);
	auto& entry = _entries[_currentEntry]._stages[static_cast<uint8_t>(stage)];
	assert(!entry._finalized);
	_currentLevel--;
	assert(entry._level == _currentLevel);
	entry._end = std::chrono::system_clock::now();
	entry._finalized = true;
}

void openblack::Profiler::Frame()
{
	auto& prevEntry = _entries[_currentEntry];
	_currentEntry = (_currentEntry + 1) % _bufferSize;
	prevEntry._frameEnd = _entries[_currentEntry]._frameStart = std::chrono::system_clock::now();
}
