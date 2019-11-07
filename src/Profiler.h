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

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <string_view>

namespace openblack
{

class Profiler
{
  public:
	enum class Stage : uint8_t
	{
		SdlInput,
		UpdateUniforms,
		UpdateEntities,
		GuiLoop,
		SceneDraw,
		ReflectionPass,
		ReflectionDrawSky,
		ReflectionDrawWater,
		ReflectionDrawIsland,
		ReflectionDrawModels,
		ReflectionDrawDebugCross,
		MainPass,
		MainPassDrawSky,
		MainPassDrawWater,
		MainPassDrawIsland,
		MainPassDrawModels,
		MainPassDrawDebugCross,
		GuiDraw,
		RendererFrame,

		_count,
	};

  private:
	struct ScopedSection
	{
		inline explicit ScopedSection(Profiler* profiler, Stage stage):
		    _profiler(profiler), _stage(stage)
		{
			_profiler->Begin(_stage);
		}
		inline ~ScopedSection()
		{
			_profiler->End(_stage);
		}

		Profiler* const _profiler;
		const Stage _stage;
	};

  public:
	struct Scope
	{
		uint8_t _level;
		std::chrono::system_clock::time_point _start;
		std::chrono::system_clock::time_point _end;
		bool _finalized = false;
	};

	static constexpr std::array<std::string_view, static_cast<uint8_t>(Stage::_count)> stageNames = {
		"SDL Input",
		"Update Uniforms",
		"Entities",
		"GUI Loop",
		"Encode Draw Scene",
		"Reflection Pass",
		"Draw Sky",
		"Draw Water",
		"Draw Island",
		"Draw Models",
		"Draw Debug Cross",
		"Main Pass",
		"Draw Sky",
		"Draw Water",
		"Draw Island",
		"Draw Models",
		"Draw Debug Cross",
		"Encode GUI Draw",
		"Renderer Frame",
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
	inline ScopedSection BeginScoped(Stage stage)
	{
		return ScopedSection(this, stage);
	}

	[[nodiscard]] uint8_t GetEntryIndex(int8_t offset) const
	{
		return (_currentEntry + _bufferSize + offset) % _bufferSize;
	}

	static constexpr uint8_t _bufferSize = 100;
	std::array<Entry, _bufferSize> _entries;

  private:
	uint8_t _currentEntry = _bufferSize - 1;
	uint8_t _currentLevel = 0;
};

} // namespace openblack
