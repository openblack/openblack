/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>

#include "Window.h"

namespace openblack::gui
{

class Profiler: public Window
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
		static constexpr uint8_t k_BufferSize = N;
		std::array<T, N> values;
		uint8_t offset = 0;

		[[nodiscard]] T Back() const
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index): perf sensitive
			return values[offset];
		}
		void PushBack(T value)
		{

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index): perf sensitive
			values[offset] = value;
			offset = (offset + 1u) % values.size();
		}
	};
	CircularBuffer<float, 100> _times;
	CircularBuffer<float, 100> _fps;
};

} // namespace openblack::gui
