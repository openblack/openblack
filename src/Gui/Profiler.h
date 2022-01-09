/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "DebugWindow.h"

#include <array>

namespace openblack::gui
{

class Profiler: public DebugWindow
{
public:
	Profiler();
	void Open() override;
	void Close() override;

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	template <typename T, uint8_t N>
	struct CircularBuffer
	{
		static constexpr uint8_t _bufferSize = N;
		std::array<T, N> _values;
		uint8_t _offset = 0;

		[[nodiscard]] T back() const { return _values[_offset]; }
		void pushBack(T value)
		{
			_values[_offset] = value;
			_offset = (_offset + 1u) % _values.size();
		}
	};
	CircularBuffer<float, 100> _times;
	CircularBuffer<float, 100> _fps;
};

} // namespace openblack::gui
