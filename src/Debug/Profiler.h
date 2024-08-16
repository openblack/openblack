/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>

#include "Window.h"

namespace openblack::debug::gui
{

class Profiler final: public Window
{
public:
	Profiler() noexcept;
	void Open() noexcept override;
	void Close() noexcept override;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;

private:
	template <typename T, uint8_t N>
	struct CircularBuffer
	{
		static constexpr uint8_t k_BufferSize = N;
		std::array<T, N> values;
		uint8_t offset = 0;

		[[nodiscard]] T Back() const noexcept
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index): perf sensitive
			return values[offset];
		}
		void PushBack(T value) noexcept
		{

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index): perf sensitive
			values[offset] = value;
			offset = (offset + 1u) % values.size();
		}
	};
	CircularBuffer<float, 100> _times;
	CircularBuffer<float, 100> _fps;
};

} // namespace openblack::debug::gui
