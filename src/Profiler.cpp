/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Profiler.h"

#include <cassert>

void openblack::Profiler::Begin(Stage stage)
{
	assert(_currentLevel < 255);
	auto& entry  = _entries[_currentEntry]._stages[static_cast<uint8_t>(stage)];
	entry._level = _currentLevel;
	_currentLevel++;
	entry._start     = std::chrono::system_clock::now();
	entry._finalized = false;
}

void openblack::Profiler::End(Stage stage)
{
	assert(_currentLevel > 0);
	auto& entry = _entries[_currentEntry]._stages[static_cast<uint8_t>(stage)];
	assert(!entry._finalized);
	_currentLevel--;
	assert(entry._level == _currentLevel);
	entry._end       = std::chrono::system_clock::now();
	entry._finalized = true;
}

void openblack::Profiler::Frame()
{
	auto& prevEntry     = _entries[_currentEntry];
	_currentEntry       = (_currentEntry + 1) % _bufferSize;
	prevEntry._frameEnd = _entries[_currentEntry]._frameStart = std::chrono::system_clock::now();
}
