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

#pragma once

#include <cstdint>

#include <array>
#include <chrono>
#include <map>
#include <string_view>

namespace openblack
{

class Profiler
{
	struct Scope
	{
		uint8_t _level;
		std::chrono::system_clock::time_point _start;
		std::chrono::system_clock::time_point _end;
		bool _finalized = false;
	};

public:
	enum class Stage : uint8_t
	{
		SdlInput,
		UpdatePositions,
		GuiLoop,
		ReflectionPass,
		MainPass,
		MainPassUploadUniforms,
		MainPassDrawScene,
		GuiDraw,
		RendererFrame,

		_count,
	};

	static constexpr std::array<std::string_view, static_cast<uint8_t>(Stage::_count)> stageNames = {
		"SdlInput",
		"UpdatePositions",
		"GuiLoop",
		"ReflectionPass",
		"MainPass",
		"MainPassUploadUniforms",
		"MainPassDrawScene",
		"GuiDraw",
		"RendererFrame",
	};

	struct Entry
	{
		std::chrono::system_clock::time_point _frameStart;
		std::chrono::system_clock::time_point _frameEnd;
		std::array<Scope, static_cast<uint8_t>(Stage::_count)> _stages;
	};

	void Frame();
	void Begin(Stage stage);
	void End(Stage stage);

	static constexpr uint8_t _bufferSize = 100;
	std::array<Entry, _bufferSize> _entries;

private:
	uint8_t _currentEntry = _bufferSize - 1;
	uint8_t _currentLevel = 0;
};

}  // openblack
